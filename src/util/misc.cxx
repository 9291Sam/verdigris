module;

#include <cstdio>

module util.misc;

[[noreturn]] void util::debugBreak()
{
    std::fputs("util::debugBreak()\n", stderr);
#ifdef _MSC_VER
    __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#else
#error Unsupported compiler
#endif
}
