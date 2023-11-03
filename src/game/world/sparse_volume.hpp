#ifndef SRC_GAME_WORLD_SPARSE_VOLUME_HPP
#define SRC_GAME_WORLD_SPARSE_VOLUME_HPP

#include <array>
#include <compare>
#include <cstdint>
#include <gfx/vulkan/gpu_structures.hpp>
#include <glm/fwd.hpp>
#include <memory>
#include <string>
#include <util/misc.hpp>
#include <variant>
#include <vector>

namespace game::world
{
    struct Position
    {
        std::int32_t x;
        std::int32_t y;
        std::int32_t z;

        Position             operator- () const;
        Position             operator- (Position other) const;
        Position             operator+ (Position other) const;
        Position             operator/ (std::int32_t) const;
        bool                 operator== (const Position&) const  = default;
        std::strong_ordering operator<=> (const Position&) const = default;

        explicit operator glm::vec3 () const;
        explicit operator std::string () const;
    };

    struct Voxel
    {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;

        [[nodiscard]] bool      shouldDraw() const;
        [[nodiscard]] glm::vec4 getColor() const;
        explicit                operator std::string () const;
    };

    struct VoxelVolume
    {
        static constexpr std::int32_t Extent {8};
        static constexpr std::int32_t Minimum {0};
        static constexpr std::int32_t Maximum {Extent - 1};

        VoxelVolume();
        VoxelVolume(Voxel fillVoxel);

        Voxel& accessFromLocalPosition(Position localPosition);

        void drawToVectors(
            std::vector<gfx::vulkan::Vertex>&,
            std::vector<gfx::vulkan::Index>&,
            Position localOffset);

    private:
        std::array<std::array<std::array<Voxel, Extent>, Extent>, Extent>
            storage;
    };

    class SparseVoxelVolume
    {
    public:
        static constexpr std::int32_t Extent {64}; // TODO: make int32_t pls

        static constexpr std::int32_t Minimum {
            -static_cast<std::int32_t>(Extent / 2)};

        static constexpr std::int32_t Maximum {
            static_cast<std::int32_t>(Extent / 2) - 1};

        static constexpr std::int32_t VoxelMinimum {
            Minimum * VoxelVolume::Extent};

        static constexpr std::int32_t VoxelMaximum {
            (Maximum + 1) * VoxelVolume::Extent - 1};

        static constexpr std::int32_t VoxelExtent {
            Extent * VoxelVolume::Extent};

        // Initalized with empty Voxels
        explicit SparseVoxelVolume();
        ~SparseVoxelVolume() = default;

        SparseVoxelVolume(const SparseVoxelVolume&)             = delete;
        SparseVoxelVolume(SparseVoxelVolume&&)                  = delete;
        SparseVoxelVolume& operator= (const SparseVoxelVolume&) = delete;
        SparseVoxelVolume& operator= (SparseVoxelVolume&&)      = delete;

        void populateVoxelsFromHeightFunction(
            util::Fn<std::int32_t>(std::int32_t, std::int32_t));
        Voxel& accessFromLocalPosition(Position localPosition);

        [[nodiscard]] std::pair<
            std::vector<gfx::vulkan::Vertex>,
            std::vector<gfx::vulkan::Index>>
        draw(Position offset);
    private:
        //   TODO: change to be contigious!
        std::array<
            std::array<
                std::array<
                    std::variant<std::unique_ptr<VoxelVolume>, Voxel>,
                    Extent>,
                Extent>,
            Extent>
            data;
    };

    // array lmfao

} // namespace game::world

#endif // SRC_GAME_WORLD_SPARSE_VOLUME_HPP