#include "game.hpp"
#include "entity/cube.hpp"
#include "entity/disk_entity.hpp"
#include <gfx/imgui_menu.hpp>
#include <gfx/renderer.hpp>
#include <util/log.hpp>

namespace game
{
    Game::Game(gfx::Renderer& renderer_)
        : renderer {renderer_}
        , entities {}
        , player {*this, {-30.0f, 20.0f, -20.0f}}
        , world {*this}
    {
        this->temp_entities.push_back(
            entity::Cube::create(*this, glm::vec3 {0.0f, 12.5f, 0.0f}));

        this->temp_entities.push_back(
            entity::DiskEntity::create(*this, "../models/gizmo.obj"));

        this->player.getCamera().addPitch(0.418879019f);
        this->player.getCamera().addYaw(2.19911486f);

        this->last_tick_end_time = std::chrono::steady_clock::now();
        this->last_tick_duration = std::chrono::duration<float> {0.0};
    }

    Game::~Game() = default;

    float Game::getTickDeltaTimeSeconds() const
    {
        return this->last_tick_duration.load(std::memory_order_acquire).count();
    }

    bool Game::continueTicking() // NOLINT: will change
    {
        return true;
    }

    void Game::tick()
    {
        std::vector<std::shared_ptr<const entity::Entity>> strongEntities;
        std::vector<std::future<void>> strongEntityTickFutures;

        for (const auto& [id, weakEntity] : this->entities.access())
        {
            if (std::shared_ptr<const entity::Entity> obj = weakEntity.lock())
            {
                strongEntityTickFutures.push_back(std::async(
                    std::launch::async,
                    [entity = obj.get()]
                    {
                        entity->tick();
                    }));

                strongEntities.push_back(std::move(obj));
            }
        }

        this->player.tick();

        this->renderer.setCamera(this->player.getCamera());

        this->world.updateChunkState();

        strongEntityTickFutures.clear(); // await all futures

        std::chrono::time_point<std::chrono::steady_clock> thisFrameEndTime =
            std::chrono::steady_clock::now();

        this->last_tick_duration.store(
            thisFrameEndTime - this->last_tick_end_time,
            std::memory_order_release);

        this->last_tick_end_time = thisFrameEndTime;

        this->renderer.getMenuState().lock(
            [&](gfx::ImGuiMenu::State& state)
            {
                state.tps             = 1 / this->getTickDeltaTimeSeconds();
                state.player_position = this->player.getCamera().getPosition();
            });
    }

    void Game::registerEntity(
        const std::shared_ptr<const entity::Entity>& entity) const
    {
        this->entities.insert(std::make_pair(entity->getUUID(), entity));
    }

    void Game::removeEntity(util::UUID id) const
    {
        this->entities.remove(id);
    }

} // namespace game
