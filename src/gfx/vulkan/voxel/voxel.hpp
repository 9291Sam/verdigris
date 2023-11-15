#ifndef SRC_GFX_VULKAN_VOXEL_VOXEL_HPP
#define SRC_GFX_VULKAN_VOXEL_VOXEL_HPP

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
        std::uint8_t alpha_emissive;

        std::uint8_t specular;
        std::uint8_t roughness;
        std::uint8_t metallic;

        /// 0 - nothing special
        /// 1 - anisotropic?
        /// [2, 255] UB
        std::uint8_t special;
    };
} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_HPP