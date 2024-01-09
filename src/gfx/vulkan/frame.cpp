#include "frame.hpp"
#include "device.hpp"
#include "render_pass.hpp"
#include <gfx/recordable.hpp>

namespace
{
    constexpr std::uint64_t Timeout =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::seconds {5})
            .count();
}

namespace gfx::vulkan
{
    FlyingFrame::FlyingFrame(Device* device_)
        : device {device_}
        , image_available {nullptr}
        , render_finished {nullptr}
        , frame_in_flight {nullptr}
        , command_pool {nullptr}
        , command_buffer {nullptr}
    {
        // Create semaphores
        {
            const vk::SemaphoreCreateInfo semaphoreCreateInfo {
                .sType {vk::StructureType::eSemaphoreCreateInfo},
                .pNext {nullptr},
                .flags {},
            };

            this->image_available =
                this->device->asLogicalDevice().createSemaphoreUnique(
                    semaphoreCreateInfo);

            this->render_finished =
                this->device->asLogicalDevice().createSemaphoreUnique(
                    semaphoreCreateInfo);
        }

        // Create Fence
        {
            const vk::FenceCreateInfo fenceCreateInfo {
                .sType {vk::StructureType::eFenceCreateInfo},
                .pNext {nullptr},
                .flags {vk::FenceCreateFlagBits::eSignaled},
            };

            this->frame_in_flight =
                this->device->asLogicalDevice().createFenceUnique(
                    fenceCreateInfo);
        }

        // Create CommandPool
        {
            const vk::CommandPoolCreateInfo commandPoolCreateInfo {
                .sType {vk::StructureType::eCommandPoolCreateInfo},
                .pNext {nullptr},
                .flags {},
                .queueFamilyIndex {
                    this->device->getMainGraphicsQueue().getFamilyIndex()}};

            this->command_pool =
                this->device->asLogicalDevice().createCommandPoolUnique(
                    commandPoolCreateInfo);
        }

        // Allocate CommandBuffer
        {
            const vk::CommandBufferAllocateInfo commandBufferAllocateInfo {
                .sType {vk::StructureType::eCommandBufferAllocateInfo},
                .pNext {nullptr},
                .commandPool {*this->command_pool},
                .level {vk::CommandBufferLevel::ePrimary},
                .commandBufferCount {1}};

            this->command_buffer = std::move(
                device_->asLogicalDevice()
                    .allocateCommandBuffersUnique(commandBufferAllocateInfo)
                    .at(0));
        }
    }

    FlyingFrame::~FlyingFrame()
    {
        if (this->frame_in_flight)
        {
            const vk::Result result =
                this->device->asLogicalDevice().waitForFences(
                    *this->frame_in_flight, vk::True, Timeout);

            util::assertFatal(
                result == vk::Result::eSuccess,
                "Failed to wait for frame to complete drawing {} |  "
                "timeout(ns) {}",
                vk::to_string(result),
                Timeout);
        }
    }

    vk::Fence FlyingFrame::getExecutionFinishFence() const
    {
        return *this->frame_in_flight;
    }

    std::expected<void, FlyingFrame::ResizeNeeded>
    FlyingFrame::recordAndDisplay(
        Camera camera,
        const std::map<
            Renderer::DrawStage,
            std::pair<
                std::optional<vulkan::RenderPass*>,
                std::span<const Recordable*>>>& recordables,
        vk::SwapchainKHR                        presentSwapchain,
        std::uint32_t            presentSwapchainFramebufferIndex,
        std::optional<vk::Fence> maybePreviousFrameInFlightFence)
    {
        std::vector<std::pair<
            std::optional<vulkan::RenderPass*>,
            std::vector<const Recordable*>>>
            sortedRecordables;

        // Initialize sortedRecordables
        {
            for (const auto& [stage, renderPassAndUnsortedRecordables] :
                 recordables)
            {
                const auto& [maybeRenderPass, unsortedRecordables] =
                    renderPassAndUnsortedRecordables;

                std::vector<const Recordable*> localSortedRecordables {
                    unsortedRecordables.begin(), unsortedRecordables.end()};

                std::ranges::sort(
                    localSortedRecordables,
                    [](const Recordable* l, const Recordable* r)
                    {
                        return *l < *r;
                    });

                sortedRecordables.push_back(
                    {maybeRenderPass, std::move(localSortedRecordables)});
            }
        }

        {
            this->device->asLogicalDevice().resetCommandPool(
                *this->command_pool);

            this->device->asLogicalDevice().resetFences(*this->frame_in_flight);

            const vk::CommandBufferBeginInfo commandBufferBeginInfo {
                .sType {vk::StructureType::eCommandBufferBeginInfo},
                .pNext {nullptr},
                .flags {},
                .pInheritanceInfo {nullptr},
            };

            this->command_buffer->begin(commandBufferBeginInfo);

            vk::Pipeline                   currentPipeline {};
            Recordable::DescriptorRefArray currentDescriptors {};

            for (const auto& [maybeRenderPass, renderPassRecordables] :
                 sortedRecordables)
            {
                auto recordFunc = [&] noexcept
                {
                    for (const Recordable* r : renderPassRecordables)
                    {
                        r->bind(
                            *this->command_buffer,
                            currentPipeline,
                            currentDescriptors);
                        r->record(*this->command_buffer, camera);
                    }
                };

                if (maybeRenderPass.has_value())
                {
                    (*maybeRenderPass)
                        ->recordWith(
                            *this->command_buffer, std::move(recordFunc));
                }
                else
                {
                    recordFunc();
                }

                currentPipeline    = nullptr;
                currentDescriptors = {nullptr, nullptr, nullptr, nullptr};
            }

            this->command_buffer->end();

            const vk::PipelineStageFlags waitStages =
                vk::PipelineStageFlagBits::eColorAttachmentOutput;

            vk::SubmitInfo submitInfo {
                .sType {vk::StructureType::eSubmitInfo},
                .pNext {nullptr},
                .waitSemaphoreCount {1},
                .pWaitSemaphores {&*this->image_available},
                .pWaitDstStageMask {&waitStages},
                .commandBufferCount {1},
                .pCommandBuffers {&*this->command_buffer},
                .signalSemaphoreCount {1},
                .pSignalSemaphores {&*this->render_finished},
            };

            bool shouldResize = false;

            util::assertFatal(
                this->device->getMainGraphicsQueue().tryAccess(
                    [&](vk::Queue queue, vk::CommandBuffer)
                    {
                        queue.submit(submitInfo, *this->frame_in_flight);

                        vk::PresentInfoKHR presentInfo {
                            .sType {vk::StructureType::ePresentInfoKHR},
                            .pNext {nullptr},
                            .waitSemaphoreCount {1},
                            .pWaitSemaphores {&*this->render_finished},
                            .swapchainCount {1},
                            .pSwapchains {&presentSwapchain},
                            .pImageIndices {&presentSwapchainFramebufferIndex},
                            .pResults {nullptr},
                        };

                        if (maybePreviousFrameInFlightFence.has_value())
                        {
                            const vk::Result result =
                                this->device->asLogicalDevice().waitForFences(
                                    *maybePreviousFrameInFlightFence,
                                    vk::True,
                                    Timeout);

                            util::assertFatal(
                                result == vk::Result::eSuccess,
                                "Failed to wait for frame to complete drawing "
                                "{}"
                                "| timeout {} ns ",
                                vk::to_string(result),
                                Timeout);
                        }

                        {
                            VkResult result =
                                VULKAN_HPP_DEFAULT_DISPATCHER.vkQueuePresentKHR(
                                    static_cast<VkQueue>(queue),
                                    reinterpret_cast< // NOLINT
                                        const VkPresentInfoKHR*>(&presentInfo));

                            if (result == VK_ERROR_OUT_OF_DATE_KHR
                                || result == VK_SUBOPTIMAL_KHR)
                            {
                                util::logTrace(
                                    "Present result {}",
                                    vk::to_string(vk::Result {result}));

                                shouldResize = true;
                                return;
                            }
                            else if (result == VK_SUCCESS)
                            {}
                            else
                            {
                                util::panic(
                                    "Unhandled error! {}",
                                    vk::to_string(vk::Result {result}));
                            }
                        }
                    },
                    false),
                "main graphics queue was not available");

            if (shouldResize)
            {
                return std::unexpected(ResizeNeeded {});
            }
            else
            {
                return {};
            }
        }
    }
} // namespace gfx::vulkan
