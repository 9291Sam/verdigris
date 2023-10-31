#include "renderer.hpp"
#include "object.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/image.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/pipelines.hpp"
#include "vulkan/render_pass.hpp"
#include "vulkan/swapchain.hpp"
#include "window.hpp"
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
        , draw_camera {Camera {{0.0f, 0.0f, 0.0f}}}
    {
        this->initializeRenderer();
    }

    Renderer::~Renderer() = default;

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

    bool Renderer::continueTicking()
    {
        return !this->window->shouldClose();
    }

    void Renderer::drawFrame()
    {
        vulkan::Frame& currentFrame = this->render_pass->getNextFrame();

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

        if (!currentFrame.render(this->draw_camera, rawObjects))
        {
            this->resize();
        }

        this->window->endFrame();
    }

    void Renderer::resize()
    {
        this->window->blockThisThreadWhileMinimized();
        this->device->asLogicalDevice().waitIdle(); // stall

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

            magic_enum::enum_for_each<vulkan::PipelineType>(
                [&](vulkan::PipelineType type)
                {
                    if (type == vulkan::PipelineType::NoPipeline)
                    {
                        return;
                    }

                    pipelineFutures.push_back(std::async(
                        std::launch::async,
                        [this, type]
                        {
                            std::ignore = this->pipelines->getPipeline(type);
                        }));
                });
        }
    }

    void Renderer::registerObject(
        const std::shared_ptr<const Object>& objectToRegister) const
    {
        this->draw_objects.insert(
            {objectToRegister->getUUID(), std::weak_ptr {objectToRegister}});
    }
} // namespace gfx