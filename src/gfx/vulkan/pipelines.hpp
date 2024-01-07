#ifndef SRC_GFX_VULKAN_PIPELINE_HPP
#define SRC_GFX_VULKAN_PIPELINE_HPP

#include <boost/unordered/concurrent_flat_map.hpp>
#include <util/threads.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Renderer;
}

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
            PipelineHandle()
                : id {~0UZ}
            {}

            [[nodiscard]] bool isValid() const
            {
                return this->id != ~0UZ;
            }

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

        PipelineHandle  cachePipeline(std::unique_ptr<Pipeline>) const;
        const Pipeline* lookupPipeline(PipelineHandle) const;

        void updateRenderPass(vk::RenderPass, const Swapchain&);

    private:
        mutable std::atomic<std::size_t> next_free_id;
        mutable boost::unordered::
            concurrent_flat_map<PipelineHandle, std::unique_ptr<Pipeline>>
                cache;
    };

    class Pipeline
    {
    public:

        Pipeline()          = default;
        virtual ~Pipeline() = default;

        Pipeline(const Pipeline&)             = delete;
        Pipeline(Pipeline&&)                  = default;
        Pipeline& operator= (const Pipeline&) = delete;
        Pipeline& operator= (Pipeline&&)      = default;

        [[nodiscard]] vk::Pipeline       operator* () const;
        [[nodiscard]] vk::PipelineLayout getLayout() const;

        virtual void recreate(vk::RenderPass, const Swapchain&) = 0;

    protected:
        vk::UniquePipeline       pipeline;
        vk::UniquePipelineLayout layout;
    };

    class ComputePipeline final : public Pipeline
    {
    public:
        ComputePipeline(
            const Renderer&,
            vk::ShaderModule,
            std::span<const vk::DescriptorSetLayout>,
            std::span<const vk::PushConstantRange>,
            const std::string& name);

        void recreate(vk::RenderPass, const Swapchain&) override;
    };

    class GraphicsPipeline final : public Pipeline
    {
    public:
        GraphicsPipeline(
            const Renderer&,
            std::span<
                std::pair<vk::ShaderStageFlagBits, vk::UniqueShaderModule>>,
            vk::PipelineVertexInputStateCreateInfo,
            vk::PrimitiveTopology,
            std::span<const vk::DescriptorSetLayout>,
            std::span<const vk::PushConstantRange>,
            std::string name);

        GraphicsPipeline(
            const Renderer&,
            std::span<
                std::pair<vk::ShaderStageFlagBits, vk::UniqueShaderModule>>,
            vk::PipelineVertexInputStateCreateInfo,
            std::optional<vk::PipelineInputAssemblyStateCreateInfo>,
            std::optional<vk::PipelineTessellationStateCreateInfo>,
            std::optional<vk::PipelineViewportStateCreateInfo>,
            std::optional<vk::PipelineRasterizationStateCreateInfo>,
            std::optional<vk::PipelineMultisampleStateCreateInfo>,
            std::optional<vk::PipelineDepthStencilStateCreateInfo>,
            std::optional<vk::PipelineColorBlendStateCreateInfo>,
            std::span<const vk::DescriptorSetLayout>,
            std::span<const vk::PushConstantRange>,
            std::string name);

        void recreate(vk::RenderPass, const Swapchain&) override;

    private:
        vk::Device  device;
        std::string name;

        std::vector<vk::PipelineShaderStageCreateInfo> shader_create_infos;
        std::vector<vk::UniqueShaderModule>            shaders;
        vk::GraphicsPipelineCreateInfo                 create_info;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_PIPELINE_HPP
