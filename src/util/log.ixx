module;

#include <concurrentqueue.h>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <latch>
#include <source_location>
#include <thread>

export module util.log;

namespace util
{
    export enum class LoggingLevel : std::uint8_t
    {
        Trace = 0,
        Debug = 1,
        Log   = 2,
        Warn  = 3,
        Fatal = 4
    };

    export class Logger
    {
    public:

        Logger();
        ~Logger();

        void send(std::string string);

    private:
        std::unique_ptr<std::atomic<bool>> should_thread_close {};
        std::thread                        worker_thread;
        std::unique_ptr<moodycamel::ConcurrentQueue<std::string>> message_queue;
    };

    export void installGlobalLoggerRacy(std::unique_ptr<Logger>);
    export std::unique_ptr<Logger> removeGlobalLoggerRacy();

    void asynchronouslyLog(
        std::string                           message,
        LoggingLevel                          level,
        std::source_location                  location,
        std::chrono::system_clock::time_point time);
    export LoggingLevel getCurrentLevel();
    export void         setLoggingLevel(LoggingLevel);

#define MAKE_LOGGER(LEVEL)                                                     \
    export template<class... Ts>                                               \
    struct log##LEVEL                                                          \
    {                                                                          \
        log##LEVEL(                                                            \
            fmt::format_string<Ts...> fmt,                                     \
            Ts&&... args,                                                      \
            const std::source_location& location =                             \
                std::source_location::current())                               \
        {                                                                      \
            using enum LoggingLevel;                                           \
            if (std::to_underlying(getCurrentLevel())                          \
                <= std::to_underlying(LEVEL))                                  \
            {                                                                  \
                asynchronouslyLog(                                             \
                    fmt::format(fmt, std::forward<Ts>(args)...),               \
                    LEVEL,                                                     \
                    location,                                                  \
                    std::chrono::system_clock::now());                         \
            }                                                                  \
        }                                                                      \
    };                                                                         \
    template<class... Ts>                                                      \
    log##LEVEL(fmt::format_string<Ts...>, Ts&&...)->log##LEVEL<Ts...>;

    MAKE_LOGGER(Trace)
    MAKE_LOGGER(Debug)
    MAKE_LOGGER(Log)
    MAKE_LOGGER(Warn)
    MAKE_LOGGER(Fatal)

} // namespace util
