
#include "game/world/world.hpp"
#include "game/world/sparse_volume.hpp"
#include <game/game.hpp>
#include <gfx/object.hpp>
#include <gfx/renderer.hpp>
#include <util/misc.hpp>
#include <util/noise.hpp>

namespace
{
    constexpr std::array<gfx::vulkan::Vertex, 8> CubeVertices {
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

    constexpr std::array<gfx::vulkan::Index, 36> CubeIndicies {
        6, 2, 7, 2, 3, 7, 0, 4, 5, 1, 0, 5, 0, 2, 6, 4, 0, 6,
        3, 1, 7, 1, 5, 7, 2, 0, 3, 0, 1, 3, 4, 6, 7, 5, 4, 7};
} // namespace

namespace game::world
{

    World::World(const Game& game_)
        : game {game_}
    {
        // std::int32_t radius = 0;

        // for (std::int32_t x = -radius; x <= radius; x++)
        // {
        //     for (std::int32_t z = -radius; z <= radius; z++)
        //     {
        //         std::int32_t chunkX = static_cast<std::int32_t>(x * 512);
        //         std::int32_t chunkZ = static_cast<std::int32_t>(z * 512);

        //         this->chunks.insert(Chunk {
        //             Position {chunkX - x, 0, chunkZ - z}, generationFunc});
        //     }
        // }

        std::vector<gfx::vulkan::ParallaxVertex> vertices {};

        for (const gfx::vulkan::Index& i : CubeIndicies)
        {
            const gfx::vulkan::Vertex& v = CubeVertices.at(i);

            vertices.push_back(gfx::vulkan::ParallaxVertex {
                .position {v.position}, .brick_pointer {~0U}});
        }

        this->obj = gfx::ParallaxRaymarchedVoxelObject::create(
            this->game.renderer, std::move(vertices));

        this->obj->transform.lock(
            [](gfx::Transform& t)
            {
                t.translation = {15, 15, 15};
            });
    }

    void World::tick()
    {
        static float sum = 0.0f;
        sum += this->game.getTickDeltaTimeSeconds();

        this->obj->transform.lock(
            [](gfx::Transform& t)
            {
                t.translation = {5, std::sin(sum) * 5, 5};
            });
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
