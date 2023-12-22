#ifndef SRC_GFX_VULKAN_VOXEL_VOXEL_HPP
#define SRC_GFX_VULKAN_VOXEL_VOXEL_HPP

#include <cstdint>
#include <engine/settings.hpp>
#include <glm/common.hpp>
#include <util/misc.hpp>

namespace gfx::vulkan::voxel
{
    struct Voxel
    {
        std::uint8_t srgb_r;
        std::uint8_t srgb_g;
        std::uint8_t srgb_b;

        /// 0          - Invisible / Invalid
        /// [1, 127]   - Translucent
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
    };

    static_assert(sizeof(Voxel) == sizeof(std::uint64_t));

    using Position = glm::vec<3, std::size_t>;

    struct Brick
    {
        static constexpr std::size_t EdgeLength = 8;

        util::CubicArray<Voxel, EdgeLength> voxels;

        Voxel& operator[] (Position p)
        {
            return this->voxels[p.x][p.y][p.z]; // NOLINT
        }

        Voxel operator[] (Position p) const
        {
            return this->voxels[p.x][p.y][p.z]; // NOLINT
        }
    };

    static_assert(sizeof(Brick) == 4096); // one page (on a normal system)

    // TODO: move to file
    union VoxelOrIndex
    {
    public:
        explicit VoxelOrIndex();
        explicit VoxelOrIndex(Voxel);
        explicit VoxelOrIndex(std::uint32_t);

        [[nodiscard]] std::uint32_t getIndex() const;
        [[nodiscard]] Voxel         getVoxel() const;

        [[nodiscard]] bool isVoxel() const;
        [[nodiscard]] bool isIndex() const;
        [[nodiscard]] bool isValid() const;

        void setIndex(std::uint32_t);
        void setVoxel(Voxel);

    private:
        Voxel voxel;
        struct
        {
            std::uint32_t _padding; // NOLINT
            std::uint32_t index;
        };
    };

} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_HPP