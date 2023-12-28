#ifndef SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP
#define SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP

#include "voxel.hpp"
#include <boost/unordered/concurrent_flat_map.hpp>
#include <cstddef>
#include <gfx/vulkan/buffer.hpp>
#include <gfx/vulkan/device.hpp>
#include <util/block_allocator.hpp>
#include <util/threads.hpp>

namespace gfx::vulkan::voxel
{
    class BrickedVolume
    {
    public:
        struct DrawingBuffers
        {
            vk::Buffer brick_pointer_buffer;
            vk::Buffer brick_buffer;
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
        // TODO: make these non blocking
        void writeVoxel(Position, Voxel) const;
        void writeVoxel(Position, const Brick&) const;

        std::size_t getEdgeLengthVoxels() const;

        // true if rebind is needed
        [[nodiscard]] bool flushToGPU(vk::CommandBuffer);
        DrawingBuffers     getBuffers();

    private:
        // TODO: make a shader that garbage collects the volumes

        struct LockedData
        {
            vulkan::Buffer            brick_pointer_buffer;
            std::vector<VoxelOrIndex> brick_pointer_data;

            vulkan::Buffer                brick_buffer;
            // TODO: this is absolutely terrible, you need to solve this
            // programmatically
            std::size_t                   frames_stalled;
            std::optional<vulkan::Buffer> maybe_prev_brick_buffer;

            util::BlockAllocator allocator;
            bool                 is_realloc_required;
        };

        [[nodiscard]] static std::
            expected<VoxelOrIndex, util::BlockAllocator::OutOfBlocks>
            allocateNewBrick(util::BlockAllocator&);

        Allocator* allocator;

        mutable boost::unordered::concurrent_flat_map<std::uint32_t, Brick>
            brick_changes;
        mutable boost::unordered::concurrent_flat_map<Position, Voxel>
            voxel_changes;

        std::size_t edge_length_bricks;

        util::Mutex<LockedData> locked_data;
    };

} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP
