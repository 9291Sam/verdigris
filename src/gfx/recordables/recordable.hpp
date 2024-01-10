#ifndef SRC_GFX_RECORDABLE_HPP
#define SRC_GFX_RECORDABLE_HPP

#include <array>
#include <gfx/camera.hpp>
#include <gfx/draw_stages.hpp>
#include <util/uuid.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::recordables
{
    namespace vulkan
    {
        class Allocator;
        class Pipeline;
    } // namespace vulkan

    class Recordable : public std::enable_shared_from_this<Recordable>
    {
    public:
        using DescriptorRefArray = std::array<vk::DescriptorSet, 4>;
    public:
        Recordable()          = delete;
        virtual ~Recordable() = default;

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
            vk::Pipeline&       currentlyBoundPipeline,
            DescriptorRefArray& currentlyBoundDescriptors) const;

        /// Function that actually draws / dispatches the Recordable, place your
        /// vkCmdDraw[s] here
        virtual void record(vk::CommandBuffer, const Camera&) const = 0;

        std::strong_ordering     operator<=> (const Recordable&) const;
        [[nodiscard]] explicit   operator std::string () const;
        [[nodiscard]] DrawStage  getDrawStage() const;
        [[nodiscard]] bool       shouldDraw() const;
        [[nodiscard]] util::UUID getUUID() const;

    protected:
        [[nodiscard]] vulkan::Allocator& getAllocator() const;
        void                             registerSelf();

        const Renderer&   renderer;
        const std::string name;
        const util::UUID  uuid;
        const DrawStage   stage;

        /// You must manage the lifetime on the descriptors, the pipelines are
        /// functionally static
        /// These are the things that will be bound by the time drawOrDispatch
        /// is called
        DescriptorRefArray      sets;
        const vulkan::Pipeline* pipeline;

        mutable std::atomic<bool> should_draw;

        Recordable(
            const gfx::Renderer&,
            std::string name,
            DrawStage   stage,
            bool        shouldDraw  = true,
            const vulkan::Pipeline* = nullptr,
            DescriptorRefArray      = {});
    };
} // namespace gfx::recordables

#endif // SRC_GFX_RECORDABLE_HPP
