#include "gfx/object.hpp"
#include <future>
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>
#include <util/log.hpp>
#include <util/noise.hpp>

bool verdigris_forceValidation = false;

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        std::puts(argv[i]);

        if (std::strcmp("--force-validation", argv[i]) == 0)
        {
            verdigris_forceValidation = true;

            std::puts("Validation Forced");
        }
    }

    util::installGlobalLoggerRacy();

#ifdef NDEBUG
    util::setLoggingLevel(util::LoggingLevel::Log);
#else
    util::setLoggingLevel(util::LoggingLevel::Trace); // TODO: commandline args
#endif

    try
    {
        gfx::Renderer renderer {};
        game::Game    game {renderer};

        std::atomic<bool> shouldStop {false};

        std::future<void> gameLoop = std::async(
            [&]
            {
                while (game.continueTicking()
                       && !shouldStop.load(std::memory_order_acquire))
                {
                    game.tick();
                }

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
        util::logFatal("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}