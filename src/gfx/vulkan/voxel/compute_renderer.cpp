#include "compute_renderer.hpp"
#include "voxel.hpp"
#include <gfx/renderer.hpp>
#include <gfx/vulkan/allocator.hpp>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <util/log.hpp>

namespace gfx::vulkan::voxel
{

    namespace
    {
        struct UniformUploadInfo
        {
            glm::mat4 inv_model_view_proj;
            glm::mat4 model_view_proj;
            glm::vec4 camera_position;
            glm::vec4 sphere_center;

            float sphere_radius;
        };

        struct VoxelUploadInfo
        {
            Brick voxels;
        };
    } // namespace

    ComputeRenderer::ComputeRenderer(
        const Renderer&  renderer_,
        Device*          device_,
        Allocator*       allocator_,
        PipelineManager* manager_,
        vk::Extent2D     extent)
        : renderer {renderer_}
        , device {device_}
        , allocator {allocator_}
        , pipeline_manager {manager_}
        , output_image {
              this->allocator,
              this->device->asLogicalDevice(),
              extent,
              vk::Format::eR8G8B8A8Snorm,
              vk::ImageUsageFlagBits::eSampled
                  | vk::ImageUsageFlagBits::eStorage, //  It's similar to trying to save a screenshot, look at Sascha's example on that.
              vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlagBits::eDeviceLocal}
        , time_alive {0.0f}
        , camera {Camera {glm::vec3 {}}}
        , generator {std::random_device {}()}
    {
        this->set = this->allocator->allocateDescriptorSet(
            DescriptorSetType::VoxelRayTracing);

        this->output_image_sampler =
            this->device->asLogicalDevice().createSamplerUnique(
                vk::SamplerCreateInfo {
                    .sType {vk::StructureType::eSamplerCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                    .magFilter {vk::Filter::eLinear},
                    .minFilter {vk::Filter::eLinear},
                    .mipmapMode {vk::SamplerMipmapMode::eLinear},
                    .addressModeU {vk::SamplerAddressMode::eRepeat},
                    .addressModeV {vk::SamplerAddressMode::eRepeat},
                    .addressModeW {vk::SamplerAddressMode::eRepeat},
                    .mipLodBias {},
                    .anisotropyEnable {static_cast<vk::Bool32>(false)},
                    .maxAnisotropy {1.0f},
                    .compareEnable {static_cast<vk::Bool32>(false)},
                    .compareOp {vk::CompareOp::eNever},
                    .minLod {0},
                    .maxLod {1},
                    .borderColor {vk::BorderColor::eFloatTransparentBlack},
                    .unnormalizedCoordinates {},
                });

        this->input_uniform_buffer = vulkan::Buffer {
            this->allocator,
            sizeof(UniformUploadInfo),
            vk::BufferUsageFlagBits::eUniformBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible};

        this->input_voxel_buffer = vulkan::Buffer {
            this->allocator,
            sizeof(VoxelUploadInfo),
            vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible};

        UniformUploadInfo uniformInfo {};
        VoxelUploadInfo   voxelInfo {};

        this->input_uniform_buffer.write(util::asBytes(&uniformInfo));
        this->input_voxel_buffer.write(util::asBytes(&voxelInfo));

        const vk::DescriptorImageInfo imageBindInfo {
            .sampler {*this->output_image_sampler},
            .imageView {*this->output_image},
            .imageLayout {vk::ImageLayout::eGeneral}};

        const vk::DescriptorBufferInfo uniformBufferBindInfo {
            .buffer {*this->input_uniform_buffer},
            .offset {0},
            .range {sizeof(UniformUploadInfo)},
        };

        const vk::DescriptorBufferInfo storageBufferBindInfo {
            .buffer {*this->input_voxel_buffer},
            .offset {0},
            .range {sizeof(VoxelUploadInfo)},
        };

        const std::array<vk::WriteDescriptorSet, 3> setUpdateInfo {
            vk::WriteDescriptorSet {
                .sType {vk::StructureType::eWriteDescriptorSet},
                .pNext {nullptr},
                .dstSet {*this->set},
                .dstBinding {0},
                .dstArrayElement {0},
                .descriptorCount {1},
                .descriptorType {vk::DescriptorType::eStorageImage},
                .pImageInfo {&imageBindInfo},
                .pBufferInfo {nullptr},
                .pTexelBufferView {nullptr}},
            vk::WriteDescriptorSet {
                .sType {vk::StructureType::eWriteDescriptorSet},
                .pNext {nullptr},
                .dstSet {*this->set},
                .dstBinding {1},
                .dstArrayElement {0},
                .descriptorCount {1},
                .descriptorType {vk::DescriptorType::eUniformBuffer},
                .pImageInfo {nullptr},
                .pBufferInfo {&uniformBufferBindInfo},
                .pTexelBufferView {nullptr}},
            vk::WriteDescriptorSet {
                .sType {vk::StructureType::eWriteDescriptorSet},
                .pNext {nullptr},
                .dstSet {*this->set},
                .dstBinding {2},
                .dstArrayElement {0},
                .descriptorCount {1},
                .descriptorType {vk::DescriptorType::eStorageBuffer},
                .pImageInfo {nullptr},
                .pBufferInfo {&storageBufferBindInfo},
                .pTexelBufferView {nullptr}},
        };

        this->device->asLogicalDevice().updateDescriptorSets(
            setUpdateInfo, nullptr);

        const vk::FenceCreateInfo fenceCreateInfo {
            .sType {vk::StructureType::eFenceCreateInfo},
            .pNext {nullptr},
            .flags {},
        };

        vk::UniqueFence endTransferFence =
            this->device->asLogicalDevice().createFenceUnique(fenceCreateInfo);

        this->device->accessQueue(
            vk::QueueFlagBits::eCompute,
            [&](vk::Queue queue, vk::CommandBuffer commandBuffer)
            {
                const vk::CommandBufferBeginInfo BeginInfo {
                    .sType {vk::StructureType::eCommandBufferBeginInfo},
                    .pNext {nullptr},
                    .flags {vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
                    .pInheritanceInfo {nullptr},
                };

                commandBuffer.begin(BeginInfo);

                // we need to change the layout on the first time
                this->output_image.transitionLayout(
                    commandBuffer,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::AccessFlagBits::eNone,
                    vk::AccessFlagBits::eShaderRead);

                commandBuffer.end();

                const vk::SubmitInfo SubmitInfo {
                    .sType {vk::StructureType::eSubmitInfo},
                    .pNext {nullptr},
                    .waitSemaphoreCount {0},
                    .pWaitSemaphores {nullptr},
                    .pWaitDstStageMask {nullptr},
                    .commandBufferCount {1},
                    .pCommandBuffers {&commandBuffer},
                    .signalSemaphoreCount {0},
                    .pSignalSemaphores {nullptr},
                };

                queue.submit(SubmitInfo, *endTransferFence);
            });

        const vk::Result result = this->device->asLogicalDevice().waitForFences(
            *endTransferFence, static_cast<vk::Bool32>(true), -1);

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to wait for image trnasfer");

        static constexpr std::array<gfx::vulkan::Vertex, 8> Vertices {
            gfx::vulkan::Vertex {
                .position {-1.0f, -1.0f, -1.0f},
                .color {0.0f, 0.0f, 0.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {-1.0f, -1.0f, 1.0f},
                .color {0.0f, 0.0f, 1.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {-1.0f, 1.0f, -1.0f},
                .color {0.0f, 1.0f, 0.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {-1.0f, 1.0f, 1.0f},
                .color {0.0f, 1.0f, 1.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {1.0f, -1.0f, -1.0f},
                .color {1.0f, 0.0f, 0.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {1.0f, -1.0f, 1.0f},
                .color {1.0f, 0.0f, 1.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {1.0f, 1.0f, -1.0f},
                .color {1.0f, 1.0f, 0.0f, 1.0f},
                .normal {},
                .uv {},
            },
            gfx::vulkan::Vertex {
                .position {1.0f, 1.0f, 1.0f},
                .color {1.0f, 1.0f, 1.0f, 1.0f},
                .normal {},
                .uv {},
            },
        };

        static constexpr std::array<gfx::vulkan::Index, 36> Indices {
            6, 2, 7, 2, 3, 7, 0, 4, 5, 1, 0, 5, 0, 2, 6, 4, 0, 6,
            3, 1, 7, 1, 5, 7, 2, 0, 3, 0, 1, 3, 4, 6, 7, 5, 4, 7};

        this->obj = gfx::SimpleTriangulatedObject::create(
            this->renderer,
            std::vector<gfx::vulkan::Vertex> {Vertices.begin(), Vertices.end()},
            std::vector<gfx::vulkan::Index> {Indices.begin(), Indices.end()});
    }

    ComputeRenderer::~ComputeRenderer() = default;

    void ComputeRenderer::tick()
    {
        // TODO: move to a logical place
        this->time_alive += this->renderer.getFrameDeltaTimeSeconds();

        Camera c = this->camera;

        glm::mat4 modelViewProj =
            c.getPerspectiveMatrix(this->renderer, Transform {});

        Brick b {};

        std::uniform_int_distribution<std::uint8_t> dist {0, 255};

        for (auto& x1 : b.voxels)
        {
            for (auto& x2 : x1)
            {
                for (Voxel& voxel : x2)
                {
                    ++this->foo;
                    if (this->foo % dist(generator) == 0
                        || this->foo % dist(generator) == 0)
                    {
                        voxel = Voxel {
                            .srgb_r {dist(generator)},
                            .srgb_g {dist(generator)},
                            .srgb_b {dist(generator)},
                            .alpha_or_emissive {128},
                            .specular {0},
                            .roughness {255},
                            .metallic {0},
                            .special {0},
                        };
                    }
                    else
                    {
                        voxel = Voxel {};
                    }
                }
            }
        }

        UniformUploadInfo uniformUploadInfo {
            .inv_model_view_proj {glm::inverse(modelViewProj)},
            .model_view_proj {modelViewProj},
            .camera_position {glm::vec4 {c.getPosition(), 0.0f}},
            .sphere_center {glm::vec4 {18.0f, 2.5f, 3.0f, 0.0f}},
            .sphere_radius {2.0f},
        };

        VoxelUploadInfo voxelUploadInfo {.voxels {b}};

        this->obj->transform.lock(
            [&](Transform& t)
            {
                t.translation = uniformUploadInfo.sphere_center;
                t.scale       = glm::vec3 {1.0f, 1.0f, 1.0f}
                        * uniformUploadInfo.sphere_radius;
            });

        this->input_uniform_buffer.write(util::asBytes(&uniformUploadInfo));
        this->input_voxel_buffer.write(util::asBytes(&voxelUploadInfo));
    }

    void
    ComputeRenderer::render(vk::CommandBuffer commandBuffer, const Camera& c)
    {
        this->camera = c;

        const vulkan::ComputePipeline& pipeline =
            this->pipeline_manager->getComputePipeline(
                ComputePipelineType::RayCaster);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);

        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            pipeline.getLayout(),
            0,
            *this->set,
            {});

        this->output_image.transitionLayout(
            commandBuffer,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderRead,
            vk::AccessFlagBits::eShaderWrite);

        auto [x, y] = this->output_image.getExtent();
        commandBuffer.dispatch(
            util::ceilingDivide(x, ComputeRenderer::ShaderDispatchSize.width),
            util::ceilingDivide(y, ComputeRenderer::ShaderDispatchSize.height),
            1);

        this->output_image.transitionLayout(
            commandBuffer,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
    }

    const vulkan::Image2D& ComputeRenderer::getImage() const
    {
        return this->output_image;
    }
} // namespace gfx::vulkan::voxel