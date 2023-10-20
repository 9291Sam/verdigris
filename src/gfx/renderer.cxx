module;

import util.log;

#include <array>
#include <print>

export module gfx.renderer;

namespace gfx
{
    export class Renderer
    {
    public:

        Renderer();
        ~Renderer();

        Renderer(const Renderer&)             = delete;
        Renderer(Renderer&&)                  = delete;
        Renderer& operator= (const Renderer&) = delete;
        Renderer& operator= (Renderer&&)      = delete;

        void drawFrame();

    private:
    };
} // namespace gfx

gfx::Renderer::Renderer() {}
gfx::Renderer::~Renderer() {}

void gfx::Renderer::drawFrame()
{
    util::logLog("drawFrame()");
}