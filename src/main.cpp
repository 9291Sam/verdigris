#include <future>
#include <gfx/renderer.hpp>
#include <util/log.hpp>

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

        renderer.drawFrame();

        util::logLog(
            "{}",
            std::async(
                []
                {
                    return 3;
                })
                .get());
    }
    catch (const std::exception& e)
    {
        util::logTrace("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}