#include "render_pass.hpp"
#include "device.hpp"
#include "image.hpp"
#include "swapchain.hpp"
#include <util/log.hpp>

namespace gfx::vulkan
{

    RenderPass::RenderPass(
        Device* device_, Swapchain* swapchain, const Image2D& depthBuffer)
        : render_pass {nullptr}
        , device {device_}
        , next_frame_index {static_cast<std::size_t>(-1)}
    {
        // Initialize render pass
        {
            const std::array<vk::AttachmentDescription, 2> attachments {
                vk::AttachmentDescription {
                    .flags {},
                    .format {swapchain->getSurfaceFormat().format},
                    .samples {vk::SampleCountFlagBits::e1},
                    .loadOp {vk::AttachmentLoadOp::eClear},
                    .storeOp {vk::AttachmentStoreOp::eStore},
                    .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                    .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                    .initialLayout {vk::ImageLayout::eUndefined},
                    .finalLayout {vk::ImageLayout::ePresentSrcKHR},
                },
                vk::AttachmentDescription {
                    .flags {},
                    .format {depthBuffer.getFormat()},
                    .samples {vk::SampleCountFlagBits::e1},
                    .loadOp {vk::AttachmentLoadOp::eClear},
                    .storeOp {vk::AttachmentStoreOp::eDontCare},
                    .stencilLoadOp {vk::AttachmentLoadOp::eDontCare},
                    .stencilStoreOp {vk::AttachmentStoreOp::eDontCare},
                    .initialLayout {vk::ImageLayout::eUndefined},
                    .finalLayout {
                        vk::ImageLayout::eDepthStencilAttachmentOptimal},
                },
            };

            const vk::AttachmentReference colorAttachmentReference {
                .attachment {0},
                .layout {vk::ImageLayout::eColorAttachmentOptimal},
            };

            const vk::AttachmentReference depthAttachmentReference {
                .attachment {1},
                .layout {vk::ImageLayout::eDepthStencilAttachmentOptimal},
            };

            const vk::SubpassDescription subpass {
                .flags {},
                .pipelineBindPoint {
                    VULKAN_HPP_NAMESPACE::PipelineBindPoint::eGraphics},
                .inputAttachmentCount {0},
                .pInputAttachments {nullptr},
                .colorAttachmentCount {1},
                .pColorAttachments {&colorAttachmentReference},
                .pResolveAttachments {nullptr},
                .pDepthStencilAttachment {&depthAttachmentReference},
                .preserveAttachmentCount {0},
                .pPreserveAttachments {nullptr},
            };

            const vk::SubpassDependency subpassDependency {
                .srcSubpass {VK_SUBPASS_EXTERNAL},
                .dstSubpass {0},
                .srcStageMask {
                    vk::PipelineStageFlagBits::eColorAttachmentOutput
                    | vk::PipelineStageFlagBits::eEarlyFragmentTests},
                .dstStageMask {
                    vk::PipelineStageFlagBits::eColorAttachmentOutput
                    | vk::PipelineStageFlagBits::eEarlyFragmentTests},
                .srcAccessMask {vk::AccessFlagBits::eNone},
                .dstAccessMask {
                    vk::AccessFlagBits::eColorAttachmentWrite
                    | vk::AccessFlagBits::eDepthStencilAttachmentWrite},
                .dependencyFlags {},
            };

            const vk::RenderPassCreateInfo renderPassCreateInfo {
                .sType {vk::StructureType::eRenderPassCreateInfo},
                .pNext {nullptr},
                .flags {},
                .attachmentCount {attachments.size()},
                .pAttachments {attachments.data()},
                .subpassCount {1},
                .pSubpasses {&subpass},
                .dependencyCount {1},
                .pDependencies {&subpassDependency},
            };

            this->render_pass =
                this->device->asLogicalDevice().createRenderPassUnique(
                    renderPassCreateInfo);
        }

        // initialize framebuffers
        {
            for (const vk::UniqueImageView& renderView :
                 swapchain->getRenderTargets())
            {
                std::array<vk::ImageView, 2> attachments {
                    *renderView, *depthBuffer};

                vk::FramebufferCreateInfo frameBufferCreateInfo {
                    .sType {vk::StructureType::eFramebufferCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .renderPass {*this->render_pass},
                    .attachmentCount {attachments.size()},
                    .pAttachments {attachments.data()},
                    .width {swapchain->getExtent().width},
                    .height {swapchain->getExtent().height},
                    .layers {1},
                };

                framebuffers.push_back(
                    this->device->asLogicalDevice().createFramebufferUnique(
                        frameBufferCreateInfo));
            }
        }

        this->frames.resize(this->framebuffers.size());

        for (Frame& f : this->frames)
        {
            f = Frame {
                &this->framebuffers,
                this->device,
                swapchain,
                *this->render_pass};
        }
    }

    vk::RenderPass RenderPass::operator* () const
    {
        return *this->render_pass;
    }

    Frame& RenderPass::getNextFrame()
    {
        this->next_frame_index =
            (this->next_frame_index + 1) % this->frames.size();

        return this->frames.at(this->next_frame_index);
    }

    Frame::Frame()
        : device {nullptr}
        , swapchain {nullptr}
        , framebuffers {nullptr}
        , image_available {nullptr}
        , render_finished {nullptr}
        , frame_in_flight {nullptr}
    {}

    Frame::Frame(
        std::vector<vk::UniqueFramebuffer>* framebuffers_,
        Device*                             device_,
        Swapchain*                          swapchain_,
        vk::RenderPass                      renderPass)
        : device {device_}
        , swapchain {swapchain_}
        , framebuffers {framebuffers_}
        , render_pass {renderPass}
        , image_available {nullptr}
        , render_finished {nullptr}
        , frame_in_flight {nullptr}
    {
        util::assertFatal(this->render_pass != nullptr, "null renderpass");

        const vk::SemaphoreCreateInfo semaphoreCreateInfo {
            .sType {vk::StructureType::eSemaphoreCreateInfo},
            .pNext {nullptr},
            .flags {},
        };

        const vk::FenceCreateInfo fenceCreateInfo {
            .sType {vk::StructureType::eFenceCreateInfo},
            .pNext {nullptr},
            .flags {vk::FenceCreateFlagBits::eSignaled},
        };

        this->image_available =
            this->device->asLogicalDevice().createSemaphoreUnique(
                semaphoreCreateInfo);
        this->render_finished =
            this->device->asLogicalDevice().createSemaphoreUnique(
                semaphoreCreateInfo);
        this->frame_in_flight =
            this->device->asLogicalDevice().createFenceUnique(fenceCreateInfo);
    }

    std::expected<void, Frame::ResizeNeeded>
    Frame::render(Camera camera, const std::span<const Object*> unsortedObjects)
    {
        std::optional<bool> shouldResize = std::nullopt;

        std::vector<const Object*> sortedObjects;
        sortedObjects.insert(
            sortedObjects.cend(),
            unsortedObjects.begin(),
            unsortedObjects.end());
        std::ranges::sort(
            sortedObjects,
            [](const Object* l, const Object* r)
            {
                return *l < *r;
            });

        this->device->accessQueue(
            vk::QueueFlagBits::eGraphics,
            [&](vk::Queue queue, vk::CommandBuffer commandBuffer)
            {
                const std::uint64_t timeout =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::duration<std::uint64_t> {1})
                        .count();

                // TODO: should this synchronization be moved to the end so we
                // can pre-draw stuff?
                {
                    vk::Result result =
                        this->device->asLogicalDevice().waitForFences(
                            *this->frame_in_flight,
                            static_cast<vk::Bool32>(true),
                            timeout);

                    util::assertFatal(
                        result == vk::Result::eSuccess,
                        "Failed to wait for render fence {}",
                        vk::to_string(result));
                }

                std::uint32_t nextImageIndex = -1;
                {
                    const auto [result, maybeNextFrameBufferIndex] =
                        this->device->asLogicalDevice().acquireNextImageKHR(
                            **this->swapchain, timeout, *this->image_available);

                    switch (result)
                    {
                    case vk::Result::eErrorOutOfDateKHR:
                        util::logTrace(
                            "Acquired invalid next image. Are we in a "
                            "resize?");
                        shouldResize = true;
                        return;

                    case vk::Result::eSuboptimalKHR:
                        util::logTrace(
                            "Acquired suboptimal next image. Are we in a "
                            "resize?");
                        [[fallthrough]];
                    case vk::Result::eSuccess:
                        nextImageIndex = static_cast<std::uint32_t>(
                            maybeNextFrameBufferIndex);
                        break;
                    default:
                        util::panic(
                            "Invalid acquireNextImage {}",
                            vk::to_string(result));
                    }
                }

                this->device->asLogicalDevice().resetFences(
                    *this->frame_in_flight);

                const vk::CommandBufferBeginInfo commandBufferBeginInfo {
                    .sType {vk::StructureType::eCommandBufferBeginInfo},
                    .pNext {nullptr},
                    .flags {},
                    .pInheritanceInfo {nullptr},
                };

                commandBuffer.begin(commandBufferBeginInfo);

                // union initialization syntax my beloved :heart:
                // clang-format off
                std::array<vk::ClearValue, 2> clearValues
                {
                    vk::ClearValue
                    {
                        .color
                        {
                            vk::ClearColorValue {
                                std::array<float, 4> {0.01f, 0.3f, 0.4f, 1.0f}
                            }
                        }
                    },
                    vk::ClearValue
                    {
                        .depthStencil
                        {
                            vk::ClearDepthStencilValue
                            {
                                .depth {1.0f},
                                .stencil {0}
                            }
                        }
                    }
                };
                // clang-format on

                vk::RenderPassBeginInfo renderPassBeginInfo {
                    .sType {vk::StructureType::eRenderPassBeginInfo},
                    .pNext {nullptr},
                    .renderPass {this->render_pass},
                    .framebuffer {*this->framebuffers->at(nextImageIndex)},
                    .renderArea {vk::Rect2D {
                        .offset {0, 0},
                        .extent {this->swapchain->getExtent()},
                    }},
                    .clearValueCount {clearValues.size()},
                    .pClearValues {clearValues.data()},
                };

                commandBuffer.beginRenderPass(
                    renderPassBeginInfo, vk::SubpassContents::eInline);

                BindState bindState {};

                for (const Object* o : sortedObjects)
                {
                    o->bindAndDraw(commandBuffer, bindState, camera);
                }

                // if (menu != std::nullopt)
                // {
                //     (*menu)->draw(commandBuffer);
                // }

                commandBuffer.endRenderPass();
                commandBuffer.end();

                const vk::PipelineStageFlags waitStages =
                    vk::PipelineStageFlagBits::eColorAttachmentOutput;

                vk::SubmitInfo submitInfo {
                    .sType {vk::StructureType::eSubmitInfo},
                    .pNext {nullptr},
                    .waitSemaphoreCount {1},
                    .pWaitSemaphores {&*this->image_available},
                    .pWaitDstStageMask {&waitStages},
                    .commandBufferCount {1},
                    .pCommandBuffers {&commandBuffer},
                    .signalSemaphoreCount {1},
                    .pSignalSemaphores {&*this->render_finished},
                };

                queue.submit(submitInfo, *this->frame_in_flight);

                vk::SwapchainKHR swapchainPtr = **this->swapchain;

                vk::PresentInfoKHR presentInfo {
                    .sType {vk::StructureType::ePresentInfoKHR},
                    .pNext {nullptr},
                    .waitSemaphoreCount {1},
                    .pWaitSemaphores {&*this->render_finished},
                    .swapchainCount {1},
                    .pSwapchains {&swapchainPtr},
                    .pImageIndices {&nextImageIndex},
                    .pResults {nullptr},
                };

                {
                    VkResult result =
                        VULKAN_HPP_DEFAULT_DISPATCHER.vkQueuePresentKHR(
                            static_cast<VkQueue>(queue),
                            reinterpret_cast<const VkPresentInfoKHR*>(
                                &presentInfo));

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

                const vk::Result result =
                    this->device->asLogicalDevice().waitForFences(
                        *this->frame_in_flight,
                        static_cast<vk::Bool32>(true),
                        timeout);

                util::assertFatal(
                    result == vk::Result::eSuccess,
                    "Failed to wait for frame to complete drawing {}",
                    vk::to_string(result));

                shouldResize = false;
                return;
            });

        if (shouldResize.value()) // NOLINT
        {
            return std::unexpected(ResizeNeeded {});
        }
        else
        {
            return {};
        }
    }

} // namespace gfx::vulkan
