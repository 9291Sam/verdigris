#include "compute_renderer.hpp"
#include <gfx/vulkan/allocator.hpp>
#include <gfx/vulkan/device.hpp>

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
                  | vk::ImageUsageFlagBits::eStorage,
              vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlagBits::eDeviceLocal}
    {}
} // namespace gfx::vulkan::voxel