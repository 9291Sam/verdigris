#ifndef SRC_GAME_GAME_HPP
#define SRC_GAME_GAME_HPP

#include "player.hpp"
#include "world/world.hpp"
#include <chrono>
#include <memory>
#include <util/registrar.hpp>
#include <util/uuid.hpp>

namespace gfx
{
    class Renderer;
}

namespace game
{

    namespace entity
    {
        class Entity;
    } // namespace entity

    class Game
    {
    public:

        explicit Game(const gfx::Renderer&);
        ~Game();

        Game(const Game&)             = delete;
        Game(Game&&)                  = delete;
        Game& operator= (const Game&) = delete;
        Game& operator= (Game&&)      = delete;

        [[nodiscard]] float getTickDeltaTimeSeconds() const;

        [[nodiscard]] bool continueTicking();
        void               tick();

    private:

        // TODO: get a better way to handle this
        friend entity::Entity;
        friend class Player;
        friend class world::World;
        void registerEntity(const std::shared_ptr<const entity::Entity>&) const;
        void removeEntity(util::UUID) const;

        const gfx::Renderer& renderer;
        util::Registrar<util::UUID, std::weak_ptr<const entity::Entity>>
            entities;
        // TODO: move this isolated since it needs to be isolated for better
        // movement
        // we need a "game"(physics) tick and then a "world" tick?

        Player                                       player;
        // world::World                                 world;
        std::vector<std::shared_ptr<entity::Entity>> temp_entities;

        std::chrono::time_point<std::chrono::steady_clock> last_tick_end_time;
        std::atomic<std::chrono::duration<float>>          last_tick_duration;
    };
} // namespace game

#endif // SRC_GAME_GAME_HPP
