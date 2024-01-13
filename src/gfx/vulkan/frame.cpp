#include "frame.hpp"
#include "device.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"
#include <gfx/recordables/recordable.hpp>
#include <ranges>
#include <util/log.hpp>

namespace
{
    constexpr std::uint64_t Timeout =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::seconds {5})
            .count();
} // namespace

namespace gfx::vulkan
{
    FrameManager::FrameManager(
        Device*                device_,
        Swapchain*             swapchain_,
        const vulkan::Image2D& depthBuffer,
        vk::RenderPass         finalRasterPass,
        std::size_t            numberOfFlyingFrames)
        : device {device_}
        , swapchain {swapchain_} // clang-format off
        // clang-format on
        , final_raster_pass {finalRasterPass}
        , maybe_previous_frame_fence {std::nullopt}
        , flying_frame_index {0}
        , number_of_flying_frames {numberOfFlyingFrames}
    {
        // Create framebuffers && FlyingFrames
        {
            std::span<const vk::UniqueImageView> swapchainImages =
                this->swapchain->getRenderTargets();

            this->swapchain_framebuffers.resize(swapchainImages.size());

            // I love when std::views::zip can't iterate over a
            // (const T&, T&)
            for (std::size_t i = 0; i < swapchainImages.size(); ++i)
            {
                const vk::UniqueImageView& swapchainImage = swapchainImages[i];
                vk::UniqueFramebuffer&     uninitializedSwapchainFramebuffer =
                    this->swapchain_framebuffers[i];

                std::array<vk::ImageView, 2> attachments {
                    *swapchainImage, *depthBuffer};

                vk::FramebufferCreateInfo frameBufferCreateInfo {
                    .sType {vk::StructureType::eFramebufferCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .renderPass {this->final_raster_pass},
                    .attachmentCount {attachments.size()},
                    .pAttachments {attachments.data()},
                    .width {this->swapchain->getExtent().width},
                    .height {this->swapchain->getExtent().height},
                    .layers {1},
                };

                uninitializedSwapchainFramebuffer =
                    this->device->asLogicalDevice().createFramebufferUnique(
                        frameBufferCreateInfo);

                this->flying_frames.push_back(FlyingFrame {this->device});
            }
        }
    }

    FrameManager::~FrameManager()
    {
        if (maybe_previous_frame_fence.has_value())
        {
            vk::Result result = this->device->asLogicalDevice().waitForFences(
                *this->maybe_previous_frame_fence, vk::True, Timeout);

            util::assertFatal(
                result == vk::Result::eSuccess,
                "Failed to wait for previous frame | Reason: {}",
                vk::to_string(result));
        }
    }

    // find final raster pass
    // update its framebuffer

    std::expected<void, ResizeNeeded> FrameManager::renderObjectsFromCamera(
        Camera                                            camera,
        std::vector<std::pair<
            std::optional<const vulkan::RenderPass*>,
            std::vector<const recordables::Recordable*>>> renderStages,
        const vulkan::PipelineCache&                      cache) // NOLINT
    {
        // Iterate wrapping
        // number_of_flying_frames: 3
        // 0 -> 1 -> 2 -> 0 -> 1 -> 2 -> 0
        ++this->flying_frame_index %= this->number_of_flying_frames;

        std::expected resizeResult =
            this->flying_frames[this->flying_frame_index].recordAndDisplay(
                camera,
                std::move(renderStages),
                cache,
                **this->swapchain,
                this->final_raster_pass,
                this->swapchain_framebuffers,
                this->swapchain->getExtent(),
                this->maybe_previous_frame_fence);

        this->maybe_previous_frame_fence =
            this->flying_frames[this->flying_frame_index]
                .getExecutionFinishFence();

        return resizeResult;
    }

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

    std::expected<void, ResizeNeeded> FlyingFrame::recordAndDisplay(
        Camera                                            camera,
        std::vector<std::pair<
            std::optional<const vulkan::RenderPass*>,
            std::vector<const recordables::Recordable*>>> recordables, // NOLINT
        const vulkan::PipelineCache&                      cache,
        vk::SwapchainKHR                                  presentSwapchain,
        vk::RenderPass                                    finalRasterPass,
        std::span<const vk::UniqueFramebuffer>            framebuffers,
        vk::Extent2D                                      framebufferExtent,
        std::optional<vk::Fence> maybePreviousFrameInFlightFence)
    {
        this->device->asLogicalDevice().resetCommandPool(*this->command_pool);

        // acquire next image
        std::uint32_t nextImageIndex = ~std::uint32_t {0};
        {
            const auto [result, maybeNextFrameBufferIndex] =
                this->device->asLogicalDevice().acquireNextImageKHR(
                    presentSwapchain, Timeout, *this->image_available);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (result)
            {
            case vk::Result::eErrorOutOfDateKHR:
                util::logTrace(
                    "Acquired invalid next image. Are we in a "
                    "resize?");
                return std::unexpected(ResizeNeeded {});

            case vk::Result::eSuboptimalKHR:
                util::logTrace(
                    "Acquired suboptimal next image. Are we in a "
                    "resize?");
                [[fallthrough]];
            case vk::Result::eSuccess:
                nextImageIndex =
                    static_cast<std::uint32_t>(maybeNextFrameBufferIndex);
                break;
            default:
                util::panic(
                    "Invalid acquireNextImage {}", vk::to_string(result));
                break;
            }
#pragma clang diagnostic pop
        }

        bool updatedRenderpass = false;

        for (auto& [maybeRenderpass, unsortedRecordables] : recordables)
        {
            // sort recordables
            // std::ranges::sort(
            //     unsortedRecordables,
            //     [](const recordables::Recordable* l,
            //        const recordables::Recordable* r)
            //     {
            //         return l < r;
            //     });

            // update the raster pass
            if (maybeRenderpass.has_value()
                && ***maybeRenderpass == finalRasterPass)
            {
                util::assertFatal(
                    !updatedRenderpass,
                    "Renderpass was discovered multiple times!");

                updatedRenderpass = true;
                (*maybeRenderpass)
                    ->setFramebuffer(
                        *framebuffers[nextImageIndex], framebufferExtent);
            }
        }

        this->device->asLogicalDevice().resetFences(*this->frame_in_flight);

        const vk::CommandBufferBeginInfo commandBufferBeginInfo {
            .sType {vk::StructureType::eCommandBufferBeginInfo},
            .pNext {nullptr},
            .flags {},
            .pInheritanceInfo {nullptr},
        };

        this->command_buffer->begin(commandBufferBeginInfo);

        const vulkan::Pipeline* currentPipeline {};
        auto                    getPipelineLayout = [&]
        {
            return currentPipeline != nullptr ? currentPipeline->getLayout()
                                              : nullptr;
        };
        gfx::recordables::Recordable::DescriptorRefArray currentDescriptors {};

        std::size_t idx = 0;
        for (const auto& [maybeRenderPass, renderPassRecordables] : recordables)
        {
            auto recordFunc = [&] noexcept
            {
                for (const recordables::Recordable* r : renderPassRecordables)
                {
                    r->bind(
                        *this->command_buffer,
                        cache,
                        &currentPipeline,
                        currentDescriptors);

                    r->record(
                        *this->command_buffer, getPipelineLayout(), camera);
                }

                util::logDebug(
                    "Recorded {} @ {}", renderPassRecordables.size(), idx++);
            };

            if (maybeRenderPass.has_value())
            {
                (*maybeRenderPass)
                    ->recordWith(*this->command_buffer, std::move(recordFunc));
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
                        .pImageIndices {&nextImageIndex},
                        .pResults {nullptr},
                    };

                    // TODO: can this be moved after so we have multiple
                    // presentations ready?
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
} // namespace gfx::vulkan
