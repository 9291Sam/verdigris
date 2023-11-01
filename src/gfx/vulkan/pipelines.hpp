#ifndef SRC_GFX_VULKAN_PIPELINE_HPP
#define SRC_GFX_VULKAN_PIPELINE_HPP

#include <memory>
#include <unordered_map>
#include <util/threads.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;
    class RenderPass;
    class Swapchain;
    class Pipeline;
    class Allocator;

    enum class PipelineType
    {
        NoPipeline,
        Flat,
        Voxel,
    };

    class PipelineManager
    {
    public:

        PipelineManager(vk::Device, vk::RenderPass, Swapchain*, Allocator*);
        ~PipelineManager() = default;

        PipelineManager(const PipelineManager&)             = delete;
        PipelineManager(PipelineManager&&)                  = delete;
        PipelineManager& operator= (const PipelineManager&) = delete;
        PipelineManager& operator= (PipelineManager&&)      = delete;

        const Pipeline& getPipeline(PipelineType) const;

    private:
        Pipeline createPipeline(PipelineType) const;

        vk::Device     device;
        vk::RenderPass render_pass;
        Swapchain*     swapchain;
        Allocator*     allocator;

        util::RwLock<std::unordered_map<PipelineType, Pipeline>> cache;
    };

    class Pipeline // GraphicsPipeline
    {
    public:

        enum class VertexType
        {
            Normal,
            None,
        };

    public:

        Pipeline() = default;
        // "optimized" constructor, (shorter)
        Pipeline(
            VertexType,
            vk::Device,
            vk::RenderPass,
            const Swapchain&,
            std::span<vk::PipelineShaderStageCreateInfo>,
            vk::PrimitiveTopology,
            vk::UniquePipelineLayout);

        // manual constructor
        Pipeline(
            vk::Device,
            vk::RenderPass,
            std::span<vk::PipelineShaderStageCreateInfo>,
            vk::PipelineVertexInputStateCreateInfo,
            std::optional<vk::PipelineInputAssemblyStateCreateInfo>,
            std::optional<vk::PipelineTessellationStateCreateInfo>,
            std::optional<vk::PipelineViewportStateCreateInfo>,
            std::optional<vk::PipelineRasterizationStateCreateInfo>,
            std::optional<vk::PipelineMultisampleStateCreateInfo>,
            std::optional<vk::PipelineDepthStencilStateCreateInfo>,
            std::optional<vk::PipelineColorBlendStateCreateInfo>,
            vk::UniquePipelineLayout);
        ~Pipeline() = default;

        Pipeline(const Pipeline&)             = delete;
        Pipeline(Pipeline&&)                  = default;
        Pipeline& operator= (const Pipeline&) = delete;
        Pipeline& operator= (Pipeline&&)      = default;

        [[nodiscard]] vk::Pipeline       operator* () const;
        [[nodiscard]] vk::PipelineLayout getLayout() const;

    private:
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline       pipeline;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_PIPELINE_HPP
