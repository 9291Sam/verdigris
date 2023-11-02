#ifndef SRC_GFX_OBJECT_HPP
#define SRC_GFX_OBJECT_HPP

#include "camera.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/gpu_structures.hpp"
#include "vulkan/pipelines.hpp"
#include <future>
#include <memory>
#include <optional>
#include <util/threads.hpp>
#include <util/uuid.hpp>

// TODO: can this be re-done so its not a steaming pile of shit?
namespace gfx
{
    class Renderer;

    namespace vulkan
    {
        class Allocator;
    } // namespace vulkan

    struct ObjectBoundDescriptor
    {
        std::optional<util::UUID> id;

        [[nodiscard]] bool
        operator== (const ObjectBoundDescriptor&) const = default;
        [[nodiscard]] std::strong_ordering
        operator<=> (const ObjectBoundDescriptor&) const = default;
    };

    struct BindState
    {
        // TODO: replace with std::variant
        vulkan::GraphicsPipelineType pipeline =
            vulkan::GraphicsPipelineType::NoPipeline;
        std::array<ObjectBoundDescriptor, 4> descriptors;

        [[nodiscard]] std::strong_ordering
        operator<=> (const BindState&) const = default;
    };

    class Object : public std::enable_shared_from_this<Object>
    {
    public:
        virtual ~Object() = default;

        Object(const Object&)             = delete;
        Object(Object&&)                  = delete;
        Object& operator= (const Object&) = delete;
        Object& operator= (Object&&)      = delete;

        /// lightweight state update function, called on render thread
        virtual void updateFrameState() const = 0;

        /// Handles all of the actions required to execute the draw call
        virtual void
        bindAndDraw(vk::CommandBuffer, BindState&, const Camera&) const = 0;

        // Misc. helper functions
        std::strong_ordering operator<=> (const Object&) const;
        explicit virtual     operator std::string () const;
        bool                 shouldDraw() const;
        util::UUID           getUUID() const;

        util::Mutex<Transform>    transform;
        mutable std::atomic<bool> should_draw;

    protected:
        // Some renderer internals need to be exposed so they're boxed in by
        // these functions rather than making them public
        [[nodiscard]] vulkan::Allocator& getRendererAllocator() const;
        [[nodiscard]] vk::PipelineLayout getCurrentPipelineLayout() const;
        void                             registerSelf() const;

        void updateBindState(
            vk::CommandBuffer,
            BindState&,
            std::array<vk::DescriptorSet, 4> setsToBindIfRequired) const;

        const gfx::Renderer& renderer;
        const std::string    name;
        const util::UUID     id;
        const BindState      bind_state;

        Object(
            const gfx::Renderer&,
            std::string name,
            BindState   requiredBindState,
            Transform   transform,
            bool        shouldDraw);
    };

    class SimpleTriangulatedObject final : public Object
    {
    public:
        static std::shared_ptr<SimpleTriangulatedObject> create(
            const gfx::Renderer&,
            std::vector<vulkan::Vertex>,
            std::vector<vulkan::Index>);
        ~SimpleTriangulatedObject() override = default;

        void updateFrameState() const override;
        void bindAndDraw(
            vk::CommandBuffer, BindState&, const Camera&) const override;

    private:
        mutable std::optional<std::future<vulkan::Buffer>> future_vertex_buffer;
        std::size_t                                        number_of_vertices;
        mutable std::optional<vulkan::Buffer>              vertex_buffer;

        mutable std::optional<std::future<vulkan::Buffer>> future_index_buffer;
        std::size_t                                        number_of_indices;
        mutable std::optional<vulkan::Buffer>              index_buffer;

        SimpleTriangulatedObject(
            const gfx::Renderer&,
            std::vector<vulkan::Vertex>,
            std::vector<vulkan::Index>);
    };

} // namespace gfx

#endif // SRC_GFX_OBJECT_HPP