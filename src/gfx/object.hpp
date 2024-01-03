#ifndef SRC_GFX_OBJECT_HPP
#define SRC_GFX_OBJECT_HPP

#include "vulkan/descriptors.hpp"
#include <array>
#include <util/uuid.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Renderer;

    namespace vulkan
    {
        class Allocator;
        class Pipeline;
    } // namespace vulkan

    class Object : public std::enable_shared_from_this<Object>
    {
    public:
        using DescriptorRefArray   = std::array<vk::DescriptorSet, 4>;
        using OwnedDescriptorArray = std::array<vulkan::DescriptorSet, 4>;

        enum class DrawStage
        {
            PrePass,
            RenderPass,
            PostPass
        };
    public:
        virtual ~Object() = default;

        Object(const Object&)             = delete;
        Object(Object&&)                  = delete;
        Object& operator= (const Object&) = delete;
        Object& operator= (Object&&)      = delete;

        /// Lightweight state update function, called on render thread just
        /// before drawing. Do not do heavy work!
        virtual void updateFrameState() const = 0;

        /// Binds given pipeline and descriptors
        void bind(
            vk::CommandBuffer,
            vk::Pipeline&       currentlyBoundPipeline,
            DescriptorRefArray& currentlyBoundDescriptors) const;

        /// Function that actually draws / dispatches the object, place your
        /// vkCmdDraw[s] here
        virtual void
            drawOrDispatch(vk::CommandBuffer, vk::PipelineLayout) const = 0;

        std::strong_ordering     operator<=> (const Object&) const;
        explicit                 operator std::string () const;
        [[nodiscard]] bool       shouldDraw() const;
        [[nodiscard]] util::UUID getUUID() const;

    protected:
        [[nodiscard]] vulkan::Allocator& getAllocator() const;
        void                             registerSelf();

        const Renderer&   renderer;
        const std::string name;
        const util::UUID  uuid;
        const DrawStage   stage;

        /// These are just references to the things that will be bound,
        /// you must ensure they remain alive.
        DescriptorRefArray sets;
        vulkan::Pipeline*  pipeline;

        mutable std::atomic<bool> should_draw;

        Object(
            const gfx::Renderer&,
            std::string name,
            DescriptorRefArray,
            vulkan::Pipeline*,
            bool shouldDraw);
    };

    class ComputeObject : public Object
    {
    public:
        static std::shared_ptr<ComputeObject>
        create(const gfx::Renderer&, std::string name, std::ara)
    };

    class DrawObject
    {
    public:


    protected:
    };
} // namespace gfx

#endif // SRC_GFX_OBJECT_HPP
