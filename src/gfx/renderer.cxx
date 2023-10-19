module;

#include <array>
#include <print>

export module gfx.renderer;

export namespace gfx
{
    void foo()
    {
        std::array<int, 8> asdf {};

        std::println("Foo!");
    }
} // namespace gfx
