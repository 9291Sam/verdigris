#include "recordable.hpp"
#include <gfx/renderer.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <ranges>

namespace gfx::recordables
{
    void Recordable::bind(
        vk::CommandBuffer   commandBuffer,
        vk::Pipeline&       currentlyBoundPipeline,
        DescriptorRefArray& currentlyBoundDescriptors) const
    {
        if (currentlyBoundPipeline != **this->pipeline
            && this->pipeline != nullptr)
        {
            commandBuffer.bindPipeline(
                this->pipeline_bind_point, **this->pipeline);

            currentlyBoundPipeline = **this->pipeline;
        }

        for (std::size_t i = 0; i < this->sets.size(); ++i)
        {
            vk::DescriptorSet& currentSet =
                currentlyBoundDescriptors[i];               // NOLINT
            vk::DescriptorSet maybeBindSet = this->sets[i]; // NOLINT

            if (currentSet != maybeBindSet)
            {
                commandBuffer.bindDescriptorSets(
                    this->pipeline_bind_point,
                    this->pipeline->getLayout(),
                    static_cast<std::uint32_t>(i),
                    {maybeBindSet},
                    {});

                currentSet = maybeBindSet;
            }
        }
    }

    void Recordable::accessRenderPass(
        DrawStage                                      accessStage,
        std::function<void(const vulkan::RenderPass*)> func) const
    {
        this->renderer.render_passes.readLock(
            [&](const Renderer::RenderPasses& passes)
            {
                std::optional<vulkan::RenderPass*> maybeRenderPass;

                util::assertFatal(
                    maybeRenderPass.has_value(),
                    "Draw stage {} does not have an associated "
                    "renderpass!",
                    std::to_underlying(accessStage));

                func(*passes.acquireRenderPassFromStage(accessStage));
            });
    }
} // namespace gfx::recordables