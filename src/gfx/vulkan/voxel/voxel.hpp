#ifndef SRC_GFX_VULKAN_VOXEL_VOXEL_HPP
#define SRC_GFX_VULKAN_VOXEL_VOXEL_HPP

#include <boost/container_hash/hash.hpp>
#include <cstdint>
#include <engine/settings.hpp>
#include <glm/common.hpp>
#include <util/misc.hpp>

namespace gfx::vulkan::voxel
{
    struct Voxel
    {
        /// 0          - Invisible / Invalid
        /// [1, 127]   - Translucent
        /// 128        - Opaque
        /// [129, 255] - Emissive
        std::uint8_t alpha_or_emissive;

        std::uint8_t srgb_r;
        std::uint8_t srgb_g;
        std::uint8_t srgb_b;

        /// 0 - nothing special
        /// 1 - anisotropic?
        /// [2, 255] UB
        std::uint8_t special;
        std::uint8_t specular;
        std::uint8_t roughness;
        std::uint8_t metallic;
    };

    static_assert(sizeof(Voxel) == sizeof(std::uint64_t));

    using Position = glm::vec<3, std::uint64_t>;

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

    /// States  |     Representation    |      Legend      |
    /// Invalid | 0x0000'0000'0000'0000 | _: any data      |
    /// Voxel   | 0x____'____'____'__~~ | ~: non zero data |
    /// Index   | 0x~~~~'~~~~'????'??00 | ?: unused        |
    /// Unused  | 0x0000'0000'????'??00 |                  |
    ///                              ^^
    /// As a note, the sentinel value is here

    union VoxelOrIndex
    {
    public:
        enum class State
        {
            Invalid,
            Voxel,
            Index,
        };
    public:
        /// Constructs a VoxelOrIndex storing its invalid state
        explicit VoxelOrIndex();

        /// Constructs a VoxelOrIndex storing a Voxel
        explicit VoxelOrIndex(Voxel);

        /// Constructs a VoxelOrIndex storing an Index
        explicit VoxelOrIndex(std::uint32_t);

        /// Checked Access
        [[nodiscard]] std::uint32_t getIndex() const;
        [[nodiscard]] Voxel         getVoxel() const;

        [[nodiscard]] State getState() const;
        [[nodiscard]] bool  isVoxel() const;
        [[nodiscard]] bool  isIndex() const;
        [[nodiscard]] bool  isValid() const;

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

namespace glm
{
    inline std::size_t
    hash_value(const gfx::vulkan::voxel::Position& pos) // NOLINT
    {
        boost::hash<std::size_t> sizeTHasher {};

        std::size_t workingHash = sizeTHasher(pos.x);
        boost::hash_combine(workingHash, sizeTHasher(pos.y));
        boost::hash_combine(workingHash, sizeTHasher(pos.z));

        return workingHash;
    }

} // namespace glm

#endif // SRC_GFX_VULKAN_VOXEL_HPP
