#include "renderer.hpp"
#include "imgui_menu.hpp"
#include "recordables/recordable.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/frame.hpp"
#include "vulkan/image.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/pipelines.hpp"
#include "vulkan/render_pass.hpp"
#include "vulkan/swapchain.hpp"
#include <GLFW/glfw3.h>
#include <magic_enum_all.hpp>
#include <util/log.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace gfx
{
    using Action            = gfx::Window::Action;
    using InteractionMethod = gfx::Window::InteractionMethod;
    using ActionInformation = gfx::Window::ActionInformation;

    Renderer::Renderer()
        : window {std::make_unique<Window>(
            std::map<gfx::Window::Action, gfx::Window::ActionInformation> {
                {Action::PlayerMoveForward,
                 ActionInformation {
                     .key {GLFW_KEY_W},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveBackward,
                 ActionInformation {
                     .key {GLFW_KEY_S},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveLeft,
                 ActionInformation {
                     .key {GLFW_KEY_A},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveRight,
                 ActionInformation {
                     .key {GLFW_KEY_D},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveUp,
                 ActionInformation {
                     .key {GLFW_KEY_SPACE},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerMoveDown,
                 ActionInformation {
                     .key {GLFW_KEY_LEFT_CONTROL},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::PlayerSprint,
                 ActionInformation {
                     .key {GLFW_KEY_LEFT_SHIFT},
                     .method {InteractionMethod::EveryFrame}}},

                {Action::ToggleConsole,
                 ActionInformation {
                     .key {GLFW_KEY_GRAVE_ACCENT},
                     .method {InteractionMethod::SinglePress}}},

                {Action::ToggleCursorAttachment,
                 ActionInformation {
                     .key {GLFW_KEY_BACKSLASH},
                     .method {InteractionMethod::SinglePress}}},

            },
            vk::Extent2D {1920, 1080}, // NOLINT
            "Verdigris")}
        , instance {std::make_unique<vulkan::Instance>()}
        , surface {std::make_unique<vk::UniqueSurfaceKHR>(
              this->window->createSurface(**this->instance))}
        , device {std::make_unique<vulkan::Device>(
              **this->instance, **this->surface)}
        , allocator {std::make_unique<vulkan::Allocator>(
              *this->instance, &*this->device)}
        , swapchain {nullptr}
        , depth_buffer {nullptr}
        , voxel_discovery_pass {{nullptr}}
        , voxel_discovery_image {nullptr}
        , final_raster_pass {{nullptr}}
        , pipelines {nullptr}
        , debug_menu {nullptr}
        , draw_camera {Camera {{0.0f, 0.0f, 0.0f}}}
        , show_menu {false}
        , is_cursor_attached {true}
    {
        this->initializeRenderer(std::nullopt);

        this->window->attachCursor();
    }

    Renderer::~Renderer() = default;

    float Renderer::getFrameDeltaTimeSeconds() const
    {
        return this->window->getDeltaTimeSeconds();
    }

    Window::Delta Renderer::getMouseDeltaRadians() const
    {
        // each value from -1.0 -> 1.0 representing how much it moved on the
        // screen
        const auto [nDeltaX, nDeltaY] =
            this->window->getScreenSpaceMouseDelta();

        const auto deltaRadiansX = (nDeltaX / 2) * this->getFovXRadians();
        const auto deltaRadiansY = (nDeltaY / 2) * this->getFovYRadians();

        return Window::Delta {.x {deltaRadiansX}, .y {deltaRadiansY}};
    }

    const util::Mutex<recordables::DebugMenu::State>&
    Renderer::getMenuState() const
    {
        return this->debug_menu_state;
    }

    [[nodiscard]] bool Renderer::isActionActive(Window::Action a) const
    {
        return this->window->isActionActive(a);
    }

    float
    gfx::Renderer::getFovYRadians() const // NOLINT: may change in the future
    {
        return glm::radians(70.f); // NOLINT
    }

    float gfx::Renderer::getFovXRadians() const
    {
        // assume that the window is never stretched with an image.
        // i.e 100x pixels are 100y pixels if rotated
        const auto [width, height] = this->window->getFramebufferSize();

        return this->getFovYRadians() * static_cast<float>(width)
             / static_cast<float>(height);
    }

    // float gfx::Renderer::getFocalLength() const
    // {
    //     const auto [width, height] = this->window->getFramebufferSize();
    //     float fovYRadians          = this->getFovYRadians();

    //     // Calculate focal length using the field of view and image size
    //     float focalLength =
    //         static_cast<float>(width) / (2.0f * std::tan(fovYRadians
    //         / 2.0f));

    //     return focalLength;
    // }
    float gfx::Renderer::getFocalLength() const
    {
        // const auto [width, height] = this->window->getFramebufferSize();
        // float fovYRadians          = this->getFovYRadians();

        // // Assuming you're using glm::perspective for rasterization
        // float focalLength = 1.0f / std::tan(fovYRadians / 2.0f);

        // return focalLength;
        return 1.0f;
    }

    float gfx::Renderer::getAspectRatio() const
    {
        const auto [width, height] = this->window->getFramebufferSize();

        return static_cast<float>(width) / static_cast<float>(height);
    }

    void Renderer::setCamera(Camera c)
    {
        this->draw_camera = c;
    }

    bool Renderer::continueTicking()
    {
        return !this->window->shouldClose();
    }

    void Renderer::drawFrame()
    {
        std::vector<std::shared_ptr<const recordables::Recordable>>
            renderables = [&]
        {
            std::vector<util::UUID> weakRecordablesToRemove {};

            std::vector<std::shared_ptr<const recordables::Recordable>>
                                           strongMaybeDrawRenderables {};
            std::vector<std::future<void>> maybeDrawStateUpdateFutures {};

            // Collect the strong
            for (const auto& [weakUUID, weakRecordable] :
                 this->recordable_registry.access())
            {
                if (std::shared_ptr<const recordables::Recordable> recordable =
                        weakRecordable.lock())
                {
                    maybeDrawStateUpdateFutures.push_back(std::async(
                        [rawRecordable = recordable.get()]
                        {
                            rawRecordable->updateFrameState();
                        }));
                    strongMaybeDrawRenderables.push_back(std::move(recordable));
                }
                else
                {
                    weakRecordablesToRemove.push_back(weakUUID);
                }
            }

            // Purge the weak
            for (const util::UUID& weakID : weakRecordablesToRemove)
            {
                this->recordable_registry.remove(weakID);
            }

            std::vector<std::shared_ptr<const recordables::Recordable>>
                strongDrawingRenderables {};

            maybeDrawStateUpdateFutures.clear(); // join all futures

            for (const std::shared_ptr<const recordables::Recordable>&
                     maybeDrawingRecordable : strongMaybeDrawRenderables)
            {
                if (maybeDrawingRecordable->shouldDraw())
                {
                    strongDrawingRenderables.push_back(
                        std::move(maybeDrawingRecordable));
                }
            }

            return strongDrawingRenderables;
        }();
        renderables.push_back(this->debug_menu);

        // get next framebuffer index
        // bind that to the last renderpass
        // renmder

        Frame& frameToDraw = {};

        frameToDraw.if (!currentFrame.render(
                            this->draw_camera,
                            rawRecordables,
                            this->show_menu ? this->menu.get() : nullptr,
                            previousFence))
        {
            this->resize();
        }

        if (this->window->isActionActive(Window::Action::ToggleConsole, true))
        {
            this->debug_menu->setVisibility(!this->debug_menu->shouldDraw());
        }

        if (this->window->isActionActive(
                Window::Action::ToggleCursorAttachment, true))
        {
            if (this->is_cursor_attached)
            {
                this->is_cursor_attached = false;
                this->window->detachCursor();
            }
            else
            {
                this->is_cursor_attached = true;
                this->window->attachCursor();
            }
        }

        this->debug_menu_state.lock(
            [&](recordables::DebugMenu::State& state)
            {
                state.fps = 1 / this->getFrameDeltaTimeSeconds();
            });

        this->window->endFrame();
    }

    void Renderer::waitIdle()
    {
        this->device->asLogicalDevice().waitIdle();
    }

    void Renderer::resize()
    {
        this->window->blockThisThreadWhileMinimized();
        this->device->asLogicalDevice().waitIdle(); // stall TODO: make better?

        this->render_pass.writeLock(
            [&](std::unique_ptr<vulkan::RenderPass>& renderPass)
            {
                this->menu.reset();
                // this->pipelines.reset();
                renderPass.reset();
                this->depth_buffer.reset();
                this->swapchain.reset();

                this->initializeRenderer(&renderPass);

                this->pipelines->updateRenderPass(**renderPass);
            });
    }

    void Renderer::initializeRenderer(
        std::optional<std::unique_ptr<vulkan::RenderPass>*>
            maybeWriteLockedRenderPass)
    {
        this->swapchain = std::make_unique<vulkan::Swapchain>(
            *this->device, **this->surface, this->window->getFramebufferSize());

        this->depth_buffer = std::make_unique<vulkan::Image2D>(
            &*this->allocator,
            this->device->asLogicalDevice(),
            this->swapchain->getExtent(),
            vk::Format::eD32Sfloat,
            vk::ImageLayout::eUndefined,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageAspectFlagBits::eDepth,
            vk::ImageTiling::eOptimal,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        static_assert(false);
        this->swapchain->getRenderTargets();

        // if (maybeWriteLockedRenderPass.has_value())
        // {
        //     **maybeWriteLockedRenderPass =
        //     std::make_unique<vulkan::RenderPass>(
        //         this->device.get(), this->swapchain.get(),
        //         *this->depth_buffer);

        //     this->menu = std::make_unique<ImGuiMenu>(
        //         *this->window,
        //         **this->instance,
        //         *this->device,
        //         ****maybeWriteLockedRenderPass); // lol
        // }
        // else
        // {
        //     this->render_pass.writeLock(
        //         [&](std::unique_ptr<vulkan::RenderPass>& renderPass)
        //         {
        //             renderPass = std::make_unique<vulkan::RenderPass>(
        //                 this->device.get(),
        //                 this->swapchain.get(),
        //                 *this->depth_buffer);

        //             this->menu = std::make_unique<ImGuiMenu>(
        //                 *this->window,
        //                 **this->instance,
        //                 *this->device,
        //                 **renderPass);
        //         });
        // }

        if (this->pipelines == nullptr)
        {
            this->pipelines = std::make_unique<vulkan::PipelineCache>();
        }
    }

    void Renderer::registerRecordable(
        const std::shared_ptr<const Recordable>& objectToRegister) const
    {
        this->draw_objects.insert(
            {objectToRegister->getUUID(), std::weak_ptr {objectToRegister}});
    }
} // namespace gfx
