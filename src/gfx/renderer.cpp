#include "renderer.hpp"
#include "GLFW/glfw3.h"
#include "window.hpp"
#include <util/log.hpp>

namespace
{

    const std::
        unordered_map<gfx::Window::Action, gfx::Window::ActionInformation>
            keys = []
    {
        std::unordered_map<gfx::Window::Action, gfx::Window::ActionInformation>
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

gfx::Renderer::Renderer()
    : window {
        std::make_unique<Window>(keys, vk::Extent2D(1920, 1080), "Verdigris")}
{}

gfx::Renderer::~Renderer() {}

void gfx::Renderer::drawFrame()
{
    util::logLog("drawFrame()");
}