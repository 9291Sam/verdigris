#ifndef SRC_GFX_OBJECT_HPP
#define SRC_GFX_OBJECT_HPP

#include "camera.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/gpu_structures.hpp"
#include "vulkan/pipelines.hpp"
#include <memory>
#include <optional>
#include <util/threads.hpp>
#include <util/uuid.hpp>

namespace gfx
{
    class Renderer;

    namespace vulkan
    {
        class Allocator;
    }

    struct ObjectBoundDescriptor
    {
        std::optional<util::UUID> id;

        [[nodiscard]] bool operator== (const ObjectBoundDescriptor&) const;
        [[nodiscard]] std::strong_ordering
        operator<=> (const ObjectBoundDescriptor&) const;
    };

    struct BindState // NOLINT: I want designated initialization
    {
        vulkan::PipelineType                 pipeline;
        std::array<ObjectBoundDescriptor, 4> descriptors;

        [[nodiscard]] std::strong_ordering operator<=> (const BindState&) const;
    };

    class Object : public std::enable_shared_from_this<Object>
    {
    public:

        static std::shared_ptr<Object> create(
            const gfx::Renderer&,
            std::string name,
            BindState   requiredBindState,
            Transform,
            bool shouldDraw);
        virtual ~Object();

        Object(const Object&)             = delete;
        Object(Object&&)                  = delete;
        Object& operator= (const Object&) = delete;
        Object& operator= (Object&&)      = delete;

        // Called every renderer frame
        // useful for checking on asynchronous workloads
        // Ex. check if buffers are uploaded
        virtual void updateFrameState() const;

        // Handles all of the actions required to execute the draw call
        // Descriptors, pipelines, push constants, and the draw call
        virtual void
        bindAndDraw(vk::CommandBuffer, BindState&, const Camera&) const;

        std::strong_ordering operator<=> (const Object&) const;
        explicit virtual     operator std::string () const;
        util::UUID           getUUID() const;
        bool                 shouldDraw() const;

        util::Mutex<Transform>    transform;
        mutable std::atomic<bool> should_draw;

    protected:
        // Some renderer internals need to be exposed
        [[nodiscard]] std::shared_ptr<vulkan::Allocator>
                                         getRendererAllocator() const;
        [[nodiscard]] vk::PipelineLayout getCurrentPipelineLayout() const;
        void                             addSelfToRenderList();

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

        SimpleTriangulatedObject(const SimpleTriangulatedObject&) = delete;
        SimpleTriangulatedObject(SimpleTriangulatedObject&&)      = delete;
        SimpleTriangulatedObject&
        operator= (const SimpleTriangulatedObject&) = delete;
        SimpleTriangulatedObject&
        operator= (SimpleTriangulatedObject&&) = delete;

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