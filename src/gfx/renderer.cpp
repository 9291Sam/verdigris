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
              *this->instance, *this->device)}
        , menu_state {ImGuiMenu::State {}}
        , draw_camera {Camera {{0.0f, 0.0f, 0.0f}}}
        , show_menu {true}
        , is_cursor_attached {true}
    {
        this->initializeRenderer();

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
        // collect objects and draw the frame TODO: move to isolate thread
        {
            auto [currentFrame, previousFence] =
                this->render_pass->getNextFrame();

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

            std::future<void> voxelRender = std::async(
                std::launch::async,
                [&]
                {
                    this->voxel_renderer->tick();
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
            voxelRender.get();

            if (!currentFrame.render(
                    this->draw_camera,
                    rawObjects,
                    this->voxel_renderer.get(),
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
            vk::ImageLayout::eUndefined,
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

        // {
        //     std::vector<std::future<void>> pipelineFutures {};

        //     magic_enum::enum_for_each<vulkan::GraphicsPipelineType>(
        //         [&](vulkan::GraphicsPipelineType type)
        //         {
        //             if (type == vulkan::GraphicsPipelineType::NoPipeline)
        //             {
        //                 return;
        //             }

        //             pipelineFutures.push_back(std::async(
        //                 std::launch::async,
        //                 [this, type]
        //                 {
        //                     std::ignore =
        //                         this->pipelines->getGraphicsPipeline(type);
        //                 }));
        //         });

        //     magic_enum::enum_for_each<vulkan::ComputePipelineType>(
        //         [&](vulkan::ComputePipelineType type)
        //         {
        //             if (type == vulkan::ComputePipelineType::NoPipeline)
        //             {
        //                 return;
        //             }

        //             pipelineFutures.push_back(std::async(
        //                 std::launch::async,
        //                 [this, type]
        //                 {
        //                     std::ignore =
        //                         this->pipelines->getComputePipeline(type);
        //                 }));
        //         });
        // }

        this->voxel_renderer = std::make_unique<vulkan::voxel::ComputeRenderer>(
            *this,
            this->device.get(),
            this->allocator.get(),
            this->pipelines.get(),
            this->swapchain->getExtent());

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
