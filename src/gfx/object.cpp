#include "object.hpp"
#include "gfx/object.hpp"
#include "gfx/renderer.hpp"
#include "gfx/transform.hpp"
#include "gfx/vulkan/gpu_data.hpp"
#include "renderer.hpp"
#include "util/log.hpp"
#include "util/threads.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <atomic>
#include <compare>
#include <stdexcept>

bool gfx::ObjectBoundDescriptor::operator== (
    const gfx::ObjectBoundDescriptor& other) const
{
    return this->id == other.id;
}
std::strong_ordering gfx::ObjectBoundDescriptor::operator<=> (
    const gfx::ObjectBoundDescriptor& other) const
{
    if (this->id.has_value() && other.id.has_value())
    {
        return (*this->id) <=> (*other.id);
    }
    else
    {
        return std::strong_ordering::equal;
    }
}

std::strong_ordering gfx::BindState::operator<=> (const BindState& other) const
{
    // TODO: replace with std::tuple::operator <=>
#define EMIT_ORDERING(field)                                                   \
    if (this->field <=> other.field != std::strong_ordering::equivalent)       \
    {                                                                          \
        return this->field <=> other.field;                                    \
    }

    EMIT_ORDERING(pipeline)       // NOLINT: shut the fuck up
    EMIT_ORDERING(descriptors[0]) // NOLINT: shut the fuck up
    EMIT_ORDERING(descriptors[1]) // NOLINT: shut the fuck up
    EMIT_ORDERING(descriptors[2]) // NOLINT: shut the fuck up
    EMIT_ORDERING(descriptors[3]) // NOLINT: shut the fuck up

    return std::strong_ordering::equivalent;
#undef EMIT_ORDERING
}

gfx::Object::Object(
    const gfx::Renderer& renderer_,
    std::string          name_,
    BindState            requiredBindState,
    Transform            transform_,
    bool                 shouldDraw)
    : renderer {renderer_}
    , name {std::move(name_)}
    , id {}
    , bind_state {requiredBindState}
    , should_draw {shouldDraw}
    , transform {transform_}
{
    // util::logTrace("Constructed Object | {}",
    // static_cast<std::string>(*this));
}

std::shared_ptr<gfx::Object> gfx::Object::create(
    const gfx::Renderer& renderer,
    std::string          name,
    BindState            requiredBindState,
    Transform            transform,
    bool                 shouldDraw)
{
    std::shared_ptr<gfx::Object> object {new gfx::Object(
        renderer, std::move(name), requiredBindState, transform, shouldDraw)};

    return object;
}

gfx::Object::~Object()
{
    // util::logTrace("~Object {}", static_cast<std::string>(*this));
    renderer.removeObject(this->id);
}

void gfx::Object::updateFrameState() const
{
    throw std::logic_error {
        "gfx::Object::updateFrameState must be overridden!"};
}

void gfx::Object::bindAndDraw(
    vk::CommandBuffer, BindState&, const Camera&) const
{
    throw std::logic_error {"gfx::Object::bindAndDraw must be overridden!"};
}

std::strong_ordering gfx::Object::operator<=> (const Object& other) const
{
    return this->bind_state <=> other.bind_state;
}

gfx::Object::operator std::string () const
{
    return fmt::format(
        "Object {} | ID: {} | Drawing?: {} | Transform: {}",
        this->name,
        static_cast<std::string>(this->id),
        this->shouldDraw(),
        static_cast<std::string>(this->transform.copy_inner()));
}

util::UUID gfx::Object::getUUID() const
{
    return this->id;
}

bool gfx::Object::shouldDraw() const
{
    return this->should_draw.load(std::memory_order_acquire);
}

std::shared_ptr<gfx::vulkan::Allocator>
gfx::Object::getRendererAllocator() const
{
    return this->renderer.allocator;
}

vk::PipelineLayout gfx::Object::getCurrentPipelineLayout() const
{
    return this->renderer.pipeline_map.at(this->bind_state.pipeline)
        .getLayout();
}

void gfx::Object::addSelfToRenderList()
{
    this->renderer.addObject(this->shared_from_this());
}

void gfx::Object::updateBindState(
    vk::CommandBuffer                commandBuffer,
    gfx::BindState&                  bindState,
    std::array<vk::DescriptorSet, 4> setsToBindIfRequired) const
{
    const vulkan::Pipeline& requiredPipeline =
        this->renderer.pipeline_map.at(this->bind_state.pipeline);

    if (bindState.pipeline != this->bind_state.pipeline)
    {
        bindState.pipeline = this->bind_state.pipeline;

        commandBuffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, *requiredPipeline);

        // Binding a new pipeline also resets descriptor sets
        bindState.descriptors = {
            std::nullopt, std::nullopt, std::nullopt, std::nullopt};
    }

    for (std::size_t idx = 0; idx < bindState.descriptors.size(); ++idx)
    {
        // TODO: optimization for the currently bound but nullopt case, will
        // be important later
        if (bindState.descriptors[idx] != this->bind_state.descriptors[idx])
        {
            bindState.descriptors[idx] = this->bind_state.descriptors[idx];

            util::assertFatal(
                setsToBindIfRequired[idx] != vk::DescriptorSet {nullptr},
                "Bind of nullptr set was requested!");

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                requiredPipeline.getLayout(),
                static_cast<std::uint32_t>(idx),
                setsToBindIfRequired[idx],
                {});
        }
    }
}

std::shared_ptr<gfx::SimpleTriangulatedObject>
gfx::SimpleTriangulatedObject::create(
    const gfx::Renderer&        renderer,
    std::vector<vulkan::Vertex> vertices,
    std::vector<vulkan::Index>  indices)
{
    std::shared_ptr<gfx::SimpleTriangulatedObject> object {
        new gfx::SimpleTriangulatedObject {
            renderer, std::move(vertices), std::move(indices)}};

    object->addSelfToRenderList();

    return object;
}

gfx::SimpleTriangulatedObject::SimpleTriangulatedObject(
    const gfx::Renderer&        renderer_,
    std::vector<vulkan::Vertex> vertices_,
    std::vector<vulkan::Index>  indices_)
    : Object(
        renderer_,
        fmt::format(
            "SimpleTriangulatedObject | Vertices: {} | Indices: {}",
            vertices_.size(),
            indices_.size()),
        BindState {
            .pipeline {vulkan::PipelineType::Flat},
            .descriptors {
                {ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt}}}},
        Transform {},
        false) // parens for clang-format
    , number_of_vertices {vertices_.size()}
    , vertex_buffer {std::nullopt}
    , number_of_indices {indices_.size()}
    , index_buffer {std::nullopt}
{
    // this->vertex_buffer = vulkan::Buffer {
    //     this->getRendererAllocator(),
    //     std::span {vertices_}.size_bytes(),
    //     vk::BufferUsageFlagBits::eVertexBuffer,
    //     vk::MemoryPropertyFlagBits::eHostVisible
    //         | vk::MemoryPropertyFlagBits::eDeviceLocal};
    // this->vertex_buffer->write(std::as_bytes(std::span {vertices_}));

    // this->index_buffer = vulkan::Buffer {
    //     this->getRendererAllocator(),
    //     std::span {indices_}.size_bytes(),
    //     vk::BufferUsageFlagBits::eIndexBuffer,
    //     vk::MemoryPropertyFlagBits::eHostVisible
    //         | vk::MemoryPropertyFlagBits::eDeviceLocal};
    // this->index_buffer->write(std::as_bytes(std::span {indices_}));

    // do the uploads asychronously and await the futures.
    this->future_vertex_buffer = util::runAsynchronously<vulkan::Buffer>(
        [vertices  = std::move(vertices_),
         allocator = this->getRendererAllocator()]
        {
            vulkan::Buffer vertexBuffer {
                std::move(allocator),
                std::span {vertices}.size_bytes(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eDeviceLocal};

            vertexBuffer.write(std::as_bytes(std::span {vertices}));

            return vertexBuffer;
        });

    this->future_index_buffer = util::runAsynchronously<vulkan::Buffer>(
        [indices   = std::move(indices_),
         allocator = this->getRendererAllocator()]
        {
            vulkan::Buffer indexBuffer {
                std::move(allocator),
                std::span {indices}.size_bytes(),
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eDeviceLocal};

            indexBuffer.write(std::as_bytes(std::span {indices}));

            return indexBuffer;
        });
    // this->should_draw.store(true, std::memory_order_release);
}

void gfx::SimpleTriangulatedObject::updateFrameState() const
{
    if (this->future_vertex_buffer.has_value())
    {
        if (std::optional<vulkan::Buffer> resolvedFuture =
                this->future_vertex_buffer->tryAwait())
        {
            this->vertex_buffer = std::move(*resolvedFuture);

            this->future_vertex_buffer = std::nullopt;

            // util::logTrace("Resolved vertex buffer");
        }
    }

    if (this->future_index_buffer.has_value())
    {
        if (std::optional<vulkan::Buffer> resolvedFuture =
                this->future_index_buffer->tryAwait())
        {
            this->index_buffer = std::move(*resolvedFuture);

            this->future_index_buffer = std::nullopt;

            // util::logTrace("Resolved index buffer");
        }
    }

    if (this->vertex_buffer.has_value() && this->index_buffer.has_value()
        && !this->shouldDraw())
    {
        this->should_draw.store(true, std::memory_order::release);
        // util::logTrace(
        //     "Object now drawable! {}", static_cast<std::string>(*this));
    }
}

void gfx::SimpleTriangulatedObject::bindAndDraw(
    vk::CommandBuffer commandBuffer,
    BindState&        rendererState,
    const Camera&     camera) const
{
    if (MANGO_ENABLE_VALIDATION)
    {
        util::assertFatal(
            this->vertex_buffer.has_value() && this->index_buffer.has_value(),
            "buffers werent valid when drawing occurred");
    }

    this->updateBindState(
        commandBuffer, rendererState, {nullptr, nullptr, nullptr, nullptr});

    commandBuffer.bindVertexBuffers(0, **this->vertex_buffer, {0});

    static_assert(sizeof(std::uint32_t) == sizeof(vulkan::Index));
    commandBuffer.bindIndexBuffer(
        **this->index_buffer, 0, vk::IndexType::eUint32);

    vulkan::PushConstants pushConstants {
        .model_view_proj {camera.getPerspectiveMatrix(
            this->renderer, this->transform.copy_inner())}};

    commandBuffer.pushConstants<vulkan::PushConstants>(
        this->getCurrentPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        pushConstants);

    commandBuffer.drawIndexed(
        static_cast<std::uint32_t>(this->number_of_indices), 1, 0, 0, 0);
}
