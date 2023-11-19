#include "gfx/object.hpp"
#include <future>
#include <game/game.hpp>
#include <gfx/renderer.hpp>
#include <util/log.hpp>
#include <util/noise.hpp>

int main()
{
    util::installGlobalLoggerRacy();

#ifdef NDEBUG
    util::setLoggingLevel(util::LoggingLevel::Log);
#else
    util::setLoggingLevel(util::LoggingLevel::Debug); // TODO: commandline args
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

        // util::logTrace
    }
    catch (const std::exception& e)
    {
        util::logFatal("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}