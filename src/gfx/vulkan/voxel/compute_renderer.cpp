#include "compute_renderer.hpp"
#include <gfx/vulkan/allocator.hpp>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <gfx/vulkan/pipelines.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <util/log.hpp>

namespace gfx::vulkan::voxel
{
    ComputeRenderer::ComputeRenderer(
        Device*          device_,
        Allocator*       allocator_,
        PipelineManager* manager_,
        vk::Extent2D     extent)
        : device {device_}
        , allocator {allocator_}
        , pipeline_manager {manager_}
        , output_image {
              this->allocator,
              this->device->asLogicalDevice(),
              extent,
              vk::Format::eB8G8R8A8Srgb,
              vk::ImageUsageFlagBits::eSampled
                  | vk::ImageUsageFlagBits::eStorage,
              //   | vk::ImageUsageFlagBits::eTransferDst, // TODO:
              //   remove once
              // its generated on
              // the gpu soley
              vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlagBits::eDeviceLocal}
    {
        this->set = this->allocator->allocateDescriptorSet(
            DescriptorSetType::VoxelRayTracing);

        vk::UniqueSampler sampler =
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

        struct UploadInfo
        {
            glm::vec4 camera_position;
            glm::vec4 camera_forward;
            glm::vec4 sphere_center;
            float     sphere_radius;
            float     focal_length;
        };

        vulkan::Buffer setBuffer = vulkan::Buffer {
            this->allocator,
            sizeof(UploadInfo),
            vk::BufferUsageFlagBits::eUniformBuffer
                | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible};

        UploadInfo info {
            .camera_position {glm::vec4 {0.0f, 0.0f, 0.0f, 1.0f}},
            .camera_forward {glm::vec4 {0.0f, 0.0f, 0.0f, 0.0f}},
            .sphere_center {glm::vec4 {0.0f, 0.0f, 0.0f, 0.0f}},
            .sphere_radius {1.0f},
            .focal_length {1.0f}};

        setBuffer.write(std::span<const std::byte> {
            reinterpret_cast<const std::byte*>(&info), sizeof(UploadInfo)});

        const vk::DescriptorImageInfo imageBindInfo {
            .sampler {*sampler},
            .imageView {*this->output_image},
            .imageLayout {vk::ImageLayout::eGeneral}};

        const vk::DescriptorBufferInfo bufferBindInfo {
            .buffer {*setBuffer},
            .offset {0},
            .range {sizeof(UploadInfo)},
        };

        const std::array<vk::WriteDescriptorSet, 2> setUpdateInfo {
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
                .pBufferInfo {&bufferBindInfo},
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

                const vulkan::ComputePipeline& pipeline =
                    this->pipeline_manager->getComputePipeline(
                        ComputePipelineType::RayCaster);

                commandBuffer.bindPipeline(
                    vk::PipelineBindPoint::eCompute, *pipeline);

                commandBuffer.bindDescriptorSets(
                    vk::PipelineBindPoint::eCompute,
                    pipeline.getLayout(),
                    0,
                    *this->set,
                    {});

                this->output_image.transitionLayout(
                    commandBuffer,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::AccessFlagBits::eNone,
                    vk::AccessFlagBits::eShaderWrite);

                commandBuffer.dispatch(8, 8, 1);

                this->output_image.transitionLayout(
                    commandBuffer,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::PipelineStageFlagBits::eComputeShader,
                    vk::AccessFlagBits::eShaderWrite,
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
    }

    ComputeRenderer::~ComputeRenderer() = default;

    vk::ImageView ComputeRenderer::getImage()
    {
        return *this->output_image;
    }
} // namespace gfx::vulkan::voxel