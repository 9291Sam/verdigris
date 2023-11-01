#include "compute_renderer.hpp"
#include <gfx/vulkan/allocator.hpp>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <util/log.hpp>

namespace gfx::vulkan::voxel
{
    ComputeRenderer::ComputeRenderer(
        Device* device_, Allocator* allocator_, vk::Extent2D extent)
        : device {device_}
        , allocator {allocator_}
        , output_image {
              this->allocator,
              this->device->asLogicalDevice(),
              extent,
              vk::Format::eB8G8R8A8Srgb,
              vk::ImageUsageFlagBits::eSampled
                  | vk::ImageUsageFlagBits::eStorage
                  | vk::ImageUsageFlagBits::eTransferDst, // TODO: remove once
                                                          // its generated on
                                                          // the gpu soley
              vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlagBits::eDeviceLocal}
    {
        std::uint32_t srgbFillColor {0x007F8FFF};

        vulkan::Buffer fillBuffer {
            this->allocator,
            sizeof(std::uint32_t) * extent.height * extent.width,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible};

        std::uint32_t* bufferData =
            static_cast<std::uint32_t*>(fillBuffer.getMappedPtr());

        for (std::size_t i = 0; i < extent.height * extent.width; ++i)
        {
            bufferData[i] = srgbFillColor;
        }

        const vk::FenceCreateInfo fenceCreateInfo {
            .sType {vk::StructureType::eFenceCreateInfo},
            .pNext {nullptr},
            .flags {},
        };

        vk::UniqueFence endTransferFence =
            this->device->asLogicalDevice().createFenceUnique(fenceCreateInfo);

        this->device->accessQueue(
            vk::QueueFlagBits::eTransfer,
            [&](vk::Queue queue, vk::CommandBuffer commandBuffer)
            {
                const vk::CommandBufferBeginInfo BeginInfo {
                    .sType {vk::StructureType::eCommandBufferBeginInfo},
                    .pNext {nullptr},
                    .flags {vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
                    .pInheritanceInfo {nullptr},
                };

                commandBuffer.begin(BeginInfo);

                this->output_image.copyFromBuffer(commandBuffer, fillBuffer);

                commandBuffer.end();

                const vk::SubmitInfo SubmitInfo {
                    .sType {vk::StructureType::eSubmitInfo},
                    .pNext {nullptr},
                    .waitSemaphoreCount {0},
                    .pWaitSemaphores {nullptr},
                    .pWaitDstStageMask {nullptr},
                    .commandBufferCount {1},
                    .pCommandBuffers {&commandBuffer},
                    .signalSemaphoreCount {0},
                    .pSignalSemaphores {nullptr},
                };

                queue.submit(SubmitInfo, *endTransferFence);
            });

        const vk::Result result = this->device->asLogicalDevice().waitForFences(
            *endTransferFence, static_cast<vk::Bool32>(true), -1);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to wait for image trnasfer");
    }

    ComputeRenderer::~ComputeRenderer() = default;

    vk::ImageView ComputeRenderer::getImage()
    {
        return *this->output_image;
    }
} // namespace gfx::vulkan::voxel