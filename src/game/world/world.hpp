#ifndef SRC_GAME_WORLD_WORLD_HPP
#define SRC_GAME_WORLD_WORLD_HPP

#include "chunk.hpp"
#include <set>

namespace game
{
    class Game;
} // namespace game

namespace game::world
{
    class World
    {
    public:

        explicit World(const Game&);
        ~World() = default;

        World(const World&)             = delete;
        World(World&&)                  = delete;
        World& operator= (const World&) = delete;
        World& operator= (World&&)      = delete;

        [[nodiscard]] std::size_t estimateSize() const;
        void                      updateChunkState();

    private:
        const Game&     game;
        std::set<Chunk> chunks;
    };
} // namespace game::world

#endif // SRC_GAME_WORLD_WORLD_HPP
