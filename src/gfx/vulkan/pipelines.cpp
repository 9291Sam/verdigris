#include "pipelines.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "gpu_data.hpp"
#include "render_pass.hpp"
#include "swapchain.hpp"
#include "util/log.hpp"
#include <fstream>
#include <memory>
#include <set>
#include <typeinfo>

namespace gfx::vulkan
{
    Pipeline createPipeline(
        PipelineType                pipeline,
        std::shared_ptr<Device>     device,
        std::shared_ptr<RenderPass> renderPass,
        std::shared_ptr<Swapchain>  swapchain)
    {
        switch (pipeline)
        {
        case PipelineType::None:
            util::panic("Tried to allocate Pipeline::None");
            break;

        case PipelineType::Flat: {
            vk::UniqueShaderModule fragmentShader =
                vulkan::Pipeline::createShaderFromFile(
                    device->asLogicalDevice(),
                    "src/gfx/vulkan/shaders/flat_pipeline.frag.bin");

            vk::UniqueShaderModule vertexShader =
                vulkan::Pipeline::createShaderFromFile(
                    device->asLogicalDevice(),
                    "src/gfx/vulkan/shaders/flat_pipeline.vert.bin");

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

            return Pipeline::create(
                PipelineVertexType::Normal,
                device,
                renderPass,
                swapchain,
                shaderStages,
                vk::PrimitiveTopology::eTriangleList,
                [&] // -> vk::UniquePipelineLayout
                {
                    const vk::PushConstantRange pushConstantsInformation {
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .offset {0},
                        .size {sizeof(vulkan::PushConstants)},
                    };

                    return device->asLogicalDevice().createPipelineLayoutUnique(
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
                }());
        }

        case PipelineType::Voxel: {
            vk::UniqueShaderModule fragmentShader =
                vulkan::Pipeline::createShaderFromFile(
                    device->asLogicalDevice(),
                    "src/gfx/vulkan/shaders/voxel.frag.bin");

            vk::UniqueShaderModule vertexShader =
                vulkan::Pipeline::createShaderFromFile(
                    device->asLogicalDevice(),
                    "src/gfx/vulkan/shaders/voxel.vert.bin");

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

            return Pipeline::create(
                PipelineVertexType::None,
                device,
                renderPass,
                swapchain,
                shaderStages,
                vk::PrimitiveTopology::eTriangleStrip,
                [&] //  -> vk::UniquePipelineLayout
                {
                    const vk::PushConstantRange pushConstantsInformation {
                        .stageFlags {vk::ShaderStageFlagBits::eVertex},
                        .offset {0},
                        .size {sizeof(vulkan::PushConstants)},
                    };

                    std::shared_ptr<vulkan::DescriptorSetLayout>
                        descriptorSetLayout = vulkan::getDescriptorSetLayout(
                            vulkan::DescriptorSetType::Voxel, device);

                    return device->asLogicalDevice().createPipelineLayoutUnique(
                        vk::PipelineLayoutCreateInfo {
                            .sType {
                                vk::StructureType::ePipelineLayoutCreateInfo},
                            .pNext {nullptr},
                            .flags {},
                            .setLayoutCount {1},
                            .pSetLayouts {**descriptorSetLayout},
                            .pushConstantRangeCount {1},
                            .pPushConstantRanges {&pushConstantsInformation},
                        });
                }());
        }
        }

        util::panic(
            "Unimplemented pipeline creation! {}", static_cast<int>(pipeline));
        util::unreachable();
    }

    vk::UniqueShaderModule
    Pipeline::createShaderFromFile(vk::Device device, const char* filePath)
    {
        std::ifstream fileStream {
            filePath, std::ios::in | std::ios::binary | std::ios::ate};

        util::assertFatal(
            fileStream.is_open(), "Failed to open file |{}|", filePath);

        std::vector<char> charBuffer {};
        charBuffer.resize(static_cast<std::size_t>(fileStream.tellg()));

        fileStream.seekg(0);
        fileStream.read(
            charBuffer.data(), static_cast<std::streamsize>(charBuffer.size()));

        // intellectual exercise :)
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

    Pipeline Pipeline::create(
        PipelineVertexType                           vertexType,
        std::shared_ptr<Device>                      device,
        std::shared_ptr<RenderPass>                  renderPass,
        std::shared_ptr<Swapchain>                   swapchain,
        std::span<vk::PipelineShaderStageCreateInfo> shaderStages,
        vk::PrimitiveTopology                        topology,
        vk::UniquePipelineLayout                     layout_)
    {
        const vk::Viewport viewport {
            .x {0.0f},
            .y {0.0f},
            .width {static_cast<float>(swapchain->getExtent().width)},
            .height {static_cast<float>(swapchain->getExtent().height)},
            .minDepth {0.0f},
            .maxDepth {1.0f},
        };

        const vk::Rect2D scissor {
            .offset {0, 0}, .extent {swapchain->getExtent()}};

        const vk::PipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable {true},
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
        case PipelineVertexType::Normal:
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
        case PipelineVertexType::None:
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

        return Pipeline {
            std::move(device),
            std::move(renderPass),
            std::move(swapchain),
            shaderStages,
            vertexInputState,
            vk::PipelineInputAssemblyStateCreateInfo {
                .sType {
                    vk::StructureType::ePipelineInputAssemblyStateCreateInfo},
                .pNext {},
                .flags {},
                .topology {topology},
                .primitiveRestartEnable {false},
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
                .depthClampEnable {false},
                .rasterizerDiscardEnable {false},
                .polygonMode {vk::PolygonMode::eFill},
                .cullMode {vk::CullModeFlagBits::eNone},
                .frontFace {vk::FrontFace::eCounterClockwise},
                .depthBiasEnable {false},
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
                .sampleShadingEnable {false},
                .minSampleShading {1.0f},
                .pSampleMask {nullptr},
                .alphaToCoverageEnable {false},
                .alphaToOneEnable {false},
            },
            vk::PipelineDepthStencilStateCreateInfo {
                .sType {
                    vk::StructureType::ePipelineDepthStencilStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .depthTestEnable {true},
                .depthWriteEnable {true},
                .depthCompareOp {vk::CompareOp::eLess},
                .depthBoundsTestEnable {false},
                .stencilTestEnable {false},
                .front {},
                .back {},
                .minDepthBounds {0.0f},
                .maxDepthBounds {1.0f},
            },
            vk::PipelineColorBlendStateCreateInfo {
                .sType {vk::StructureType::ePipelineColorBlendStateCreateInfo},
                .pNext {nullptr},
                .flags {},
                .logicOpEnable {false},
                .logicOp {vk::LogicOp::eCopy},
                .attachmentCount {1},
                .pAttachments {&colorBlendAttachment},
                .blendConstants {{0.0f, 0.0f, 0.0f, 0.0f}},
            },
            std::move(layout_)};
    }

    Pipeline::Pipeline()
        : device {nullptr}
        , render_pass {nullptr}
        , swapchain {nullptr}
        , layout {nullptr}
        , pipeline {nullptr}
    {}

    Pipeline::Pipeline(
        std::shared_ptr<Device>                      device_,
        std::shared_ptr<RenderPass>                  renderPass,
        std::shared_ptr<Swapchain>                   swapchain_,
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
        : device {std::move(device_)}
        , render_pass {std::move(renderPass)}
        , swapchain {std::move(swapchain_)}
        , layout {std::move(layout_)}
        , pipeline {nullptr}
    {
#define GET_VALUE_OR_NULLPTR(optional)                                         \
    (optional.has_value() ? &*optional : nullptr)

        const vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
            .sType {vk::StructureType::eGraphicsPipelineCreateInfo},
            .pNext {nullptr},
            .flags {},
            .stageCount {static_cast<std::uint32_t>(shaderStages.size())},
            .pStages {shaderStages.data()},
            .pVertexInputState {&vertexInputState},
            .pInputAssemblyState {GET_VALUE_OR_NULLPTR(inputAssembly)},
            .pTessellationState {GET_VALUE_OR_NULLPTR(tesselationInfo)},
            .pViewportState {GET_VALUE_OR_NULLPTR(viewportState)},
            .pRasterizationState {GET_VALUE_OR_NULLPTR(rasterization)},
            .pMultisampleState {GET_VALUE_OR_NULLPTR(multiSampleState)},
            .pDepthStencilState {GET_VALUE_OR_NULLPTR(depthState)},
            .pColorBlendState {GET_VALUE_OR_NULLPTR(colorBlendState)},
            .pDynamicState {nullptr},
            .layout {*this->layout},
            .renderPass {**this->render_pass},
            .subpass {0},
            .basePipelineHandle {nullptr},
            .basePipelineIndex {-1},
        };

#undef GET_VALUE_OR_NULLPTR

        auto [result, maybeGraphicsPipeline] =
            this->device->asLogicalDevice().createGraphicsPipelineUnique(
                nullptr, graphicsPipelineCreateInfo);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to create graphics pipeline | Error: {}",
            vk::to_string(result));

        this->pipeline = std::move(maybeGraphicsPipeline);
    }

    vk::Pipeline Pipeline::operator* () const
    {
        return *this->pipeline;
    }
    vk::PipelineLayout Pipeline::getLayout() const
    {
        return *this->layout;
    }

    // FlatPipeline::FlatPipeline(
    //     std::shared_ptr<Device>     device,
    //     std::shared_ptr<Swapchain>  swapchain,
    //     std::shared_ptr<RenderPass> renderPass)
    //     : Pipeline {
    //         device, // this is captured by the lambda
    //         std::move(renderPass),
    //         std::move(swapchain),
    //         PipelineBuilder {}
    //             .withVertexShader(vulkan::Pipeline::createShaderFromFile(
    //                 device->asLogicalDevice(),
    //                 "src/gfx/vulkan/shaders/flat_pipeline.vert.bin"))
    //             .withFragmentShader(vulkan::Pipeline::createShaderFromFile(
    //                 device->asLogicalDevice(),
    //                 "src/gfx/vulkan/shaders/flat_pipeline.frag.bin"))
    //             .withLayout()
    //             .transfer(),
    //         0} // id
    // {}

    // FlatPipeline::~FlatPipeline() {}
} // namespace gfx::vulkan
