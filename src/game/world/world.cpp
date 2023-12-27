
#include "game/world/world.hpp"
#include "game/world/sparse_volume.hpp"
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <util/misc.hpp>
#include <util/noise.hpp>

namespace game::world
{

    World::World(const Game& game_)
        : game {game_}
    {
        std::int32_t radius = 1;

        for (std::int32_t x = -radius; x <= radius; x++)
        {
            for (std::int32_t z = -radius; z <= radius; z++)
            {
                std::int32_t chunkX = static_cast<std::int32_t>(x * 512);
                std::int32_t chunkZ = static_cast<std::int32_t>(z * 512);

                this->chunks.insert(Chunk {
                    Position {chunkX - x, 0, chunkZ - z}, generationFunc});
            }
        }
    }

    void World::updateChunkState()
    {
        for (const Chunk& c : this->chunks)
        {
            // this function, while non const, doesn't change the chunk's
            // ordering, thus this is fine
            // NOLINTNEXTLINE
            const_cast<Chunk&>(c).updateDrawState(this->game.renderer);
        }
    }

    std::size_t World::estimateSize() const
    {
        // a realloc causes a surprising performance hit due to it calling
        // potentially ~30k+ move constructors, avoid please
        return this->chunks.size() * 3 / 2;
    }

} // namespace game::world
