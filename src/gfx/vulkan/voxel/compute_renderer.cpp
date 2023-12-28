#include "compute_renderer.hpp"
#include "voxel.hpp"
#include <game/world/world.hpp>
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
            std::array<Brick, 64> voxels;
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
              vk::ImageLayout::eUndefined,
              vk::ImageUsageFlagBits::eSampled
                  | vk::ImageUsageFlagBits::eStorage, //  It's similar to trying to save a screenshot, look at Sascha's example on that.
              vk::ImageAspectFlagBits::eColor,
              vk::ImageTiling::eOptimal,
              vk::MemoryPropertyFlagBits::eDeviceLocal}
        , time_alive {0.0f}
        , camera {Camera {glm::vec3 {}}}
        , generator {std::random_device {}()}
        , foo {0}
        , is_first_pass {true}
        // TODO: actually make the buffers sparse and reallocating (vkCmdCopyBuffer)
        , volume {this->device, this->allocator, 512} //! sync with shader!
    {
        // TODO: fix, wasnt writign enough data to the gpu
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

        const vk::DescriptorBufferInfo brickBufferBindInfo {
            // .buffer {*this->input_voxel_buffer},
            .buffer {this->volume.getBuffers().brick_buffer},
            .offset {0},
            .range {vk::WholeSize},
        };

        const vk::DescriptorBufferInfo brickPointerBufferBindInfo {
            // .buffer {*this->input_voxel_buffer},
            .buffer {this->volume.getBuffers().brick_pointer_buffer},
            .offset {0},
            .range {vk::WholeSize},
        };

        util::assertFatal(
            this->volume.getBuffers().brick_buffer != nullptr,
            "Brick Buffer was null!");

        const std::array<vk::WriteDescriptorSet, 4> setUpdateInfo {
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
                .pBufferInfo {&brickBufferBindInfo},
                .pTexelBufferView {nullptr}},
            vk::WriteDescriptorSet {
                .sType {vk::StructureType::eWriteDescriptorSet},
                .pNext {nullptr},
                .dstSet {*this->set},
                .dstBinding {3},
                .dstArrayElement {0},
                .descriptorCount {1},
                .descriptorType {vk::DescriptorType::eStorageBuffer},
                .pImageInfo {nullptr},
                .pBufferInfo {&brickPointerBufferBindInfo},
                .pTexelBufferView {nullptr}},
        };

        this->device->asLogicalDevice().updateDescriptorSets(
            setUpdateInfo, nullptr);

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

        if (!this->insert_voxels.has_value())
        {
            this->insert_voxels = std::async(
                [&]
                {
                    std::uniform_real_distribution<float> dist {0.8f, 1.0f};
                    std::uniform_real_distribution<float> nDist {0.0f, 1.0f};
                    std::uniform_int_distribution<std::size_t> distI {1, 2483};

                    auto distFunc = [&] -> float
                    {
                        return dist(this->generator);
                    };

                    for (const auto [x, y, z] : std::views::cartesian_product(
                             std::views::iota(
                                 0UZ, this->volume.getEdgeLengthVoxels()),
                             std::views::iota(
                                 0UZ, this->volume.getEdgeLengthVoxels()),
                             std::views::iota(
                                 0UZ, this->volume.getEdgeLengthVoxels())))
                    {
                        if ((3 * x + 2 * y + z) % distI(this->generator) == 0)
                        {
                            this->volume.writeVoxel(
                                {x, y, z},
                                Voxel {
                                    .alpha_or_emissive {128},
                                    .srgb_r {util::convertLinearToSRGB(
                                        nDist(this->generator))},
                                    .srgb_g {util::convertLinearToSRGB(
                                        nDist(this->generator))},
                                    .srgb_b {util::convertLinearToSRGB(
                                        nDist(this->generator))},
                                    .special {0},
                                    .specular {0},
                                    .roughness {255},
                                    .metallic {0},
                                });
                        }
                    }

                    for (std::size_t i = 0;
                         i < this->volume.getEdgeLengthVoxels();
                         ++i)
                    {
                        for (std::size_t j = 0;
                             j < this->volume.getEdgeLengthVoxels();
                             ++j)
                        {
                            float iF = static_cast<float>(i);
                            float jF = static_cast<float>(j);

                            std::int32_t i32oi =
                                static_cast<std::int32_t>(i)
                                - static_cast<std::int32_t>(
                                    this->volume.getEdgeLengthVoxels() / 2);

                            std::int32_t i32oj =
                                static_cast<std::int32_t>(j)
                                - static_cast<std::int32_t>(
                                    this->volume.getEdgeLengthVoxels() / 2);

                            this->volume.writeVoxel(
                                Position {
                                    i,
                                    // 4 * std::sin(iF / 4.0f) + 4 * std::cos(jF
                                    // / 4.0f)
                                    // + 32,
                                    ::game::world::World::generationFunc(
                                        i32oi, i32oj)
                                        + 256,
                                    j},
                                Voxel {
                                    .alpha_or_emissive {128},
                                    .srgb_r {util::convertLinearToSRGB(0.0f)},
                                    .srgb_g {
                                        util::convertLinearToSRGB(util::map(
                                            iF,
                                            0.0f,
                                            distFunc()
                                                * static_cast<float>(
                                                    this->volume
                                                        .getEdgeLengthVoxels()),
                                            0.0f,
                                            1.0f))},
                                    .srgb_b {
                                        util::convertLinearToSRGB(util::map(
                                            jF,
                                            0.0f,
                                            distFunc()
                                                * static_cast<float>(
                                                    this->volume
                                                        .getEdgeLengthVoxels()),
                                            0.0f,
                                            1.0f))},
                                    .special {0},
                                    .specular {0},
                                    .roughness {255},
                                    .metallic {0},
                                });
                        }
                    }
                });
        }
    }

    ComputeRenderer::~ComputeRenderer() = default;

    /// HACK:
    /// This function is very very broken and forces a data race to occur on
    /// the gpu side, all buffers and gpu objects need to be duplicated so
    /// that they can be synchronized with the command buffer's interleaving
    void ComputeRenderer::tick()
    {
        // TODO: move to a logical place
        this->time_alive += this->renderer.getFrameDeltaTimeSeconds();

        Camera c = this->camera;

        glm::mat4 modelViewProj =
            c.getPerspectiveMatrix(this->renderer, Transform {});

        UniformUploadInfo uniformUploadInfo {
            .inv_model_view_proj {glm::inverse(modelViewProj)},
            .model_view_proj {modelViewProj},
            .camera_position {glm::vec4 {c.getPosition(), 0.0f}},
            .sphere_center {glm::vec4 {18.0f, 2.5f, 3.0f, 0.0f}},
            .sphere_radius {2.0f},
        };

        this->input_uniform_buffer.write(util::asBytes(&uniformUploadInfo));

        {
            std::array<Brick, 64> b {};

            // ok, for some blessed reason, you can't have a uniform int
            // distribution over some types, including char
            std::uniform_int_distribution<std::uint16_t> dist {1, 255};

            auto distFunc = [&] -> std::uint8_t
            {
                return static_cast<std::uint8_t>(dist(this->generator));
            };

            this->foo = 0;
            glm::ivec3 index {};

            for (auto& x0 : b)
            {
                for (auto& x1 : x0.voxels)
                {
                    for (auto& x2 : x1)
                    {
                        for (Voxel& voxel : x2)
                        {
                            ++this->foo;
                            if (this->foo % distFunc() == 0
                                || this->foo % distFunc() == 0)
                            {
                                voxel = Voxel {
                                    .alpha_or_emissive {128},
                                    .srgb_r {static_cast<std::uint8_t>(
                                        index.x * 32 * distFunc())},
                                    .srgb_g {static_cast<std::uint8_t>(
                                        index.y * 32 * distFunc())},
                                    .srgb_b {static_cast<std::uint8_t>(
                                        index.z * 32 * distFunc())},
                                    .special {0},
                                    .specular {0},
                                    .roughness {255},
                                    .metallic {0},
                                };
                            }
                            else
                            {
                                voxel = Voxel {};
                            }
                            ++index.x %= 8;
                        }
                        ++index.y %= 8;
                    }
                    ++index.z %= 8;
                }
            }

            VoxelUploadInfo voxelUploadInfo {.voxels {b}};

            this->obj->transform.lock(
                [&](Transform& t)
                {
                    t.translation = uniformUploadInfo.sphere_center;
                    t.scale       = glm::vec3 {1.0f, 1.0f, 1.0f}
                            * uniformUploadInfo.sphere_radius;
                });

            this->input_voxel_buffer.write(util::asBytes(&voxelUploadInfo));
        }
    }

    void
    ComputeRenderer::render(vk::CommandBuffer commandBuffer, const Camera& c)
    {
        this->camera = c;

        bool reallocRequired = this->volume.flushToGPU(commandBuffer);

        if (reallocRequired)
        {
            const vk::DescriptorBufferInfo brickBufferBindInfo {
                // .buffer {*this->input_voxel_buffer},
                .buffer {this->volume.getBuffers().brick_buffer},
                .offset {0},
                .range {vk::WholeSize},
            };

            std::array update {vk::WriteDescriptorSet {
                .sType {vk::StructureType::eWriteDescriptorSet},
                .pNext {nullptr},
                .dstSet {*this->set},
                .dstBinding {2},
                .dstArrayElement {0},
                .descriptorCount {1},
                .descriptorType {vk::DescriptorType::eStorageBuffer},
                .pImageInfo {nullptr},
                .pBufferInfo {&brickBufferBindInfo},
                .pTexelBufferView {nullptr}}};

            this->device->asLogicalDevice().updateDescriptorSets(
                update, nullptr);
        }

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

        if (this->is_first_pass)
        {
            this->output_image.transitionLayout(
                commandBuffer,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eShaderWrite);

            this->is_first_pass = false;
        }
        else
        {
            this->output_image.transitionLayout(
                commandBuffer,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageLayout::eGeneral,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eShaderWrite);
        }

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
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead);
    }

    const vulkan::Image2D& ComputeRenderer::getImage() const
    {
        return this->output_image;
    }
} // namespace gfx::vulkan::voxel
