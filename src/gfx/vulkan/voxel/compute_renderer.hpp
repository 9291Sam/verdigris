#ifndef SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP
#define SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP

#include <gfx/vulkan/image.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;
    class Allocator;
} // namespace gfx::vulkan

namespace gfx::vulkan::voxel
{
    class ComputeRenderer
    {
    public:

        ComputeRenderer(Device*, Allocator*, vk::Extent2D);
        ~ComputeRenderer();

        ComputeRenderer(const ComputeRenderer&)             = delete;
        ComputeRenderer(ComputeRenderer&&)                  = delete;
        ComputeRenderer& operator= (const ComputeRenderer&) = delete;
        ComputeRenderer& operator= (ComputeRenderer&&)      = delete;

        // void          render(vk::CommandBuffer);
        vk::ImageView getImage();

    private:
        Device*    device;
        Allocator* allocator;

        Image2D output_image;

        // fill cpu
        // fixed size image solid color
        // gradient
        // fixed world
        // dynamic size
        // dynamic world
    };
} // namespace gfx::vulkan::voxel
#endif // SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP