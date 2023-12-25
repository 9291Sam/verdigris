#include "misc.hpp"
#include <cstdio>
#include <thread>
#include <tuple>

[[noreturn]] void util::debugBreak()
{
    std::ignore = std::fputs("util::debugBreak()\n", stderr);

    std::this_thread::sleep_for(std::chrono::milliseconds {100});

#ifdef _MSC_VER
    __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#else
#error Unsupported compiler
#endif
}
