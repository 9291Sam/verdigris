export module util.misc;

namespace util
{
    export [[noreturn]] void debugBreak()
    {
#ifdef _MSC_VER
        __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_trap();
#else
#error Unsupported compiler
#endif
    }
} // namespace util
