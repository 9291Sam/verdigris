#include "cube.hpp"
#include "game/entity/cube.hpp"
#include "game/game.hpp"
#include <gfx/recordables/flat_recordable.hpp>
#include <gfx/renderer.hpp>
#include <gfx/vulkan/gpu_structures.hpp>

namespace
{
    constexpr std::array<gfx::recordables::FlatRecordable::Vertex, 8> Vertices {
        gfx::recordables::FlatRecordable::Vertex {
            .color {0.0f, 0.0f, 0.0f, 1.0f},
            .position {-1.0f, -1.0f, -1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {0.0f, 0.0f, 1.0f, 1.0f},
            .position {-1.0f, -1.0f, 1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {0.0f, 1.0f, 0.0f, 1.0f},
            .position {-1.0f, 1.0f, -1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {0.0f, 1.0f, 1.0f, 1.0f},
            .position {-1.0f, 1.0f, 1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {1.0f, 0.0f, 0.0f, 1.0f},
            .position {1.0f, -1.0f, -1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {1.0f, 0.0f, 1.0f, 1.0f},
            .position {1.0f, -1.0f, 1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {1.0f, 1.0f, 0.0f, 1.0f},
            .position {1.0f, 1.0f, -1.0f},
            .normal {},
        },
        gfx::recordables::FlatRecordable::Vertex {
            .color {1.0f, 1.0f, 1.0f, 1.0f},
            .position {1.0f, 1.0f, 1.0f},
            .normal {},
        },
    };

    constexpr std::array<gfx::recordables::FlatRecordable::Index, 36> Indices {
        6, 2, 7, 2, 3, 7, 0, 4, 5, 1, 0, 5, 0, 2, 6, 4, 0, 6,
        3, 1, 7, 1, 5, 7, 2, 0, 3, 0, 1, 3, 4, 6, 7, 5, 4, 7};
} // namespace

namespace game::entity
{

    std::shared_ptr<Cube> Cube::create(const Game& game_, glm::vec3 position)
    {
        std::shared_ptr<Cube> cube {new Cube(game_, position)};

        cube->registerSelf();

        return cube;
    }

    Cube::Cube(const Game& game_, glm::vec3 position)
        : Entity {game_}
        , object {gfx::recordables::FlatRecordable::create(
              this->getRenderer(),
              std::vector<gfx::recordables::FlatRecordable::Vertex> {
                  Vertices.begin(), Vertices.end()},
              std::vector<gfx::recordables::FlatRecordable::Index> {
                  Indices.begin(), Indices.end()},
              gfx::Transform {.translation {position}},
              "Cube")}
        , root {position}
        , time_alive {0.0f}
    {
        this->object->transform.lock(
            [&](gfx::Transform& t)
            {
                t.translation = this->root;
            });
    }

    void Cube::tick() const
    {
        this->object->transform.lock(
            [this](gfx::Transform& t)
            {
                this->time_alive += this->game.getTickDeltaTimeSeconds();

                t.yawBy(1.0f * this->game.getTickDeltaTimeSeconds());

                t.translation = this->root
                              + (glm::vec3 {0.0f, 4.5f, 0.0f}
                                 * std::sin(this->time_alive));
            });
    }

} // namespace game::entity
