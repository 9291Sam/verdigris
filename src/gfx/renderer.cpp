#include "renderer.hpp"
#include "imgui_menu.hpp"
#include "object.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/image.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/pipelines.hpp"
#include "vulkan/render_pass.hpp"
#include "vulkan/swapchain.hpp"
#include "vulkan/voxel/compute_renderer.hpp"
#include <GLFW/glfw3.h>
#include <magic_enum_all.hpp>
#include <util/log.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace gfx
{

    Renderer::Renderer()
        : window {std::make_unique<Window>(
            std::map<gfx::Window::Action, gfx::Window::ActionInformation> {
                {gfx::Window::Action::PlayerMoveForward,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_W},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerMoveBackward,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_S},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerMoveLeft,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_A},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerMoveRight,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_D},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerMoveUp,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_SPACE},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerMoveDown,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_LEFT_CONTROL},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::PlayerSprint,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_LEFT_SHIFT},
                     .method {gfx::Window::InteractionMethod::EveryFrame}}},

                {gfx::Window::Action::ToggleConsole,
                 gfx::Window::ActionInformation {
                     .key {GLFW_KEY_GRAVE_ACCENT},
                     .method {gfx::Window::InteractionMethod::SinglePress}}},

            },
            vk::Extent2D {1920, 1080}, // NOLINT
            "Verdigris")}
        , instance {std::make_unique<vulkan::Instance>()}
        , surface {std::make_unique<vk::UniqueSurfaceKHR>(
              this->window->createSurface(**this->instance))}
        , device {std::make_unique<vulkan::Device>(
              **this->instance, **this->surface)}
        , allocator {std::make_unique<vulkan::Allocator>(
              *this->instance, *this->device)}
        , menu_state {ImGuiMenu::State {}}
        , draw_camera {Camera {{0.0f, 0.0f, 0.0f}}}
        , show_menu {false}
    {
        this->initializeRenderer();
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
        auto updateAfterFrameFunc = [&]
        {
            if (this->window->isActionActive(
                    Window::Action::ToggleConsole, true))
            {
                this->show_menu = !this->show_menu;

                if (this->show_menu)
                {
                    this->window->detachCursor();
                }
                else
                {
                    this->window->attachCursor();
                }
            }

            this->menu_state.lock(
                [&](ImGuiMenu::State& state)
                {
                    state.fps = 1 / this->getFrameDeltaTimeSeconds();
                });
        };

        // collect objects and draw the frame TODO: move to isolate thread
        {
            vulkan::Frame& currentFrame = this->render_pass->getNextFrame();

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

            std::vector<std::shared_ptr<const Object>> strongObjects;
            std::vector<const Object*>                 rawObjects;

            for (const auto& [id, weakRenderable] : this->draw_objects.access())
            {
                if (std::shared_ptr<const Object> obj = weakRenderable.lock())
                {
                    obj->updateFrameState();

                    if (obj->shouldDraw())
                    {
                        rawObjects.push_back(obj.get());
                        strongObjects.push_back(std::move(obj));
                    }
                }
            }

            menuRender.get();

            if (!currentFrame.render(
                    this->draw_camera,
                    rawObjects,
                    this->show_menu ? this->menu.get() : nullptr,
                    updateAfterFrameFunc))
            {
                this->resize();
            }
        }

        this->window->endFrame();
    }

    void Renderer::resize()
    {
        this->window->blockThisThreadWhileMinimized();
        this->device->asLogicalDevice().waitIdle(); // stall TODO: make better?

        this->menu.reset();
        this->voxel_renderer.reset();
        this->pipelines.reset();
        this->render_pass.reset();
        this->depth_buffer.reset();
        this->swapchain.reset();

        this->initializeRenderer();
    }

    void Renderer::initializeRenderer()
    {
        this->swapchain = std::make_unique<vulkan::Swapchain>(
            *this->device, **this->surface, this->window->getFramebufferSize());

        this->depth_buffer = std::make_unique<vulkan::Image2D>(
            &*this->allocator,
            this->device->asLogicalDevice(),
            this->swapchain->getExtent(),
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageAspectFlagBits::eDepth,
            vk::ImageTiling::eOptimal,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

        this->render_pass = std::make_unique<vulkan::RenderPass>(
            this->device.get(), this->swapchain.get(), *this->depth_buffer);

        this->pipelines = std::make_unique<vulkan::PipelineManager>(
            this->device->asLogicalDevice(),
            **this->render_pass,
            this->swapchain.get(),
            this->allocator.get());

        {
            std::vector<std::future<void>> pipelineFutures {};

            magic_enum::enum_for_each<vulkan::GraphicsPipelineType>(
                [&](vulkan::GraphicsPipelineType type)
                {
                    if (type == vulkan::GraphicsPipelineType::NoPipeline)
                    {
                        return;
                    }

                    pipelineFutures.push_back(std::async(
                        std::launch::async,
                        [this, type]
                        {
                            std::ignore =
                                this->pipelines->getGraphicsPipeline(type);
                        }));
                });
        }

        this->voxel_renderer = std::make_unique<vulkan::voxel::ComputeRenderer>(
            this->device.get(),
            this->allocator.get(),
            this->pipelines.get(),
            vk::Extent2D {256, 256});

        this->menu = std::make_unique<ImGuiMenu>(
            *this->window,
            **this->instance,
            *this->device,
            **this->render_pass);

        this->menu->bindImage(this->voxel_renderer->getImage());
    }

    void Renderer::registerObject(
        const std::shared_ptr<const Object>& objectToRegister) const
    {
        this->draw_objects.insert(
            {objectToRegister->getUUID(), std::weak_ptr {objectToRegister}});
    }
} // namespace gfx