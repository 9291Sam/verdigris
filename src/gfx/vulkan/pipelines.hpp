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
    class GraphicsPipeline;
    class DescriptorPool;
    class ComputePipeline;
    class Allocator;

    enum class GraphicsPipelineType
    {
        NoPipeline,
        Flat,
        Voxel,
        ParallaxRayMarching
    };

    enum class ComputePipelineType
    {
        NoPipeline,
        RayCaster
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

        const GraphicsPipeline& getGraphicsPipeline(GraphicsPipelineType) const;
        const ComputePipeline&  getComputePipeline(ComputePipelineType) const;

    private:
        GraphicsPipeline createGraphicsPipeline(GraphicsPipelineType) const;
        ComputePipeline  createComputePipeline(ComputePipelineType) const;

        vk::Device     device;
        vk::RenderPass render_pass;
        Swapchain*     swapchain;
        Allocator*     allocator;

        util::RwLock<std::unordered_map<GraphicsPipelineType, GraphicsPipeline>>
            graphics_pipeline_cache;
        util::RwLock<std::unordered_map<ComputePipelineType, ComputePipeline>>
            compute_pipeline_cache;
    };

    class GraphicsPipeline // GraphicsPipeline
    {
    public:

        enum class VertexType
        {
            Parallax,
            Normal,
            None,
        };

    public:

        GraphicsPipeline() = default;
        // "optimized" constructor, (shorter)
        GraphicsPipeline(
            VertexType,
            vk::Device,
            vk::RenderPass,
            const Swapchain&,
            std::span<vk::PipelineShaderStageCreateInfo>,
            vk::PrimitiveTopology,
            vk::UniquePipelineLayout);

        // manual constructor
        GraphicsPipeline(
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
        ~GraphicsPipeline() = default;

        GraphicsPipeline(const GraphicsPipeline&)             = delete;
        GraphicsPipeline(GraphicsPipeline&&)                  = default;
        GraphicsPipeline& operator= (const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator= (GraphicsPipeline&&)      = default;

        [[nodiscard]] vk::Pipeline       operator* () const;
        [[nodiscard]] vk::PipelineLayout getLayout() const;

    private:
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline       pipeline;
    };

    class ComputePipeline
    {
    public:

        ComputePipeline() = default;
        ComputePipeline(
            vk::Device,
            vk::UniqueShaderModule,
            std::span<const vk::DescriptorSetLayout>);
        ~ComputePipeline() = default;

        ComputePipeline(const ComputePipeline&)             = delete;
        ComputePipeline(ComputePipeline&&)                  = default;
        ComputePipeline& operator= (const ComputePipeline&) = delete;
        ComputePipeline& operator= (ComputePipeline&&)      = default;

        [[nodiscard]] vk::Pipeline       operator* () const;
        [[nodiscard]] vk::PipelineLayout getLayout() const;


    private:
        vk::UniquePipelineLayout layout;
        vk::UniquePipeline       pipeline;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_PIPELINE_HPP
