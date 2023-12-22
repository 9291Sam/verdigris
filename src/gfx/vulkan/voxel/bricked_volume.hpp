#ifndef SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP
#define SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP

#include "voxel.hpp"
#include <cstddef>
#include <gfx/vulkan/buffer.hpp>
#include <glm/vec4.hpp>

namespace gfx::vulkan::voxel
{
    class BrickedVolume
    {
    public:
        using Position = glm::vec<3, std::size_t>;

        struct DrawingArrays
        {
            vulkan::Buffer* brick_pointer_buffer;
            vulkan::Buffer* brick_buffer;
        };
    public:

        explicit BrickedVolume(Allocator*, std::size_t edgeLengthVoxels);
        ~BrickedVolume();

        BrickedVolume(const BrickedVolume&)             = delete;
        BrickedVolume(BrickedVolume&&)                  = delete;
        BrickedVolume& operator= (const BrickedVolume&) = delete;
        BrickedVolume& operator= (BrickedVolume&&)      = delete;

        void                writeVoxel(Position, Voxel);
        [[nodiscard]] Voxel readVoxel(Position) const;

        DrawingArrays draw();

    private:

        vulkan::Buffer            brick_pointer_buffer;
        std::vector<VoxelOrIndex> brick_pointer_data;

        vulkan::Buffer            brick_buffer;
        std::vector<VoxelOrIndex> brick_data;

        std::size_t edge_length_bricks;
    };
} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_BRICKED_VOLUME_HPP