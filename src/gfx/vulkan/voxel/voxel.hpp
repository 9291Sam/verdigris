#ifndef SRC_GFX_VULKAN_VOXEL_VOXEL_HPP
#define SRC_GFX_VULKAN_VOXEL_VOXEL_HPP

#include <array>
#include <cstdint>

namespace gfx::vulkan::voxel
{
    struct Voxel
    {
        std::uint8_t srgb_r;
        std::uint8_t srgb_g;
        std::uint8_t srgb_b;

        /// [0, 127]   - Transluscent
        /// 128        - Opaque
        /// [129, 255] - Emissive
        std::uint8_t alpha_or_emissive;

        // TODO: there's too much information here
        std::uint8_t specular;
        std::uint8_t roughness;
        std::uint8_t metallic;

        /// 0 - nothing special
        /// 1 - anisotropic?
        /// [2, 255] UB
        std::uint8_t special;
        // std::uint64_t padding;
    };

    // static_assert(sizeof(Voxel) == sizeof(std::uint64_t) * 2);

    struct Brick
    {
        std::array<std::array<std::array<Voxel, 12>, 12>, 12> voxels;
    };
} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_HPP