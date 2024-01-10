#ifndef SRC_GFX_RENDERER_HPP
#define SRC_GFX_RENDERER_HPP

#include "camera.hpp"
#include "recordables/debug_menu.hpp"
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

    namespace recordables
    {
        class DebugMenu;
        class Recordable;
    } // namespace recordables

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
        class PipelineCache;
        class GraphicsPipeline;
        class ComputePipeline;
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

        // TODO: use events for the menu.
        [[nodiscard]] const util::Mutex<recordables::DebugMenu::State>&
                                    getMenuState() const;
        [[nodiscard]] float         getFrameDeltaTimeSeconds() const;
        [[nodiscard]] Window::Delta getMouseDeltaRadians() const;

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
        void
        initializeRenderer(std::optional<std::unique_ptr<vulkan::RenderPass>*>
                               maybeWriteLockedRenderPass);

        friend recordables::Recordable;
        void registerRecordable(
            const std::shared_ptr<const recordables::Recordable>&) const;

        friend vulkan::GraphicsPipeline;
        friend vulkan::ComputePipeline;
        friend recordables::DebugMenu;
        [[nodiscard]] util::RwLock<std::unique_ptr<vulkan::RenderPass>>
            getRenderPass(DrawStage) const;

        // Vulkan prelude objects
        std::unique_ptr<Window>               window;
        std::unique_ptr<vulkan::Instance>     instance;
        std::unique_ptr<vk::UniqueSurfaceKHR> surface;
        std::unique_ptr<vulkan::Device>       device;
        std::unique_ptr<vulkan::Allocator>    allocator;

        // Pre-renderpass Rendering objects
        std::unique_ptr<vulkan::Swapchain> swapchain;
        std::unique_ptr<vulkan::Image2D>   depth_buffer;

        // Render passes
        util::RwLock<std::unique_ptr<vulkan::RenderPass>> voxel_discovery_pass;
        std::unique_ptr<vulkan::Image2D>                  voxel_discovery_image;
        util::RwLock<std::unique_ptr<vulkan::RenderPass>> final_raster_pass;

        // Post-renderpass Rendering Objects
        std::unique_ptr<vulkan::PipelineCache> pipelines;
        std::unique_ptr<vulkan::FrameManager>  frame_manager;

        // Objects
        std::shared_ptr<recordables::DebugMenu> debug_menu;
        util::
            Registrar<util::UUID, std::weak_ptr<const recordables::Recordable>>
                recordable_registry;

        // State
        util::Mutex<recordables::DebugMenu::State> debug_menu_state;
        std::atomic<Camera>                        draw_camera;
        bool                                       is_cursor_attached;
    };
} // namespace gfx

#endif // SRC_GFX_RENDERER_HPP
