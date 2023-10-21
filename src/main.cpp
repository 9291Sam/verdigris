#include <gfx/renderer.hpp>
#include <util/log.hpp>
#include <util/thread_pool.hpp>

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

        auto [sender, receiver] = util::createChannel<int>();

        sender.send(3);

        util::ThreadPool pool {std::thread::hardware_concurrency()};

        std::async();

        util::logLog(
            "{}",
            pool.run<int>(
                    []
                    {
                        return 3;
                    })
                .get());

        throw std::runtime_error {
            std::format("{}", receiver.receive().value())};
    }
    catch (const std::exception& e)
    {
        util::logTrace("Verdigris crash | {}", e.what());
    }

    util::removeGlobalLoggerRacy();
}