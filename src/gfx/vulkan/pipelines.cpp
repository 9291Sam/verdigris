#include "pipelines.hpp"
#include "allocator.hpp"
#include "descriptors.hpp"
#include "gpu_structures.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"
#include "util/log.hpp"
#include <fstream>

namespace
{
    vk::UniqueShaderModule
    createShaderFromFile(vk::Device device, const char* filePath)
    {
        std::ifstream fileStream {
            filePath,
            // NOLINTNEXTLINE
            std::ios::in | std::ios::binary | std::ios::ate};

        util::assertFatal(
            fileStream.is_open(), "Failed to open file |{}|", filePath);

        std::vector<char> charBuffer {};
        charBuffer.resize(static_cast<std::size_t>(fileStream.tellg()));

        fileStream.seekg(0);
        fileStream.read(
            charBuffer.data(), static_cast<std::streamsize>(charBuffer.size()));

        // intellectual exercise :) ((actually required, this UB as a
        // problem...))
        std::vector<std::uint32_t> u32Buffer {};
        u32Buffer.resize((charBuffer.size() / 4) + 1);

        std::memcpy(u32Buffer.data(), charBuffer.data(), charBuffer.size());

        vk::ShaderModuleCreateInfo shaderCreateInfo {
            .sType {vk::StructureType::eShaderModuleCreateInfo},
            .pNext {nullptr},
            .flags {},
            .codeSize {charBuffer.size()}, // lol
            .pCode {u32Buffer.data()},
        };

        return device.createShaderModuleUnique(shaderCreateInfo);
    }
} // namespace

namespace gfx::vulkan
{
    PipelineManager::PipelineManager(
        vk::Device     device_,
        vk::RenderPass renderPass,
        Swapchain*     swapchain_,
        Allocator*     allocator_)
        : device {device_}
        , render_pass {renderPass}
        , swapchain {swapchain_}
        , allocator {allocator_}
        , graphics_pipeline_cache {{}}
        , compute_pipeline_cache {{}}
    {}

    const GraphicsPipeline& PipelineManager::getGraphicsPipeline(
        GraphicsPipelineType pipelineToGet) const
    {
        const GraphicsPipeline* maybePipeline = nullptr;

        this->graphics_pipeline_cache.readLock(
            [pipelineToGet, &maybePipeline](const std::unordered_map<
                                            GraphicsPipelineType,
                                            GraphicsPipeline>& cache)
            {
                if (cache.contains(pipelineToGet))
                {
                    maybePipeline = &cache.at(pipelineToGet);
                }
            });

        if (maybePipeline != nullptr)
        {
            return *maybePipeline;
        }
        else // we probablly to create the pipeline
        {
            this->graphics_pipeline_cache.writeLock(
                [pipelineToGet, &maybePipeline, this](
                    std::unordered_map<GraphicsPipelineType, GraphicsPipeline>&
                        cache)
                {
                    // Double check someone else hasn't already created the
                    // pipeline
                    if (cache.contains(pipelineToGet))
                    {
                        maybePipeline = &cache.at(pipelineToGet);
                        return;
                    }

                    cache[pipelineToGet] =
                        this->createGraphicsPipeline(pipelineToGet);

                    maybePipeline = &cache[pipelineToGet];
                });
        }

        return *maybePipeline;
    }

    const ComputePipeline&
    PipelineManager::getComputePipeline(ComputePipelineType pipelineToGet) const
    {
        const ComputePipeline* maybePipeline = nullptr;

        this->compute_pipeline_cache.readLock(
            [pipelineToGet, &maybePipeline](
                const std::unordered_map<ComputePipelineType, ComputePipeline>&
                    cache)
            {
                if (cache.contains(pipelineToGet))
                {
                    maybePipeline = &cache.at(pipelineToGet);
                }
            });

        if (maybePipeline != nullptr)
        {
            return *maybePipeline;
        }
        else // we probablly to create the pipeline
        {
            this->compute_pipeline_cache.writeLock(
                [pipelineToGet, &maybePipeline, this](
                    std::unordered_map<ComputePipelineType, ComputePipeline>&
                        cache)
                {
                    // Double check someone else hasn't already created the
                    // pipeline
                    if (cache.contains(pipelineToGet))
                    {
                        maybePipeline = &cache.at(pipelineToGet);
                        return;
                    }

                    cache[pipelineToGet] =
                        this->createComputePipeline(pipelineToGet);

                    maybePipeline = &cache[pipelineToGet];
                });
        }

        return *maybePipeline;
    }

    GraphicsPipeline PipelineManager::createGraphicsPipeline(
        GraphicsPipelineType typeToCreate) const
    {
        switch (typeToCreate)
        {
        case gfx::vulkan::GraphicsPipelineType::NoPipeline:
            util::panic("Tried to create a null pipeline!");
            break;

        case gfx::vulkan::GraphicsPipelineType::Flat: {
            // TODO: replace with #embed
            vk::UniqueShaderModule fragmentShader = createShaderFromFile(
                device, "src/gfx/vulkan/shaders/flat_pipeline.frag.bin");

            vk::UniqueShaderModule vertexShader = createShaderFromFile(
                device, "src/gfx/vulkan/shaders/flat_pipeline.vert.bin");

            std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages {
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eVertex},
                    .module {*vertexShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eFragment},
                    .module {*fragmentShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
            };

            return GraphicsPipeline {
                GraphicsPipeline::VertexType::Normal,
                this->device,
                this->render_pass,
                *this->swapchain,
                shaderStages,
                vk::PrimitiveTopology::eTriangleList,
                [&] // -> vk::UniquePipelineLayout
                {
                    const vk::PushConstantRange pushConstantsInformation {
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .offset {0},
                        .size {sizeof(vulkan::PushConstants)},
                    };

                    return this->device.createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo {
                            .sType {
                                vk::StructureType::ePipelineLayoutCreateInfo},
                            .pNext {nullptr},
                            .flags {},
                            .setLayoutCount {0},
                            .pSetLayouts {nullptr},
                            .pushConstantRangeCount {1},
                            .pPushConstantRanges {&pushConstantsInformation},
                        });
                }()};
        }

        case gfx::vulkan::GraphicsPipelineType::ParallaxRayMarching: {
            vk::UniqueShaderModule fragmentShader = createShaderFromFile(
                device, "src/gfx/vulkan/shaders/parallax_raymarching.frag.bin");

            vk::UniqueShaderModule vertexShader = createShaderFromFile(
                device, "src/gfx/vulkan/shaders/parallax_raymarching.vert.bin");

            std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages {
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eVertex},
                    .module {*vertexShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eFragment},
                    .module {*fragmentShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
            };

            return GraphicsPipeline {
                GraphicsPipeline::VertexType::Parallax,
                this->device,
                this->render_pass,
                *this->swapchain,
                shaderStages,
                vk::PrimitiveTopology::eTriangleList,
                [&] // -> vk::UniquePipelineLayout
                {
                    const vk::PushConstantRange pushConstantsInformation {
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .offset {0},
                        .size {sizeof(vulkan::ParallaxPushConstants)},
                    };

                    return this->device.createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo {
                            .sType {
                                vk::StructureType::ePipelineLayoutCreateInfo},
                            .pNext {nullptr},
                            .flags {},
                            .setLayoutCount {1},
                            .pSetLayouts {
                                *this->allocator->getDescriptorSetLayout(
                                    DescriptorSetType::VoxelRayTracing)},
                            .pushConstantRangeCount {1},
                            .pPushConstantRanges {&pushConstantsInformation},
                        });
                }()};
        }

        case GraphicsPipelineType::Voxel: {
            vk::UniqueShaderModule fragmentShader = createShaderFromFile(
                this->device, "src/gfx/vulkan/shaders/voxel.frag.bin");

            vk::UniqueShaderModule vertexShader = createShaderFromFile(
                this->device, "src/gfx/vulkan/shaders/voxel.vert.bin");

            std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages {
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eVertex},
                    .module {*vertexShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
                vk::PipelineShaderStageCreateInfo {
                    .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .stage {vk::ShaderStageFlagBits::eFragment},
                    .module {*fragmentShader},
                    .pName {"main"},
                    .pSpecializationInfo {},
                },
            };

            return GraphicsPipeline {
                GraphicsPipeline::VertexType::None,
                this->device,
                this->render_pass,
                *this->swapchain,
                shaderStages,
                vk::PrimitiveTopology::eTriangleStrip,
                [&] // -> vk::UniquePipelineLayout
                {
                    const vk::PushConstantRange pushConstantsInformation {
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .offset {0},
                        .size {sizeof(vulkan::PushConstants)},
                    };

                    return this->device.createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo {
                            .sType {
                                vk::StructureType::ePipelineLayoutCreateInfo},
                            .pNext {nullptr},
                            .flags {},
                            .setLayoutCount {1},
                            .pSetLayouts {
                                *this->allocator->getDescriptorSetLayout(
                                    DescriptorSetType::Voxel)},
                            .pushConstantRangeCount {1},
                            .pPushConstantRanges {&pushConstantsInformation},
                        });
                }()};
        }
        }
        util::panic(
            "Unimplemented pipeline creation! {}",
            std::to_underlying(typeToCreate));
        util::debugBreak();
    }

    ComputePipeline PipelineManager::createComputePipeline(
        ComputePipelineType typeToCreate) const
    {
        switch (typeToCreate)
        {
        case ComputePipelineType::NoPipeline:
            util::panic("Tried to create null compute pipeline");
            return {};
        case ComputePipelineType::RayCaster: {
            vk::UniqueShaderModule shader = createShaderFromFile(
                this->device,
                "src/gfx/vulkan/shaders/voxel/uber_voxel.comp.bin");

            std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts {
                **this->allocator->getDescriptorSetLayout(
                    DescriptorSetType::VoxelRayTracing)};

            return ComputePipeline {
                this->device, std::move(shader), descriptorSetLayouts};
        }
        }
    }

    GraphicsPipeline::GraphicsPipeline(
        VertexType                                   vertexType,
        vk::Device                                   device,
        vk::RenderPass                               renderPass,
        const Swapchain&                             swapchain,
        std::span<vk::PipelineShaderStageCreateInfo> shaderStages,
        vk::PrimitiveTopology                        topology,
        vk::UniquePipelineLayout                     layout_)
        : GraphicsPipeline {}
    {
        const vk::Viewport viewport {
            .x {0.0f},
            .y {0.0f},
            .width {static_cast<float>(swapchain.getExtent().width)},
            .height {static_cast<float>(swapchain.getExtent().height)},
            .minDepth {0.0f},
            .maxDepth {1.0f},
        };

        const vk::Rect2D scissor {
            .offset {0, 0}, .extent {swapchain.getExtent()}};

        const vk::PipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable {static_cast<vk::Bool32>(true)},
            .srcColorBlendFactor {vk::BlendFactor::eSrcAlpha},
            .dstColorBlendFactor {vk::BlendFactor::eOneMinusSrcAlpha},
            .colorBlendOp {vk::BlendOp::eAdd},
            .srcAlphaBlendFactor {vk::BlendFactor::eOne},
            .dstAlphaBlendFactor {vk::BlendFactor::eZero},
            .alphaBlendOp {vk::BlendOp::eAdd},
            .colorWriteMask {
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA},
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputState;

        switch (vertexType)
        {
        case VertexType::Parallax:
            vertexInputState = vk::PipelineVertexInputStateCreateInfo {
                .sType {vk::StructureType::ePipelineVertexInputStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .vertexBindingDescriptionCount {1},
                .pVertexBindingDescriptions {
                    ParallaxVertex::getBindingDescription()},
                .vertexAttributeDescriptionCount {static_cast<std::uint32_t>(
                    ParallaxVertex::getAttributeDescriptions()->size())},
                .pVertexAttributeDescriptions {
                    ParallaxVertex::getAttributeDescriptions()->data()},
            };
            break;
        case VertexType::Normal:
            vertexInputState = vk::PipelineVertexInputStateCreateInfo {
                .sType {vk::StructureType::ePipelineVertexInputStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .vertexBindingDescriptionCount {1},
                .pVertexBindingDescriptions {Vertex::getBindingDescription()},
                .vertexAttributeDescriptionCount {static_cast<std::uint32_t>(
                    Vertex::getAttributeDescriptions()->size())},
                .pVertexAttributeDescriptions {
                    Vertex::getAttributeDescriptions()->data()},
            };
            break;
        case VertexType::None:
            vertexInputState = vk::PipelineVertexInputStateCreateInfo {
                .sType {vk::StructureType::ePipelineVertexInputStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .vertexBindingDescriptionCount {0},
                .pVertexBindingDescriptions {nullptr},
                .vertexAttributeDescriptionCount {0},
                .pVertexAttributeDescriptions {nullptr},
            };
            break;
        }

        (*this) = GraphicsPipeline {
            device,
            renderPass,
            shaderStages,
            vertexInputState,
            vk::PipelineInputAssemblyStateCreateInfo {
                .sType {
                    vk::StructureType::ePipelineInputAssemblyStateCreateInfo},
                .pNext {},
                .flags {},
                .topology {topology},
                .primitiveRestartEnable {static_cast<vk::Bool32>(false)},
            },
            std::nullopt,
            vk::PipelineViewportStateCreateInfo {
                .sType {vk::StructureType::ePipelineViewportStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .viewportCount {1},
                .pViewports {&viewport},
                .scissorCount {1},
                .pScissors {&scissor},
            },
            vk::PipelineRasterizationStateCreateInfo {
                .sType {
                    vk::StructureType::ePipelineRasterizationStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .depthClampEnable {static_cast<vk::Bool32>(false)},
                .rasterizerDiscardEnable {static_cast<vk::Bool32>(false)},
                .polygonMode {vk::PolygonMode::eFill},
                .cullMode {vk::CullModeFlagBits::eBack},
                .frontFace {vk::FrontFace::eCounterClockwise},
                .depthBiasEnable {static_cast<vk::Bool32>(false)},
                .depthBiasConstantFactor {0.0f},
                .depthBiasClamp {0.0f},
                .depthBiasSlopeFactor {0.0f},
                .lineWidth {1.0f},
            },
            vk::PipelineMultisampleStateCreateInfo {
                .sType {vk::StructureType::ePipelineMultisampleStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .rasterizationSamples {vk::SampleCountFlagBits::e1},
                .sampleShadingEnable {static_cast<vk::Bool32>(false)},
                .minSampleShading {1.0f},
                .pSampleMask {nullptr},
                .alphaToCoverageEnable {static_cast<vk::Bool32>(false)},
                .alphaToOneEnable {static_cast<vk::Bool32>(false)},
            },
            vk::PipelineDepthStencilStateCreateInfo {
                .sType {
                    vk::StructureType::ePipelineDepthStencilStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .depthTestEnable {static_cast<vk::Bool32>(true)},
                .depthWriteEnable {static_cast<vk::Bool32>(true)},
                .depthCompareOp {vk::CompareOp::eLess},
                .depthBoundsTestEnable {static_cast<vk::Bool32>(false)},
                .stencilTestEnable {static_cast<vk::Bool32>(false)},
                .front {},
                .back {},
                .minDepthBounds {0.0f},
                .maxDepthBounds {1.0f},
            },
            vk::PipelineColorBlendStateCreateInfo {
                .sType {vk::StructureType::ePipelineColorBlendStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .logicOpEnable {static_cast<vk::Bool32>(false)},
                .logicOp {vk::LogicOp::eCopy},
                .attachmentCount {1},
                .pAttachments {&colorBlendAttachment},
                .blendConstants {{0.0f, 0.0f, 0.0f, 0.0f}},
            },
            std::move(layout_)};
    }

    GraphicsPipeline::GraphicsPipeline(
        vk::Device                                   device,
        vk::RenderPass                               renderPass,
        std::span<vk::PipelineShaderStageCreateInfo> shaderStages,
        vk::PipelineVertexInputStateCreateInfo       vertexInputState,
        std::optional<vk::PipelineInputAssemblyStateCreateInfo> inputAssembly,
        std::optional<vk::PipelineTessellationStateCreateInfo>  tesselationInfo,
        std::optional<vk::PipelineViewportStateCreateInfo>      viewportState,
        std::optional<vk::PipelineRasterizationStateCreateInfo> rasterization,
        std::optional<vk::PipelineMultisampleStateCreateInfo>  multiSampleState,
        std::optional<vk::PipelineDepthStencilStateCreateInfo> depthState,
        std::optional<vk::PipelineColorBlendStateCreateInfo>   colorBlendState,
        vk::UniquePipelineLayout                               layout_)
        : layout {std::move(layout_)}
        , pipeline {nullptr}
    {
        auto getValueOrNullptr = []<class T>(const std::optional<T>& o)
        {
            return (o.has_value() ? &*o : nullptr);
        };

        const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
            .sType {vk::StructureType::eGraphicsPipelineCreateInfo},
            .pNext {nullptr},
            .flags {},
            .stageCount {static_cast<std::uint32_t>(shaderStages.size())},
            .pStages {shaderStages.data()},
            .pVertexInputState {&vertexInputState},
            .pInputAssemblyState {getValueOrNullptr(inputAssembly)},
            .pTessellationState {getValueOrNullptr(tesselationInfo)},
            .pViewportState {getValueOrNullptr(viewportState)},
            .pRasterizationState {getValueOrNullptr(rasterization)},
            .pMultisampleState {getValueOrNullptr(multiSampleState)},
            .pDepthStencilState {getValueOrNullptr(depthState)},
            .pColorBlendState {getValueOrNullptr(colorBlendState)},
            .pDynamicState {nullptr},
            .layout {*this->layout},
            .renderPass {renderPass},
            .subpass {0},
            .basePipelineHandle {nullptr},
            .basePipelineIndex {-1},
        };

        auto [result, maybePipeline] = device.createGraphicsPipelineUnique(
            nullptr, graphicsPipelineCreateInfo);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to create graphics pipeline");

        this->pipeline = std::move(maybePipeline); // NOLINT
    }

    vk::Pipeline GraphicsPipeline::operator* () const
    {
        return *this->pipeline;
    }
    vk::PipelineLayout GraphicsPipeline::getLayout() const
    {
        return *this->layout;
    }

    ComputePipeline::ComputePipeline(
        vk::Device                               device,
        vk::UniqueShaderModule                   module,
        std::span<const vk::DescriptorSetLayout> setLayouts)
        : layout {nullptr}
        , pipeline {nullptr}
    {
        const vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {

            .sType {vk::StructureType::ePipelineShaderStageCreateInfo},
            .pNext {nullptr},
            .flags {},
            .stage {vk::ShaderStageFlagBits::eCompute},
            .module {*module},
            .pName {"main"},
            .pSpecializationInfo {},
        };

        const vk::PipelineLayoutCreateInfo layoutCreateInfo {
            .sType {vk::StructureType::ePipelineLayoutCreateInfo},
            .pNext {nullptr},
            .flags {},
            .setLayoutCount {static_cast<std::uint32_t>(setLayouts.size())},
            .pSetLayouts {setLayouts.data()},
            .pushConstantRangeCount {0}, // TODO: add push constants
            .pPushConstantRanges {nullptr},
        };

        this->layout = device.createPipelineLayoutUnique(layoutCreateInfo);

        const vk::ComputePipelineCreateInfo computePipelineCreateInfo {
            .sType {vk::StructureType::eComputePipelineCreateInfo},
            .pNext {nullptr},
            .flags {},
            .stage {shaderStageCreateInfo},
            .layout {*this->layout},
            .basePipelineHandle {nullptr},
            .basePipelineIndex {0},
        };

        auto [result, maybePipeline] = device.createComputePipelineUnique(
            nullptr, computePipelineCreateInfo);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to create compute pipeline");

        this->pipeline = std::move(maybePipeline); // NOLINT
    }

    vk::Pipeline ComputePipeline::operator* () const
    {
        return *this->pipeline;
    }
    vk::PipelineLayout ComputePipeline::getLayout() const
    {
        return *this->layout;
    }

} // namespace gfx::vulkan
