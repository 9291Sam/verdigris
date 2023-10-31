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
        class Allocator;
        class Swapchain;
        class Image2D;
        class RenderPass;
        class PipelineManager;
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
        void resize();
        void initializeRenderer();

        // Vulkan prelude objects
        std::unique_ptr<Window>               window;
        std::unique_ptr<vulkan::Instance>     instance;
        std::unique_ptr<vk::UniqueSurfaceKHR> surface;
        std::unique_ptr<vulkan::Device>       device;
        std::unique_ptr<vulkan::Allocator>    allocator;

        // Rendering objects
        std::unique_ptr<vulkan::Swapchain>       swapchain;
        std::unique_ptr<vulkan::Image2D>         depth_buffer;
        std::unique_ptr<vulkan::RenderPass>      render_pass;
        std::unique_ptr<vulkan::PipelineManager> pipelines;
    };
} // namespace gfx

#endif