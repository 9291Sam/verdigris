#ifndef SRC_GFX_RENDERER_HPP
#define SRC_GFX_RENDERER_HPP

#include "camera.hpp"
#include "imgui_menu.hpp"
#include "window.hpp"
#include <memory>
#include <util/registrar.hpp>
#include <util/threads.hpp>
#include <util/uuid.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Window;
    class Object;

    namespace vulkan
    {
        namespace voxel
        {
            class ComputeRenderer;
        } // namespace voxel

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

        [[nodiscard]] const util::Mutex<ImGuiMenu::State>& getMenuState() const;
        [[nodiscard]] float         getFrameDeltaTimeSeconds() const;
        [[nodiscard]] Window::Delta getMouseDeltaRadians() const;
        [[nodiscard]] Camera        getCamera() const;

        [[nodiscard]] bool  isActionActive(Window::Action) const;
        [[nodiscard]] float getFovYRadians() const;
        [[nodiscard]] float getFovXRadians() const;
        [[nodiscard]] float getFocalLength() const;
        [[nodiscard]] float getAspectRatio() const;

        void               setCamera(Camera);
        [[nodiscard]] bool continueTicking();
        void               drawFrame();
        void               waitIdle();

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

        // Drawing things
        std::unique_ptr<vulkan::voxel::ComputeRenderer> voxel_renderer;
        std::unique_ptr<ImGuiMenu>                      menu;

        // Rasterizeables
        util::Registrar<util::UUID, std::weak_ptr<const Object>> draw_objects;

        // State
        util::Mutex<ImGuiMenu::State> menu_state;
        std::atomic<Camera>           draw_camera;
        bool                          show_menu;
        bool                          is_cursor_attached;
        // things to draw
        // std::unique_ptr<VoxelComputeRenderer> voxel_renderer;
        // poke into menu and get the image
        // start with rendering a constant color image
        // then try to do a simple ray trace of a fixed size like 1024
    };
} // namespace gfx

#endif // SRC_GFX_RENDERER_HPP
