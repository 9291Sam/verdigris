#ifndef SRC_GFX_VULKAN_VOXEL_STORAGE_MANAGER_HPP
#define SRC_GFX_VULKAN_VOXEL_STORAGE_MANAGER_HPP

#include <gfx/vulkan/buffer.hpp>

namespace gfx::vulkan::voxel
{
    /// Implementation a similar scheme
    ///

    /// Explanation
    /// One massive, linear buffer called the brick buffer voxel
    /// this is a massive, entierly linear buffer split into chunks
    /// this is entierly managed by the cpu
    /// modifications to it are managed by the cpu and the cpu keeps a list of
    /// buffer, when one needs to be removed, its cleared and marked as clear
    /// and used for new allocations

    class VoxelStorageManager
    {
    public:
        struct Voxel
        {
            std::uint8_t srgb_r;
            std::uint8_t srgb_g;
            std::uint8_t srgb_b;

            // 0 -> 127 represent varying levels of transparency 128 -> 255
            // represent emissive / light emissivity strength
            std::uint8_t alpha;

            std::uint8_t specular;
            std::uint8_t roughness;
            std::uint8_t metallic;
            std::uint8_t anisotropic; // / special, this is going to be shoved
                                      // into a switch case
        };
        static_assert(sizeof(Voxel) == sizeof(std::uint64_t));

        struct Brick
        {
            static constexpr std::size_t Dimension = 8;

            std::array<Voxel, Dimension * Dimension * Dimension> voxels;
        };

        static_assert(sizeof(Brick) == 4096);
        static_assert(std::is_trivially_copyable_v<Brick>);

    public:

    private:

        vulkan::Buffer brick_buffer;
        std::set
    };
}; // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_STORAGE_MANAGER_HPP
