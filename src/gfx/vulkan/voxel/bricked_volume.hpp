#ifndef SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP
#define SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP

#include "voxel.hpp"
#include <cstddef>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <util/threads.hpp>

namespace gfx::vulkan::voxel
{
    class BrickedVolume
    {
    public:
        struct DrawingBuffers
        {
            vulkan::Buffer* brick_pointer_buffer;
            vulkan::Buffer* brick_buffer;
        };

        struct BrickFlushInformation
        {};
    public:

        explicit BrickedVolume(
            Device*, Allocator*, std::size_t edgeLengthVoxels);
        ~BrickedVolume() = default;

        BrickedVolume(const BrickedVolume&)             = delete;
        BrickedVolume(BrickedVolume&&)                  = delete;
        BrickedVolume& operator= (const BrickedVolume&) = delete;
        BrickedVolume& operator= (BrickedVolume&&)      = delete;

        // TODO: deal with overlapping positions!
        void writeVoxel(Position, Voxel) const;
        void writeVoxel(Position, const Brick&) const;

        void           flushToGPU(vk::CommandBuffer);
        DrawingBuffers getBuffers();

    private:

        [[nodiscard]] VoxelOrIndex allocateNewBrick() const;

        struct LockedData
        {
            vulkan::Buffer brick_pointer_buffer;
            vulkan::Buffer brick_buffer;

            // TODO: move out of the critical section with some lock free map
            std::unordered_map<VoxelOrIndex, Brick> brick_changes;
            std::unordered_map<Position, Voxel>     voxel_changes;

            // TODO: make a heap or some other data structure that can deal with
            // alloc and free
            std::size_t next_free_brick_index;
        };

        util::Mutex<LockedData> locked_data;

        std::size_t edge_length_bricks;
    };
} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP