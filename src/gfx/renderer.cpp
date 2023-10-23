#include "renderer.hpp"
#include "vulkan/allocator.hpp"
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <util/log.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace gfx
{

    Renderer::Renderer()
        : window {std::make_unique<Window>(
            []
            {
                std::map<gfx::Window::Action, gfx::Window::ActionInformation> workingMap;
                using enum gfx::Window::Action;
                using enum gfx::Window::InteractionMethod;
                using ActionInformation = gfx::Window::ActionInformation;

                workingMap[PlayerMoveForward] =
                    ActionInformation {.key {GLFW_KEY_W}, .method {EveryFrame}};
                workingMap[PlayerMoveBackward] =
                    ActionInformation {.key {GLFW_KEY_S}, .method {EveryFrame}};
                workingMap[PlayerMoveLeft] =
                    ActionInformation {.key {GLFW_KEY_A}, .method {EveryFrame}};
                workingMap[PlayerMoveRight] =
                    ActionInformation {.key {GLFW_KEY_D}, .method {EveryFrame}};
                workingMap[PlayerMoveUp] =
                    ActionInformation {.key {GLFW_KEY_SPACE}, .method {EveryFrame}};
                workingMap[PlayerMoveDown] =
                    ActionInformation {.key {GLFW_KEY_LEFT_CONTROL}, .method {EveryFrame}};
                workingMap[PlayerSprint] =
                    ActionInformation {.key {GLFW_KEY_LEFT_SHIFT}, .method {EveryFrame}};
                workingMap[ToggleConsole] =
                    ActionInformation {.key {GLFW_KEY_GRAVE_ACCENT}, .method {SinglePress}};

                return workingMap;
            }(),
            vk::Extent2D {1920, 1080}, // NOLINT
            "Verdigris")}
        , instance {std::make_unique<vulkan::Instance>()}
        , surface {std::make_unique<vk::UniqueSurfaceKHR>(
              this->window->createSurface(**this->instance))}
        , device {std::make_unique<vulkan::Device>(**this->instance, **this->surface)}
        , allocator {std::make_unique<vulkan::Allocator>(*this->instance, *this->device)}
    {}

    Renderer::~Renderer() = default;

    bool Renderer::continueTicking()
    {
        return !this->window->shouldClose();
    }

    void Renderer::drawFrame()
    {
        this->window->endFrame();
    }

} // namespace gfx