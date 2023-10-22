#include "misc.hpp"
#include <cstdio>
#include <tuple>
#include <utility>

[[noreturn]] void util::debugBreak()
{
    std::ignore = std::fputs("util::debugBreak()\n", stderr);
#ifdef _MSC_VER
    __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#else
#error Unsupported compiler
#endif

    std::unreachable();
}
