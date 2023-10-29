#include "renderer.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/image.hpp"
#include "vulkan/instance.hpp"
#include "vulkan/render_pass.hpp"
#include "vulkan/swapchain.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
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
    {
        this->initializeRenderer();
    }

    Renderer::~Renderer() = default;

    bool Renderer::continueTicking()
    {
        return !this->window->shouldClose();
    }

    void Renderer::drawFrame()
    {
        this->window->endFrame();
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
            this->device->asLogicalDevice(),
            *this->swapchain,
            *this->depth_buffer);
    }
} // namespace gfx