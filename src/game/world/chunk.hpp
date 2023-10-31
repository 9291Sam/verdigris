#ifndef SRC_GAME_WORLD_CHUNK_HPP
#define SRC_GAME_WORLD_CHUNK_HPP

#include "game/world/sparse_volume.hpp"
#include "gfx/object.hpp"
#include "util/misc.hpp"
#include <memory>
#include <optional>

namespace game::world
{

    using ChunkCoordinate = Position;

    struct LodLevel
    {
        constexpr LodLevel(std::size_t distanceFromView)
        {
            static_assert(SparseVoxelVolume::VoxelExtent == 512);

            if (distanceFromView <= 512)
            {
                this->level = util::log2<std::size_t>(512) + 1;
            }

            std::size_t result = 256;

            while (result > 1 && distanceFromView > 1024)
            {
                distanceFromView /= 2;
                result /= 2;
            }

            this->level = util::log2(result) + 1;
        }

        std::strong_ordering operator<=> (const LodLevel&) const = default;

        constexpr std::size_t getNumberOfVoxelsPerChunks() const
        {
            return util::exp(2, this->level - 1);
        }

    private:
        std::uint8_t level;
    };
    static_assert(LodLevel {755}.getNumberOfVoxelsPerChunks() == 256);

    enum class ChunkStates : std::uint8_t;

    // yup its a state machine, deal with it :cry:
    class Chunk
    {
    public:
        Chunk();
        Chunk(
            Position,
            util::Fn<std::int32_t(std::int32_t, std::int32_t) noexcept>);
        ~Chunk() = default;

        Chunk(const Chunk&)                 = delete;
        Chunk(Chunk&&) noexcept             = default;
        Chunk& operator= (const Chunk&)     = delete;
        Chunk& operator= (Chunk&&) noexcept = default;

        std::strong_ordering operator<=> (const Chunk& other) const
        {
            return this->location <=> other.location;
        }

        // draw and lod is too low, increase, if its too high, keep it high,
        // unless its really far or we need more memory

        // returns std::nullopt when the chunk is still being drawn and
        // isn't
        // ready
        // TODO: add position and have each one manage their own LODs
        void updateDrawState(const gfx::Renderer&);

    private:

        Position getCenterLocation() const;
        bool     isPositionWithinRadius(Position) const;

        ChunkCoordinate     location;
        LodLevel            lod;
        mutable ChunkStates state;

        std::shared_ptr<SparseVoxelVolume>             volume;
        std::shared_ptr<gfx::SimpleTriangulatedObject> object;

        std::optional<std::future<std::shared_ptr<SparseVoxelVolume>>>
            future_volume;
        std::optional<
            std::future<std::shared_ptr<gfx::SimpleTriangulatedObject>>>
            future_object;
    };
} // namespace game::world

#endif // SRC_GAME_WORLD_CHUNK_HPP
