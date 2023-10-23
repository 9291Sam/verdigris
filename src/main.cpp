#include <future>
#include <gfx/renderer.hpp>
#include <util/log.hpp>
#include <util/vector.hpp>

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
        constexpr util::Vec3 vec {1.0f, 2.0f, 3.0f};

        gfx::Renderer renderer {};

        std::atomic<bool> shouldStop {false};

        std::future<void> gameLoop = std::async(
            [&]
            {
                while (!shouldStop.load(std::memory_order_acquire))
                {}

                shouldStop.store(true, std::memory_order_release);
            });

        while (renderer.continueTicking() && !shouldStop.load(std::memory_order_acquire))
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