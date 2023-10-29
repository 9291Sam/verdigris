#ifndef SRC_GFX_VULKAN_PIPELINE_HPP
#define SRC_GFX_VULKAN_PIPELINE_HPP

#include "includes.hpp"
#include "util/log.hpp"
#include <memory>
#include <optional>
#include <span>

namespace gfx::vulkan
{
    class Device;
    class RenderPass;
    class Swapchain;
    class Pipeline;

    enum class PipelineType
    {
        None,
        Flat,
        Voxel,
    };

    constexpr std::size_t PipelineTypeNumberOfValidEntries = 2;

    enum class PipelineVertexType
    {
        Normal,
        None,
    };

    Pipeline createPipeline(
        PipelineType,
        std::shared_ptr<Device>,
        std::shared_ptr<RenderPass>,
        std::shared_ptr<Swapchain>);

    class Pipeline
    {
    public:
        static vk::UniqueShaderModule
        createShaderFromFile(vk::Device, const char* filePath);

        static Pipeline create(
            PipelineVertexType,
            std::shared_ptr<Device>,
            std::shared_ptr<RenderPass>,
            std::shared_ptr<Swapchain>,
            std::span<vk::PipelineShaderStageCreateInfo>,
            vk::PrimitiveTopology,
            vk::UniquePipelineLayout);

    public:

        Pipeline();
        Pipeline(
            std::shared_ptr<Device>,
            std::shared_ptr<RenderPass>,
            std::shared_ptr<Swapchain>,
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
        std::shared_ptr<Device>     device;
        std::shared_ptr<RenderPass> render_pass;
        std::shared_ptr<Swapchain>  swapchain;
        vk::UniquePipelineLayout    layout;
        vk::UniquePipeline          pipeline;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_PIPELINE_HPP
