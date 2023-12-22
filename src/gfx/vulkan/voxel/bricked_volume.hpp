#ifndef SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP
#define SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP

#include "voxel.hpp"
#include <cstddef>
#include <gfx/vulkan/buffer.hpp>

namespace gfx::vulkan::voxel
{
    class BrickedVolume
    {
    public:
        struct DrawingArrays
        {
            vulkan::Buffer* brick_pointer_buffer;
            vulkan::Buffer* brick_buffer;
        };
    public:

        explicit BrickedVolume(Allocator*, std::size_t edgeLengthVoxels);
        ~BrickedVolume() = default;

        BrickedVolume(const BrickedVolume&)             = delete;
        BrickedVolume(BrickedVolume&&)                  = delete;
        BrickedVolume& operator= (const BrickedVolume&) = delete;
        BrickedVolume& operator= (BrickedVolume&&)      = delete;

        void                writeVoxel(Position, Voxel);
        [[nodiscard]] Voxel readVoxel(Position) const;

        void          updateGPU();
        DrawingArrays draw();

    private:

        vulkan::Buffer            brick_pointer_buffer;
        std::vector<VoxelOrIndex> brick_pointer_data;

        vulkan::Buffer     brick_buffer;
        std::vector<Brick> brick_data;
        std::size_t        next_free_brick_index;

        std::size_t edge_length_bricks;
    };
} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP