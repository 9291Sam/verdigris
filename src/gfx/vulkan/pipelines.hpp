#ifndef SRC_GFX_VULKAN_PIPELINE_HPP
#define SRC_GFX_VULKAN_PIPELINE_HPP

#include <boost/unordered/concurrent_flat_map.hpp>
#include <util/threads.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;
    class RenderPass;
    class Swapchain;
    class Pipeline;
    class GraphicsPipeline;
    class DescriptorPool;
    class ComputePipeline;
    class Allocator;

    class PipelineCache
    {
    public:
        struct PipelineHandle
        {
            std::strong_ordering
            operator<=> (const PipelineHandle&) const = default;
        private:
            friend class PipelineCache;

            explicit PipelineHandle(std::size_t newID)
                : id {newID}
            {}
            std::size_t id;
        };
    public:

        PipelineCache();
        ~PipelineCache() = default;

        PipelineCache(const PipelineCache&)             = delete;
        PipelineCache(PipelineCache&&)                  = delete;
        PipelineCache& operator= (const PipelineCache&) = delete;
        PipelineCache& operator= (PipelineCache&&)      = delete;

        PipelineHandle cachePipeline(Pipeline) const;
        std::pair<vk::Pipeline, vk::PipelineLayout>
            lookupPipeline(PipelineHandle) const;

    private:
        mutable std::atomic<std::size_t> next_free_id;
        mutable boost::unordered::concurrent_flat_map<PipelineHandle, Pipeline>
            cache;
    };

    class Pipeline
    {
    public:

        Pipeline()  = default;
        ~Pipeline() = default;

        explicit Pipeline(const Pipeline&)    = default; // NOLINT
        Pipeline(Pipeline&&)                  = default;
        Pipeline& operator= (const Pipeline&) = delete;
        Pipeline& operator= (Pipeline&&)      = default;

        [[nodiscard]] vk::Pipeline       operator* () const;
        [[nodiscard]] vk::PipelineLayout getLayout() const;

    protected:
        vk::UniquePipeline       pipeline;
        vk::UniquePipelineLayout layout;
    };

    class ComputePipeline : public Pipeline
    {
    public:
        ComputePipeline(
            vk::Device,
            vk::ShaderModule,
            std::array<vk::DescriptorSetLayout, 4>,
            std::optional<vk::PushConstantRange>,
            const std::string& name);
    };

    class GraphicsPipeline : public Pipeline
    {
    public:
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
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_PIPELINE_HPP
