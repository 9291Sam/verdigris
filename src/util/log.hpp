#ifndef SRC_UTIL_LOG_HPP
#define SRC_UTIL_LOG_HPP

#include "misc.hpp"
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
        Fatal = 4,
        Panic = 5,
    };
    const char* LoggingLevel_asString(LoggingLevel);

    void installGlobalLoggerRacy();
    void removeGlobalLoggerRacy();

    void         setLoggingLevel(LoggingLevel);
    LoggingLevel getCurrentLevel();

    void asynchronouslyLog(
        std::string                           message,
        LoggingLevel                          level,
        std::source_location                  location,
        std::chrono::system_clock::time_point time);

// TODO: see if theres a way to move that fmt stuff to the other thread, take a
// fmt string, validate it at compile time and then just memcpy the args over
// and construct later
#define MAKE_LOGGER(LEVEL) /* NOLINT */                                        \
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

    MAKE_LOGGER(Trace) // NOLINT: We want implicit conversions
    MAKE_LOGGER(Debug) // NOLINT: We want implicit conversions
    MAKE_LOGGER(Log)   // NOLINT: We want implicit conversions
    MAKE_LOGGER(Warn)  // NOLINT: We want implicit conversions
    MAKE_LOGGER(Fatal) // NOLINT: We want implicit conversions

#undef MAKE_LOGGER

#define MAKE_ASSERT(LEVEL, THROW_ON_FAIL) /* NOLINT */                         \
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
                       <= std::to_underlying(LEVEL)) [[unlikely]]              \
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
                    util::debugBreak();                                        \
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

    MAKE_ASSERT(Trace, false) // NOLINT: We want implicit conversions
    MAKE_ASSERT(Debug, false) // NOLINT: We want implicit conversions
    MAKE_ASSERT(Log, false)   // NOLINT: We want implicit conversions
    MAKE_ASSERT(Warn, false)  // NOLINT: We want implicit conversions
    MAKE_ASSERT(Fatal, true)  // NOLINT: We want implicit conversions

#undef MAKE_ASSERT

    template<class... Ts>
    struct panic // NOLINT: we want to lie and pretend this is a function
    {
        panic( // NOLINT
            fmt::format_string<Ts...> fmt,
            Ts&&... args,
            const std::source_location& location =
                std::source_location::current())
        {
            using enum LoggingLevel;
            using namespace std::chrono_literals;
            std::string message = fmt::format(fmt, std::forward<Ts>(args)...);

            asynchronouslyLog(
                message,
                LoggingLevel::Panic,
                location,
                std::chrono::system_clock::now());

            util::debugBreak();

            throw std::runtime_error {message};
        }
    };
    template<class... J>
    panic(fmt::format_string<J...>, J&&...) -> panic<J...>;

} // namespace util

#endif // SRC_UTIL_LOG_HPP
