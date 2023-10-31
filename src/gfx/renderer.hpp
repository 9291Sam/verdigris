#ifndef SRC_GFX_RENDERER_HPP
#define SRC_GFX_RENDERER_HPP

#include "camera.hpp"
#include <memory>
#include <util/registrar.hpp>
#include <util/uuid.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Window;
    class Object;

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

        [[nodiscard]] float getFovYRadians() const;
        [[nodiscard]] float getFovXRadians() const;
        [[nodiscard]] float getAspectRatio() const;

        [[nodiscard]] bool continueTicking();
        void               drawFrame();

    private:
        void resize();
        void initializeRenderer();

        friend Object;
        void registerObject(const std::shared_ptr<const Object>&) const;

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

        // Renderer state
        util::Registrar<util::UUID, std::weak_ptr<const Object>> draw_objects;

        mutable std::atomic<Camera> draw_camera;
    };
} // namespace gfx

#endif