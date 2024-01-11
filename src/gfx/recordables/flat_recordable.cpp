#include "flat_recordable.hpp"
#include <atomic>
#include <gfx/vulkan/allocator.hpp>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <util/log.hpp>

namespace gfx::recordables
{

    const vk::VertexInputBindingDescription*
    FlatRecordable::Vertex::getBindingDescription()
    {
        static const vk::VertexInputBindingDescription bindings {
            .binding {0},
            .stride {sizeof(Vertex)},
            .inputRate {vk::VertexInputRate::eVertex},
        };

        return &bindings;
    }

    const std::array<vk::VertexInputAttributeDescription, 3>*
    FlatRecordable::Vertex::getAttributeDescriptions()
    {
        static const std::array<vk::VertexInputAttributeDescription, 3>
            descriptions {
                vk::VertexInputAttributeDescription {
                    .location {0},
                    .binding {0},
                    .format {vk::Format::eR32G32B32A32Sfloat},
                    .offset {offsetof(Vertex, color)},
                },
                vk::VertexInputAttributeDescription {
                    .location {1},
                    .binding {0},
                    .format {vk::Format::eR32G32B32Sfloat},
                    .offset {offsetof(Vertex, position)},
                },
                vk::VertexInputAttributeDescription {
                    .location {2},
                    .binding {0},
                    .format {vk::Format::eR32G32B32Sfloat},
                    .offset {offsetof(Vertex, normal)},
                }};

        return &descriptions;
    }

    std::shared_ptr<FlatRecordable> FlatRecordable::create(
        const gfx::Renderer& renderer_,
        std::vector<Vertex>  vertices,
        std::vector<Index>   indicies,
        Transform            transform_,
        std::string          name_)
    {
        std::shared_ptr<FlatRecordable> recordable {new FlatRecordable {
            renderer_,
            std::move(vertices),
            std::move(indicies),
            transform_,
            std::move(name_)}};

        recordable->registerSelf();

        return recordable;
    }

    void FlatRecordable::updateFrameState() const
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

    void FlatRecordable::record(
        vk::CommandBuffer commandBuffer, const Camera& camera) const
    {
        util::assertFatal(
            this->vertex_buffer.has_value() && this->index_buffer.has_value(),
            "buffers weren't valid when drawing occurred");

        commandBuffer.bindVertexBuffers(
            0, **this->vertex_buffer, {0}); // NOLINT

        commandBuffer.bindIndexBuffer(
            **this->index_buffer, 0, vk::IndexType::eUint32); // NOLINT

        PushConstants pushConstants {
            .model_view_proj {camera.getPerspectiveMatrix(
                this->renderer, this->transform.copyInner())}};

        commandBuffer.pushConstants<PushConstants>(
            this->pipeline->getLayout(),
            vk::ShaderStageFlagBits::eVertex,
            0,
            pushConstants);

        commandBuffer.drawIndexed(
            static_cast<std::uint32_t>(this->number_of_indices), 1, 0, 0, 0);
    }

    const vulkan::Pipeline* FlatRecordable::getPipeline() const
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
        static util::Mutex<vulkan::PipelineCache::PipelineHandle>
            maybeHandleMutex {}; // NOLINT
#pragma clang diagnostic pop

        vulkan::PipelineCache::PipelineHandle handle = maybeHandleMutex.lock(
            [&](vulkan::PipelineCache::PipelineHandle& maybeHandle)
            {
                if (!maybeHandle.isValid())
                {
                    return maybeHandle;
                }

                this->accessRenderPass(
                    this->stage,
                    [&](const vulkan::RenderPass* renderPass)
                    {
                        std::vector<vk::UniqueShaderModule> uniqueShaders {};

                        std::vector<std::pair<
                            vk::ShaderStageFlagBits,
                            vk::ShaderModule>>
                            pipelineShaders {};

                        {
                            vk::UniqueShaderModule fragmentShader =
                                vulkan::createShaderFromFile(
                                    this->getAllocator()
                                        .getOwningDevice()
                                        ->asLogicalDevice(),
                                    "src/gfx/vulkan/shaders/"
                                    "flat_pipeline.frag.bin");

                            pipelineShaders.push_back(
                                {vk::ShaderStageFlagBits::eFragment,
                                 *fragmentShader});
                        }

                        std::unique_ptr<vulkan::Pipeline> newPipeline {
                            new vulkan::GraphicsPipeline {
                                this->renderer,
                                renderPass,
                                pipelineShaders,
                                vk::PipelineVertexInputStateCreateInfo {
                                    .sType {
                                        vk::StructureType::
                                            ePipelineVertexInputStateCreateInfo},
                                    .pNext {nullptr},
                                    .flags {},
                                    .vertexBindingDescriptionCount {1},
                                    .pVertexBindingDescriptions {
                                        Vertex::getBindingDescription()},
                                    .vertexAttributeDescriptionCount {
                                        static_cast<std::uint32_t>(
                                            Vertex::getAttributeDescriptions()
                                                ->size())},
                                    .pVertexAttributeDescriptions {
                                        Vertex::getAttributeDescriptions()
                                            ->data()},
                                },
                                vk::PrimitiveTopology::eTriangleList,
                                {},
                                std::array {vk::PushConstantRange {
                                    .stageFlags {
                                        vk::ShaderStageFlagBits::eVertex},
                                    .offset {0},
                                    .size {sizeof(PushConstants)},
                                }},
                                "FlatRecordable"}};

                        return this->getPipelineCache().cachePipeline(
                            std::move(newPipeline));
                    });

                return maybeHandle;
            });

        return this->getPipelineCache().lookupPipeline(handle);
    }

    FlatRecordable::FlatRecordable(
        const gfx::Renderer& renderer_,
        std::vector<Vertex>  vertices,
        std::vector<Index>   indicies,
        Transform            transform_,
        std::string          name_)
        : Recordable {
            renderer_,
            std::format(
                "FlatRecordable | {} | Vertices: {} | Indicies {} ",
                name_,
                vertices.size(),
                indicies.size()), 
            DrawStage::DisplayPass,
            nullptr,
            vk::PipelineBindPoint::eGraphics}
        , transform {std::move(transform_)} // NOLINT: rvalue is needed
        , number_of_vertices {vertices.size()}
        , number_of_indices {indicies.size()}
    {
        this->pipeline = this->getPipeline();

        this->future_vertex_buffer = std::async(
            [lambdaVertices = std::move(vertices),
             &allocator     = this->getAllocator(),
             debugName      = static_cast<std::string>(*this)]
            {
                vulkan::Buffer vertexBuffer {
                    &allocator,
                    std::span {lambdaVertices}.size_bytes(),
                    vk::BufferUsageFlagBits::eVertexBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible
                        | vk::MemoryPropertyFlagBits::eDeviceLocal,
                    fmt::format("Vertex Buffer | {}", debugName)};

                vertexBuffer.write(std::as_bytes(std::span {lambdaVertices}));

                return vertexBuffer;
            });

        this->future_index_buffer = std::async(
            [lambdaIndicies = std::move(indicies),
             &allocator     = this->getAllocator(),
             debugName      = static_cast<std::string>(*this)]
            {
                vulkan::Buffer indexBuffer {
                    &allocator,
                    std::span {lambdaIndicies}.size_bytes(),
                    vk::BufferUsageFlagBits::eIndexBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible
                        | vk::MemoryPropertyFlagBits::eDeviceLocal,
                    fmt::format("INdex Buffer | {}", debugName)};

                indexBuffer.write(std::as_bytes(std::span {lambdaIndicies}));

                return indexBuffer;
            });
    }

} // namespace gfx::recordables
