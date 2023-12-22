#include "bricked_volume.hpp"
#include <gfx/vulkan/allocator.hpp>

namespace gfx::vulkan::voxel
{
    BrickedVolume::BrickedVolume(
        Allocator* allocator, std::size_t edgeLengthVoxels)
        : edge_length_bricks {edgeLengthVoxels / Brick::EdgeLength}
    {
        util::assertFatal(
            edgeLengthVoxels % Brick::EdgeLength == 0,
            "Edge length of {} is not a multiple of {}",
            edgeLengthVoxels,
            Brick::EdgeLength);

        const std::size_t numberOfBricks = this->edge_length_bricks
                                         * this->edge_length_bricks
                                         * this->edge_length_bricks;

        {
            this->brick_pointer_buffer = vulkan::Buffer {
                allocator,
                sizeof(VoxelOrIndex) * numberOfBricks,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
                    | vk::MemoryPropertyFlagBits::eHostVisible};

            this->brick_pointer_data.resize(
                numberOfBricks, VoxelOrIndex {~std::uint32_t {0}});

            this->brick_pointer_buffer.write(
                std::as_bytes(std::span {this->brick_pointer_data}));
        }

        {
            this->brick_buffer = vulkan::Buffer {
                allocator,
                sizeof(VoxelOrIndex) * numberOfBricks,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
                    | vk::MemoryPropertyFlagBits::eHostVisible};

            this->brick_data.resize(numberOfBricks, VoxelOrIndex {Voxel {}});

            this->brick_buffer.write(
                std::as_bytes(std::span {this->brick_data}));
        }

        util::panic("add asserts and esure throws for VoxelOrIndex");
    }
} // namespace gfx::vulkan::voxel