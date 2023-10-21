#ifndef SRC_UTIL_LOG_HPP
#define SRC_UTIL_LOG_HPP

#include <concurrentqueue.h>
#include <cstdint>
#include <fmt/core.h>
#include <source_location>

namespace util
{
    enum class LoggingLevel : std::uint8_t
    {
        Trace = 0,
        Debug = 1,
        Log   = 2,
        Warn  = 3,
        Fatal = 4
    };

    void installGlobalLoggerRacy();
    void removeGlobalLoggerRacy();

    void         setLoggingLevel(LoggingLevel);
    LoggingLevel getCurrentLevel();

    void asynchronouslyLog(
        std::string                           message,
        LoggingLevel                          level,
        std::source_location                  location,
        std::chrono::system_clock::time_point time);

#define MAKE_LOGGER(LEVEL)                                                     \
    template<class... Ts>                                                      \
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

#endif // SRC_UTIL_LOG_HPP