#ifndef SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP
#define SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP

#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/descriptors.hpp>
#include <gfx/vulkan/image.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Renderer;
}

namespace gfx::vulkan
{
    class Device;
    class Allocator;
    class PipelineManager;
} // namespace gfx::vulkan

namespace gfx::vulkan::voxel
{
    class ComputeRenderer
    {
    public:

        ComputeRenderer(
            const Renderer&,
            Device*,
            Allocator*,
            PipelineManager*,
            vk::Extent2D);
        ~ComputeRenderer();

        ComputeRenderer(const ComputeRenderer&)             = delete;
        ComputeRenderer(ComputeRenderer&&)                  = delete;
        ComputeRenderer& operator= (const ComputeRenderer&) = delete;
        ComputeRenderer& operator= (ComputeRenderer&&)      = delete;

        void          render(vk::CommandBuffer);
        vk::ImageView getImage();

    private:
        const Renderer&  renderer;
        Device*          device;
        Allocator*       allocator;
        PipelineManager* pipeline_manager;

        Image2D           output_image;
        vk::UniqueSampler output_image_sampler;
        vulkan::Buffer    input_buffer;

        DescriptorSet set;

        float time_alive;

        // fixed world
        // dynamic size
        // dynamic world
    };
} // namespace gfx::vulkan::voxel
#endif // SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP