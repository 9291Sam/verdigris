#include "renderer.hpp"
#include "imgui_menu.hpp"
#include "recordable.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
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
        , render_pass {nullptr}
        , menu_state {ImGuiMenu::State {}}
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

    const util::Mutex<ImGuiMenu::State>& Renderer::getMenuState() const
    {
        return this->menu_state;
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
        // TODO: integrate the menu as a Renderable
        {
            auto [currentFrame, previousFence] = this->render_pass.readLock(
                [](const std::unique_ptr<vulkan::RenderPass>& renderPass)
                {
                    return renderPass->getNextFrame();
                });

            // TODO: move to a draw object.
            std::future<void> menuRender = std::async(
                std::launch::async,
                [&]
                {
                    if (this->show_menu)
                    {
                        this->menu_state.lock(
                            [&](ImGuiMenu::State& state)
                            {
                                this->menu->render(state);
                            });
                    }
                });

            std::vector<std::shared_ptr<const Recordable>> strongRecordables;
            std::vector<const Recordable*>                 rawRecordables;

            for (const auto& [id, weakRenderable] : this->draw_objects.access())
            {
                if (std::shared_ptr<const Recordable> obj =
                        weakRenderable.lock())
                {
                    obj->updateFrameState();

                    if (obj->shouldDraw())
                    {
                        rawRecordables.push_back(obj.get());
                        strongRecordables.push_back(std::move(obj));
                    }
                }
            }

            menuRender.get();

            if (!currentFrame.render(
                    this->draw_camera,
                    rawRecordables,
                    this->show_menu ? this->menu.get() : nullptr,
                    previousFence))
            {
                this->resize();
            }

            if (this->window->isActionActive(
                    Window::Action::ToggleConsole, true))
            {
                this->show_menu = !this->show_menu;
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

            this->menu_state.lock(
                [&](ImGuiMenu::State& state)
                {
                    state.fps = 1 / this->getFrameDeltaTimeSeconds();
                });
        }

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

        if (maybeWriteLockedRenderPass.has_value())
        {
            **maybeWriteLockedRenderPass = std::make_unique<vulkan::RenderPass>(
                this->device.get(), this->swapchain.get(), *this->depth_buffer);

            this->menu = std::make_unique<ImGuiMenu>(
                *this->window,
                **this->instance,
                *this->device,
                ****maybeWriteLockedRenderPass); // lol
        }
        else
        {
            this->render_pass.writeLock(
                [&](std::unique_ptr<vulkan::RenderPass>& renderPass)
                {
                    renderPass = std::make_unique<vulkan::RenderPass>(
                        this->device.get(),
                        this->swapchain.get(),
                        *this->depth_buffer);

                    this->menu = std::make_unique<ImGuiMenu>(
                        *this->window,
                        **this->instance,
                        *this->device,
                        **renderPass);
                });
        }

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
