#ifndef SRC_GFX_RENDERER_HPP
#define SRC_GFX_RENDERER_HPP

#include <memory>

namespace gfx
{
    class Window;

    namespace vulkan
    {
        class Instance;
    }

    class Renderer
    {
    public:

        Renderer();
        ~Renderer();

        Renderer(const Renderer&)             = delete;
        Renderer(Renderer&&)                  = delete;
        Renderer& operator= (const Renderer&) = delete;
        Renderer& operator= (Renderer&&)      = delete;

        bool continueTicking();
        void drawFrame();

    private:
        std::unique_ptr<Window> window;

        std::shared_ptr<vulkan::Instance> instance;
    };
} // namespace gfx

#endif