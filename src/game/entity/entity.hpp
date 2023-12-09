#ifndef SRC_GAME_ENTITY_HPP
#define SRC_GAME_ENTITY_HPP

#include <gfx/renderer.hpp>
#include <util/uuid.hpp>

namespace game
{
    class Game;
}

namespace game::entity
{
    class Entity : public std::enable_shared_from_this<Entity>
    {
    public:
        static std::shared_ptr<Entity> create(const Game&);
        virtual ~Entity();

        Entity(const Entity&)             = delete;
        Entity(Entity&&)                  = delete;
        Entity& operator= (const Entity&) = delete;
        Entity& operator= (Entity&&)      = delete;

        // This function will be called concurrently with other Entity's tick
        // fuctions. It is safe to mark a field mutable if and only if it only
        // accessed by one tick function. If there is direct communication
        // between other entities, that's a race without synchronization
        virtual void     tick() const = 0;
        virtual explicit operator std::string () const;

        util::UUID getUUID() const;

    protected:
        const gfx::Renderer& getRenderer() const;
        void                 registerSelf();

        const Game&      game;
        const util::UUID uuid;

        explicit Entity(const Game&);
    };
} // namespace game::entity

#endif // SRC_GAME_ENTITY_HPP
