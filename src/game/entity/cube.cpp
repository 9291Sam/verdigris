#include "cube.hpp"
#include "game/entity/cube.hpp"
#include "game/game.hpp"
#include <gfx/object.hpp>
#include <gfx/renderer.hpp>
#include <gfx/vulkan/gpu_structures.hpp>

namespace
{
    static constexpr std::array<gfx::vulkan::Vertex, 8> Vertices {
        gfx::vulkan::Vertex {
            .position {-1.0f, -1.0f, -1.0f},
            .color {0.0f, 0.0f, 0.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {-1.0f, -1.0f, 1.0f},
            .color {0.0f, 0.0f, 1.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {-1.0f, 1.0f, -1.0f},
            .color {0.0f, 1.0f, 0.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {-1.0f, 1.0f, 1.0f},
            .color {0.0f, 1.0f, 1.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {1.0f, -1.0f, -1.0f},
            .color {1.0f, 0.0f, 0.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {1.0f, -1.0f, 1.0f},
            .color {1.0f, 0.0f, 1.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {1.0f, 1.0f, -1.0f},
            .color {1.0f, 1.0f, 0.0f, 1.0f},
            .normal {},
            .uv {},
        },
        gfx::vulkan::Vertex {
            .position {1.0f, 1.0f, 1.0f},
            .color {1.0f, 1.0f, 1.0f, 1.0f},
            .normal {},
            .uv {},
        },
    };

    static constexpr std::array<gfx::vulkan::Index, 36> Indices {
        6, 2, 7, 2, 3, 7, 0, 4, 5, 1, 0, 5, 0, 2, 6, 4, 0, 6,
        3, 1, 7, 1, 5, 7, 2, 0, 3, 0, 1, 3, 4, 6, 7, 5, 4, 7};
} // namespace

auto game::entity::Cube::create(const Game& game, glm::vec3 position)
    -> std::shared_ptr<Cube>
{
    std::shared_ptr<Cube> cube {new Cube(game, position)};

    cube->registerSelf();

    return cube;
}

game::entity::Cube::Cube(const Game& game_, glm::vec3 position)
    : Entity {game_}
    , object {gfx::SimpleTriangulatedObject::create(
          this->getRenderer(),
          std::vector<gfx::vulkan::Vertex> {Vertices.begin(), Vertices.end()},
          std::vector<gfx::vulkan::Index> {Indices.begin(), Indices.end()})}
{
    this->object->transform.lock(
        [=](gfx::Transform& t)
        {
            t.translation = position;
        });
}

void game::entity::Cube::tick() const
{
    this->object->transform.lock(
        [this](gfx::Transform& t)
        {
            t.yawBy(1.0f * this->game.getTickDeltaTimeSeconds());

            t.translation = {
                10.0f * std::sin(this->game.getTickDeltaTimeSeconds()),
                10.0f,
                0.0f};
        });
}
