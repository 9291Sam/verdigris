#include "entity.hpp"
#include "game/game.hpp"
#include "gfx/camera.hpp"
#include <fmt/format.h>

game::entity::Entity::Entity(const Game& game_)
    : game {game_}
    , uuid {}
{}

game::entity::Entity::~Entity()
{
    this->game.removeEntity(this->uuid);
}

game::entity::Entity::operator std::string () const
{
    return fmt::format("Entity {}", static_cast<std::string>(this->uuid));
}

util::UUID game::entity::Entity::getUUID() const
{
    return this->uuid;
}

const gfx::Renderer& game::entity::Entity::getRenderer() const
{
    return this->game.renderer;
}

void game::entity::Entity::registerSelf()
{
    this->game.registerEntity(this->shared_from_this());
}
