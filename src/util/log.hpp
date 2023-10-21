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

#undef MAKE_LOGGER

#define MAKE_ASSERT(LEVEL, THROW_ON_FAIL)                                      \
    template<class... Ts>                                                      \
    struct assert##LEVEL                                                       \
    {                                                                          \
        assert##LEVEL(                                                         \
            bool                      condition,                               \
            fmt::format_string<Ts...> fmt,                                     \
            Ts&&... args,                                                      \
            const std::source_location& location =                             \
                std::source_location::current())                               \
        {                                                                      \
            using enum LoggingLevel;                                           \
            if (!condition                                                     \
                && std::to_underlying(getCurrentLevel())                       \
                       <= std::to_underlying(LEVEL))                           \
            {                                                                  \
                if constexpr (THROW_ON_FAIL)                                   \
                {                                                              \
                    std::string message =                                      \
                        fmt::format(fmt, std::forward<Ts>(args)...);           \
                                                                               \
                    asynchronouslyLog(                                         \
                        message,                                               \
                        LEVEL,                                                 \
                        location,                                              \
                        std::chrono::system_clock::now());                     \
                                                                               \
                    throw std::runtime_error {std::move(message)};             \
                }                                                              \
                else                                                           \
                {                                                              \
                    asynchronouslyLog(                                         \
                        fmt::format(fmt, std::forward<Ts>(args)...),           \
                        LEVEL,                                                 \
                        location,                                              \
                        std::chrono::system_clock::now());                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
    };                                                                         \
    template<class... J>                                                       \
    assert##LEVEL(bool, fmt::format_string<J...>, J&&...)                      \
        ->assert##LEVEL<J...>;

    MAKE_ASSERT(Trace, false)
    MAKE_ASSERT(Debug, false)
    MAKE_ASSERT(Log, false)
    MAKE_ASSERT(Warn, false)
    MAKE_ASSERT(Fatal, true)

#undef MAKE_ASSERT

    template<class... Ts>
    struct panic
    {
        panic(
            fmt::format_string<Ts...> fmt,
            Ts&&... args,
            const std::source_location& location =
                std::source_location::current())
        {
            using enum LoggingLevel;
            std::string message = fmt::format(fmt, std::forward<Ts>(args)...);

            asynchronouslyLog(
                message,
                LoggingLevel::Fatal,
                location,
                std::chrono::system_clock::now());

            throw std::runtime_error {std::move(message)};
        }
    };
    template<class... J>
    panic(fmt::format_string<J...>, J&&...) -> panic<J...>;

} // namespace util

#endif // SRC_UTIL_LOG_HPP