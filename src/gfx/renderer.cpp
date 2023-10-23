#include "renderer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/instance.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <util/log.hpp>
#include <vulkan/vulkan.hpp>

namespace gfx
{
    namespace
    {
        static /* NOLINT */ std::
            map<gfx::Window::Action, gfx::Window::ActionInformation>
                keys = [] // NOLINT
        {
            std::map<gfx::Window::Action, gfx::Window::ActionInformation>
                workingMap;
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
            workingMap[PlayerMoveDown] = ActionInformation {
                .key {GLFW_KEY_LEFT_CONTROL}, .method {EveryFrame}};
            workingMap[PlayerSprint] = ActionInformation {
                .key {GLFW_KEY_LEFT_SHIFT}, .method {EveryFrame}};
            workingMap[ToggleConsole] = ActionInformation {
                .key {GLFW_KEY_GRAVE_ACCENT}, .method {SinglePress}};

            return workingMap;
        }();

    } // namespace

    Renderer::Renderer()
        : window {std::make_unique<Window>(
            keys, vk::Extent2D {1920, 1080}, "Verdigris")} // NOLINT
        , instance {std::make_unique<vulkan::Instance>()}
        , surface {std::make_unique<vk::UniqueSurfaceKHR>(
              this->window->createSurface(**this->instance))}
        , device {std::make_unique<vulkan::Device>(
              **this->instance, **this->surface)}
        , allocator {}
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