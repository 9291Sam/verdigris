#include <util/log.hpp>

// NOLINTBEGIN
// Feature flags
#define NK_PRIVATE                      // ODR
#define NK_INCLUDE_FIXED_TYPES          // use fixed width integers
#define NK_INCLUDE_DEFAULT_ALLOCATOR    // Use malloc & free
#define NK_INCLUDE_STANDARD_IO          // Adds extra functions
#define NK_INCLUDE_STANDARD_VARARGS     // Adds extra functions
#define NK_INCLUDE_STANDARD_BOOL        // Turns int -> bool
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT // Adds vulkan support
#define NK_INCLUDE_FONT_BAKING          // Adds std_truetype font support
#define NK_INCLUDE_DEFAULT_FONT         // Sets ProggyClean.ttf as font
#define NK_INCLUDE_COMMAND_USERDATA     // Allows for userdata callbacks
#define NK_ZERO_COMMAND_MEMORY          // Optimization
// # define NK_UINT_DRAW_INDEX // Makes index type U32 instead of U16

// Configs
#define NK_BUFFER_DEFAULT_INITIAL_SIZE 4096
#define NK_INPUT_MAX                   64
#define NK_MAX_NUMBER_BUFFER           64

// Function overwrites
#define NK_ASSERT(...)                                                         \
    ::util::assertFatal(__VA_ARGS__, "nuklear assert failed!");
#define NK_VSNPRINTF(...)     ::std::vsnprintf(__VA_ARGS__);
#define NK_STRTOD(begin, end) ::std::strtod(begin, const_cast<char**>(end));

namespace
{
    char* verdigris_nk_dtoa_replacement(char* outputBuffer, double number)
    {
        int result =
            std::snprintf(outputBuffer, NK_MAX_NUMBER_BUFFER, "%f", number);

        util::assertFatal(
            result < NK_MAX_NUMBER_BUFFER - 2 && result >= 0,
            "verdigris_nk_dtoa_replacement raised error! | Result: {} | "
            "Number: {}",
            result,
            number);
    }
} // namespace

#define NK_DTOA(...) verdigris_nk_dtoa_replacement(__VA_ARGS__);
// NOLINTEND

#define NK_IMPLEMENTATION
#include <nuklear.h>

// size_t foo = ;
