#include "recordable.hpp"
#include <gfx/renderer.hpp>
#include <gfx/vulkan/image.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <ranges>

namespace gfx::recordables
{
    // placed here to fix Recordable's vtable
    Recordable::~Recordable() = default;

    void Recordable::bind(
        vk::CommandBuffer            commandBuffer,
        const vulkan::PipelineCache& pipelineCache,
        const vulkan::Pipeline**     currentlyBoundPipeline,
        DescriptorRefArray&          currentlyBoundDescriptors) const
    {
        const auto [handle, bindPoint] = this->getPipeline(pipelineCache);

        if (!handle.isValid())
        {
            return;
        }

        std::expected<
            const vulkan::Pipeline*,
            vulkan::PipelineCache::InvalidCacheHandle>
            shouldBePipeline = pipelineCache.lookupPipeline(handle);

        if (!shouldBePipeline.has_value())
        {
            util::panic("Pipeline Cache lookup failed!");
        }

        if (*shouldBePipeline != nullptr
            && *currentlyBoundPipeline != *shouldBePipeline)
        {
            commandBuffer.bindPipeline(bindPoint, ***shouldBePipeline);

            *currentlyBoundPipeline = *shouldBePipeline;
        }

        for (std::size_t i = 0; i < this->sets.size(); ++i)
        {
            vk::DescriptorSet& currentSet =
                currentlyBoundDescriptors[i];               // NOLINT
            vk::DescriptorSet maybeBindSet = this->sets[i]; // NOLINT

            if (currentSet != maybeBindSet)
            {
                commandBuffer.bindDescriptorSets(
                    bindPoint,
                    (*shouldBePipeline)->getLayout(),
                    static_cast<std::uint32_t>(i),
                    {maybeBindSet},
                    {});

                currentSet = maybeBindSet;
            }
        }
    }

    std::strong_ordering Recordable::operator<=> (const Recordable& other) const
    {
        //! bruh!!!
        return std::tie(
                   this->pipeline_handle,
                   this->sets[0],
                   this->sets[1],
                   this->sets[2],
                   this->sets[3])
           <=> std::tie(
                   other.pipeline_handle,
                   other.sets[0],
                   other.sets[1],
                   other.sets[2],
                   other.sets[3]);
    }

    Recordable::operator std::string () const
    {
        return fmt::format(
            "Object {} | ID: {} | Drawing?: {}",
            this->name,
            static_cast<std::string>(this->uuid),
            this->shouldDraw());
    }

    DrawStage Recordable::getDrawStage() const
    {
        return this->stage;
    }

    bool Recordable::shouldDraw() const
    {
        return this->should_draw.load(std::memory_order_acquire);
    }

    util::UUID Recordable::getUUID() const
    {
        return this->uuid;
    }

    vulkan::Allocator& Recordable::getAllocator() const
    {
        return *this->renderer.allocator;
    }

    void Recordable::accessRenderPass(
        DrawStage                                      accessStage,
        std::function<void(const vulkan::RenderPass*)> func) const
    {
        this->renderer.render_passes.readLock(
            [&](const Renderer::RenderPasses& passes)
            {
                std::optional<const vulkan::RenderPass*> maybeRenderPass =
                    passes.acquireRenderPassFromStage(accessStage);

                util::assertFatal(
                    maybeRenderPass.has_value(),
                    "Draw stage {} does not have an associated "
                    "renderpass!",
                    std::to_underlying(accessStage));

                func(*maybeRenderPass);
            });
    }

    void Recordable::registerSelf()
    {
        this->renderer.registerRecordable(this->shared_from_this());
    }

    Recordable::Recordable(
        const gfx::Renderer& renderer_,
        std::string          name_,
        DrawStage            stage_,
        DescriptorRefArray   sets_,
        bool                 shouldDraw)
        : renderer {renderer_}
        , name {std::move(name_)}
        , uuid {util::UUID {}}
        , stage {stage_}
        , should_draw {shouldDraw}
        , sets {sets_}
    {}

} // namespace gfx::recordables
