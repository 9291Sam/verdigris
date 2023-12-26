#ifndef SRC_GAME_WORLD_WORLD_HPP
#define SRC_GAME_WORLD_WORLD_HPP

#include "chunk.hpp"
#include <set>
#include <util/noise.hpp>

namespace game
{
    class Game;
} // namespace game

namespace game::world
{

    class World
    {
    public:
        static std::int32_t
        generationFunc(std::int32_t x, std::int32_t z) noexcept
        {
            const float fX = static_cast<float>(x);
            const float fZ = static_cast<float>(z);

            float workingHeight = 0;

            const std::uint64_t seed {123890123123};

            workingHeight +=
                util::perlin(
                    glm::vec2 {fX / 783.2f + 0.4f, fZ / 783.2f - 2.0f},
                    (~seed + 16) >> 3U)
                * 384;

            workingHeight +=
                util::perlin(glm::vec2 {fX / 383.2f, fZ / 383.2f}, seed) * 128;

            workingHeight +=
                util::perlin(glm::vec2 {fX / 89.7f, fZ / 89.7f}, seed - 2) * 32;

            workingHeight +=
                util::perlin(glm::vec2 {fX / 65.6f, fZ / 312.6f}, seed + 4) * 3;

            return static_cast<std::int32_t>(workingHeight);
        }

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
