#include "gfx/object.hpp"
#include <future>
#include <gfx/renderer.hpp>
#include <util/log.hpp>

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

int main()
{
    util::installGlobalLoggerRacy();

#ifdef NDEBUG
    util::setLoggingLevel(util::LoggingLevel::Log);
#else
    util::setLoggingLevel(util::LoggingLevel::Trace);
#endif

    try
    {
        gfx::Renderer renderer {};

        std::atomic<bool> shouldStop {false};

        std::future<void> gameLoop = std::async(
            [&]
            {
                std::shared_ptr<gfx::SimpleTriangulatedObject> obj =
                    gfx::SimpleTriangulatedObject::create(
                        renderer,
                        std::vector<gfx::vulkan::Vertex> {
                            Vertices.begin(), Vertices.end()},
                        std::vector<gfx::vulkan::Index> {
                            Indices.begin(), Indices.end()});

                while (!shouldStop.load(std::memory_order_acquire))
                {}

                shouldStop.store(true, std::memory_order_release);
            });

        while (renderer.continueTicking()
               && !shouldStop.load(std::memory_order_acquire))
        {
            renderer.drawFrame();
        }

        shouldStop.store(true, std::memory_order_release);

        gameLoop.wait();
    }
    catch (const std::exception& e)
    {
        util::logTrace("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}