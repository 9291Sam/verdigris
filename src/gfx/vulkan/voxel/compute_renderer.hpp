#ifndef SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP
#define SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP

#include "bricked_volume.hpp"
#include <gfx/camera.hpp>
#include <gfx/object.hpp>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/descriptors.hpp>
#include <gfx/vulkan/image.hpp>
#include <random>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Renderer;
} // namespace gfx

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

        void                   tick();
        void                   render(vk::CommandBuffer, const Camera&);
        const vulkan::Image2D& getImage() const;

    private:
        const Renderer&  renderer;
        Device*          device;
        Allocator*       allocator;
        PipelineManager* pipeline_manager;

        Image2D           output_image;
        vk::UniqueSampler output_image_sampler;

        vulkan::Buffer input_uniform_buffer;
        vulkan::Buffer input_voxel_buffer;

        DescriptorSet set;

        std::shared_ptr<gfx::SimpleTriangulatedObject> obj;

        float time_alive;

        static constexpr vk::Extent2D ShaderDispatchSize {32, 32};

        std::atomic<Camera> camera;

        std::mt19937 generator;
        std::size_t  foo;

        bool is_first_pass;

        BrickedVolume volume;

        std::optional<std::future<void>> insert_voxels;
        // fixed world
        // dynamic size
        // dynamic world
    };
} // namespace gfx::vulkan::voxel
#endif // SRC_GFX_VULKAN_VOXEL_COMPUTE_RENDERER_HPP
