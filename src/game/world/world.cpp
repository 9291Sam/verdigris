
#include "game/world/world.hpp"
#include "game/world/sparse_volume.hpp"
#include "gfx/object.hpp"
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <util/misc.hpp>
#include <util/noise.hpp>

namespace
{
    std::int32_t generationFunc(std::int32_t x, std::int32_t z) noexcept
    {
        const float fX = static_cast<float>(x);
        const float fZ = static_cast<float>(z);

        float workingHeight = 0;

        const std::uint64_t seed {123890123123};

        workingHeight += util::perlin(
                             glm::vec2 {fX / 783.2f + 0.4f, fZ / 783.2f - 2.0f},
                             (~seed + 16) >> 3)
                       * 384;

        workingHeight +=
            util::perlin(glm::vec2 {fX / 383.2f, fZ / 383.2f}, seed) * 128;

        workingHeight +=
            util::perlin(glm::vec2 {fX / 89.7f, fZ / 89.7f}, seed - 2) * 32;

        workingHeight +=
            util::perlin(glm::vec2 {fX / 65.6f, fZ / 312.6f}, seed + 4) * 3;

        return workingHeight;
    }
} // namespace

namespace game::world
{

    World::World(const Game& game_)
        : game {game_}
        , chunks {}
    {
        // util::panic("update world to draw stuff");
        this->chunks.insert(Chunk {Position {-511, 0, -511}, generationFunc});
        this->chunks.insert(Chunk {Position {0, 0, -511}, generationFunc});
        this->chunks.insert(Chunk {Position {511, 0, -511}, generationFunc});

        this->chunks.insert(Chunk {Position {-511, 0, 0}, generationFunc});
        this->chunks.insert(Chunk {Position {0, 0, 0}, generationFunc});
        this->chunks.insert(Chunk {Position {511, 0, 0}, generationFunc});

        this->chunks.insert(Chunk {Position {-511, 0, 511}, generationFunc});
        this->chunks.insert(Chunk {Position {0, 0, 511}, generationFunc});
        this->chunks.insert(Chunk {Position {511, 0, 511}, generationFunc});
    }

    void World::updateChunkState()
    {
        for (const Chunk& c : this->chunks)
        {
            // this function, while non const, doesn't change the chunk's
            // storted state, thus this is fine
            const_cast<Chunk&>(c).updateDrawState(this->game.renderer);
        }
    }

    std::size_t World::estimateSize() const
    {
        // a realloc causes a surprising performance hit due to it calling
        // potentially ~30k+ move constructors, avoid please
        // return this->objects.size() * 3 / 2;
        return 150;
    }

} // namespace game::world
