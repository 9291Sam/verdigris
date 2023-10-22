#ifndef SRC_GFX_RENDERER_HPP
#define SRC_GFX_RENDERER_HPP

#include <memory>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Window;

    namespace vulkan
    {
        class Instance;
        class Device;
    } // namespace vulkan

    class Renderer
    {
    public:

        Renderer();
        ~Renderer();

        Renderer(const Renderer&)             = delete;
        Renderer(Renderer&&)                  = delete;
        Renderer& operator= (const Renderer&) = delete;
        Renderer& operator= (Renderer&&)      = delete;

        [[nodiscard]] bool continueTicking();
        void               drawFrame();

    private:
        std::unique_ptr<Window>               window;
        std::unique_ptr<vulkan::Instance>     instance;
        std::unique_ptr<vk::UniqueSurfaceKHR> surface;
        std::unique_ptr<vulkan::Device>       device;
    };
} // namespace gfx

#endif