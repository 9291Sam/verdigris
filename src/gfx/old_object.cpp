#include "object.hpp"
#include "gfx/object.hpp"
#include "gfx/renderer.hpp"
#include "gfx/transform.hpp"
#include "renderer.hpp"
#include "vulkan/allocator.hpp"
#include <atomic>
#include <compare>
#include <engine/settings.hpp>
#include <glm/gtx/string_cast.hpp>
#include <util/log.hpp>

namespace gfx
{

    Object::Object(
        const Renderer& renderer_,
        std::string     name_,
        BindState       requiredBindState,
        Transform       transform_,
        bool            shouldDraw)
        : transform {{std::move(transform_)}} // NOLINT: rvalue is required
        , should_draw {shouldDraw}
        , renderer {renderer_}
        , name {std::move(name_)}
        , id {}
        , bind_state {requiredBindState}
    {}

    std::strong_ordering Object::operator<=> (const Object& other) const
    {
        return this->bind_state <=> other.bind_state;
    }

    Object::operator std::string () const
    {
        return fmt::format(
            "Object {} | ID: {} | Drawing?: {} | Transform: {}",
            this->name,
            static_cast<std::string>(this->id),
            this->shouldDraw(),
            static_cast<std::string>(this->transform.copyInner()));
    }

    util::UUID Object::getUUID() const
    {
        return this->id;
    }

    bool Object::shouldDraw() const
    {
        return this->should_draw.load(std::memory_order_acquire);
    }

    vulkan::Allocator& Object::getRendererAllocator() const
    {
        return *this->renderer.allocator;
    }

    vk::PipelineLayout Object::getCurrentPipelineLayout() const
    {
        return this->renderer.pipelines
            ->getGraphicsPipeline(this->bind_state.pipeline)
            .getLayout();
    }

    void Object::registerSelf() const
    {
        this->renderer.registerObject(this->shared_from_this());
    }

    void gfx::Object::updateBindState(
        vk::CommandBuffer                commandBuffer,
        gfx::BindState&                  workingBindState,
        std::array<vk::DescriptorSet, 4> setsToBindIfRequired) const
    {
        const vulkan::GraphicsPipeline& requiredPipeline =
            this->renderer.pipelines->getGraphicsPipeline(
                this->bind_state.pipeline);

        if (workingBindState.pipeline != this->bind_state.pipeline)
        {
            workingBindState.pipeline = this->bind_state.pipeline;

            commandBuffer.bindPipeline(
                vk::PipelineBindPoint::eGraphics, *requiredPipeline);

            // Binding a new pipeline also resets descriptor sets
            workingBindState.descriptors = {
                ObjectBoundDescriptor {std::nullopt},
                ObjectBoundDescriptor {std::nullopt},
                ObjectBoundDescriptor {std::nullopt},
                ObjectBoundDescriptor {std::nullopt}};
        }

        // NOLINTBEGIN
        for (std::size_t idx = 0; idx < workingBindState.descriptors.size();
             ++idx)
        {
            ObjectBoundDescriptor& workingDescriptor =
                workingBindState.descriptors[idx];
            const ObjectBoundDescriptor& desiredDescriptor =
                this->bind_state.descriptors[idx];

            if (workingDescriptor != desiredDescriptor)
            {
                if (engine::getSettings()
                        .lookupSetting<engine::Setting::EnableAppValidation>())
                {
                    util::assertFatal(
                        setsToBindIfRequired[idx]
                            != vk::DescriptorSet {nullptr},
                        "Bind of nullptr set was requested!");
                }

                commandBuffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    requiredPipeline.getLayout(),
                    static_cast<std::uint32_t>(idx),
                    setsToBindIfRequired[idx],
                    {});

                workingDescriptor = desiredDescriptor;
            }
        }
        // NOLINTEND
    }

} // namespace gfx

std::shared_ptr<gfx::SimpleTriangulatedObject>
gfx::SimpleTriangulatedObject::create(
    const gfx::Renderer&        renderer_,
    std::vector<vulkan::Vertex> vertices,
    std::vector<vulkan::Index>  indices)
{
    std::shared_ptr<gfx::SimpleTriangulatedObject> object {
        new gfx::SimpleTriangulatedObject {
            renderer_, std::move(vertices), std::move(indices)}};

    object->registerSelf();

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
            .pipeline {vulkan::GraphicsPipelineType::Flat},
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
    // do the uploads asychronously and await the futures.
    this->future_vertex_buffer = std::async(
        [vertices   = std::move(vertices_),
         &allocator = this->getRendererAllocator(),
         debugName  = static_cast<std::string>(*this)]
        {
            vulkan::Buffer vertexBuffer {
                &allocator,
                std::span {vertices}.size_bytes(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eDeviceLocal,
                fmt::format("Vertex Buffer | {}", debugName)};

            vertexBuffer.write(std::as_bytes(std::span {vertices}));

            return vertexBuffer;
        });

    this->future_index_buffer = std::async(
        [indices    = std::move(indices_),
         &allocator = this->getRendererAllocator()]
        {
            vulkan::Buffer indexBuffer {
                &allocator,
                std::span {indices}.size_bytes(),
                vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eDeviceLocal};

            indexBuffer.write(std::as_bytes(std::span {indices}));

            return indexBuffer;
        });
}

void gfx::SimpleTriangulatedObject::updateFrameState() const
{
    if (this->future_vertex_buffer.has_value()
        && this->future_vertex_buffer->valid())
    {
        this->vertex_buffer = this->future_vertex_buffer->get();

        this->future_vertex_buffer = std::nullopt;
    }

    if (this->future_index_buffer.has_value()
        && this->future_index_buffer->valid())
    {
        this->index_buffer = this->future_index_buffer->get();

        this->future_index_buffer = std::nullopt;
    }

    if (this->vertex_buffer.has_value() && this->index_buffer.has_value()
        && !this->shouldDraw())
    {
        this->should_draw.store(true, std::memory_order::release);
    }
}

void gfx::SimpleTriangulatedObject::bindAndDraw(
    vk::CommandBuffer commandBuffer,
    BindState&        rendererState,
    const Camera&     camera) const
{
    if (engine::getSettings()
            .lookupSetting<engine::Setting::EnableAppValidation>())
    {
        util::assertFatal(
            this->vertex_buffer.has_value() && this->index_buffer.has_value(),
            "buffers weren't valid when drawing occurred");
    }

    this->updateBindState(
        commandBuffer, rendererState, {nullptr, nullptr, nullptr, nullptr});

    commandBuffer.bindVertexBuffers(0, **this->vertex_buffer, {0}); // NOLINT

    static_assert(sizeof(std::uint32_t) == sizeof(vulkan::Index));
    commandBuffer.bindIndexBuffer(
        **this->index_buffer, 0, vk::IndexType::eUint32); // NOLINT

    vulkan::PushConstants pushConstants {
        .model_view_proj {camera.getPerspectiveMatrix(
            this->renderer, this->transform.copyInner())}};

    commandBuffer.pushConstants<vulkan::PushConstants>(
        this->getCurrentPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        pushConstants);

    commandBuffer.drawIndexed(
        static_cast<std::uint32_t>(this->number_of_indices), 1, 0, 0, 0);
}

std::shared_ptr<gfx::ParallaxRaymarchedVoxelObject>
gfx::ParallaxRaymarchedVoxelObject::create(
    const gfx::Renderer&                renderer_,
    std::vector<vulkan::ParallaxVertex> vertices)
{
    std::shared_ptr<gfx::ParallaxRaymarchedVoxelObject> object {
        new gfx::ParallaxRaymarchedVoxelObject {
            renderer_, std::move(vertices)}};

    object->registerSelf();

    return object;
}

gfx::ParallaxRaymarchedVoxelObject::ParallaxRaymarchedVoxelObject(
    const gfx::Renderer&                renderer_,
    std::vector<vulkan::ParallaxVertex> vertices_)
    : Object(
        renderer_,
        fmt::format(
            "ParallaxRaymarchedVoxelObject | Vertices: {}", vertices_.size()),
        BindState {
            .pipeline {vulkan::GraphicsPipelineType::ParallaxRayMarching},
            .descriptors {
                {ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt},
                 ObjectBoundDescriptor {std::nullopt}}}},
        Transform {},
        false) // parens for clang-format
    , number_of_vertices {vertices_.size()}
    , vertex_buffer {std::nullopt}
{
    // do the uploads asychronously and await the futures.
    this->future_vertex_buffer = std::async(
        [vertices   = std::move(vertices_),
         &allocator = this->getRendererAllocator(),
         debugName  = static_cast<std::string>(*this)]
        {
            vulkan::Buffer vertexBuffer {
                &allocator,
                std::span {vertices}.size_bytes(),
                vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
                    | vk::MemoryPropertyFlagBits::eDeviceLocal,
                fmt::format("Vertex Buffer | {}", debugName)};

            vertexBuffer.write(std::as_bytes(std::span {vertices}));

            return vertexBuffer;
        });
}

void gfx::ParallaxRaymarchedVoxelObject::updateFrameState() const
{
    if (this->future_vertex_buffer.has_value()
        && this->future_vertex_buffer->valid())
    {
        this->vertex_buffer = this->future_vertex_buffer->get();

        this->future_vertex_buffer = std::nullopt;
    }

    if (this->vertex_buffer.has_value() && !this->shouldDraw())
    {
        this->should_draw.store(true, std::memory_order::release);
    }
}

void gfx::ParallaxRaymarchedVoxelObject::bindAndDraw(
    vk::CommandBuffer commandBuffer,
    BindState&        rendererState,
    const Camera&     camera) const
{
    util::logDebug("drawing ParallaxRaymarchedVoxelObject");

    if (engine::getSettings()
            .lookupSetting<engine::Setting::EnableAppValidation>())
    {
        util::assertFatal(
            this->vertex_buffer.has_value(),
            "buffers weren't valid when drawing occurred");
    }

    this->updateBindState(
        commandBuffer, rendererState, {nullptr, nullptr, nullptr, nullptr});

    commandBuffer.bindVertexBuffers(0, **this->vertex_buffer, {0}); // NOLINT

    Transform renderTransform = this->transform.copyInner();

    vulkan::ParallaxPushConstants pushConstants {
        .model_view_proj {camera.getPerspectiveMatrix(
            this->renderer, this->transform.copyInner())},
        .model {renderTransform.asModelMatrix()},
        .camera_pos {camera.getPosition()}};

    util::logDebug("Camera pos: {}", glm::to_string(camera.getPosition()));

    commandBuffer.pushConstants<vulkan::ParallaxPushConstants>(
        this->getCurrentPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        pushConstants);

    commandBuffer.draw(
        static_cast<std::uint32_t>(this->number_of_vertices), 1, 0, 0);
}
