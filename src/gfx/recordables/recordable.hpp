#ifndef SRC_GFX_RECORDABLES_RECORDABLE_HPP
#define SRC_GFX_RECORDABLES_RECORDABLE_HPP

#include <array>
#include <functional>
#include <gfx/camera.hpp>
#include <gfx/draw_stages.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <util/uuid.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Allocator;
    class Pipeline;
    class PipelineCache;
    class RenderPass;
} // namespace gfx::vulkan

namespace gfx::recordables
{

    class Recordable : public std::enable_shared_from_this<Recordable>
    {
    public:
        using DescriptorRefArray = std::array<vk::DescriptorSet, 4>;
    public:
        Recordable() = delete;
        virtual ~Recordable();

        Recordable(const Recordable&)             = delete;
        Recordable(Recordable&&)                  = delete;
        Recordable& operator= (const Recordable&) = delete;
        Recordable& operator= (Recordable&&)      = delete;

        /// Lightweight state update function, called on render thread just
        /// before drawing. Do not do heavy work!
        virtual void updateFrameState() const = 0;

        /// Binds given pipeline and descriptors
        void bind(
            vk::CommandBuffer,
            const vulkan::PipelineCache&,
            const vulkan::Pipeline** currentlyBoundPipeline,
            DescriptorRefArray&      currentlyBoundDescriptors) const;

        /// Function that actually draws / dispatches the Recordable, place your
        /// vkCmdDraw[s] here
        virtual void
        record(vk::CommandBuffer, vk::PipelineLayout, const Camera&) const = 0;

        std::strong_ordering     operator<=> (const Recordable&) const;
        [[nodiscard]] explicit   operator std::string () const;
        [[nodiscard]] DrawStage  getDrawStage() const;
        [[nodiscard]] bool       shouldDraw() const;
        [[nodiscard]] util::UUID getUUID() const;

    protected:
        // TODO: combine allocator into master class of memory, descriptor,
        // pipeline, and renderpass allocation
        [[nodiscard]] vulkan::Allocator& getAllocator() const;

        virtual std::
            pair<vulkan::PipelineCache::PipelineHandle, vk::PipelineBindPoint>
            getPipeline(const vulkan::PipelineCache&) const = 0;

        void accessRenderPass(
            DrawStage                                      accessStage,
            std::function<void(const vulkan::RenderPass*)> func) const;

        void registerSelf();

        const Renderer&           renderer;
        const std::string         name;
        const util::UUID          uuid;
        const DrawStage           stage;
        mutable std::atomic<bool> should_draw;

        /// You must manage the lifetime on the descriptors
        Recordable(
            const gfx::Renderer&,
            std::string name,
            DrawStage   stage,
            DescriptorRefArray = {},
            bool shouldDraw    = true);

    private:
        DescriptorRefArray                    sets;
        vulkan::PipelineCache::PipelineHandle pipeline_handle;
    };
} // namespace gfx::recordables

#endif // SRC_GFX_RECORDABLES_RECORDABLE_HPP
