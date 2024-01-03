#include "window.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_structs.hpp"
#include <atomic>
#include <chrono>
#include <magic_enum_all.hpp>
#include <thread>
#include <util/log.hpp>
#include <vulkan/vulkan.hpp>

// NOLINTBEGIN clang-format off

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#define VMA_IMPLEMENTATION           1
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"
#undef VMA_IMPLEMENTATION
#undef VMA_STATIC_VULKAN_FUNCTIONS
#undef VMA_DYNAMIC_VULKAN_FUNCTIONS

#pragma clang diagnostic pop

// NOLINTEND clang-format on

// refacator goals
// make more readable
// remove ignore frames

namespace gfx
{
    Window::Window(
        const std::map<Action, ActionInformation>& keyInformationMap,
        vk::Extent2D                               size,
        const char*                                name)
        : window {nullptr}
        , framebuffer_size {size}
        , key_to_actions_map {}     // NOLINT
        , action_active_map {}      // NOLINT
        , action_interaction_map {} // NOLINT
        , last_frame_end_time {std::chrono::steady_clock::now()}
        , last_frame_duration {std::chrono::milliseconds {16}} // NOLINT
        , mouse_ignore_frames {3}
        , previous_mouse_position {{0.0f, 0.0f}}
        , mouse_delta_pixels {{0.0f, 0.0f}}
        , screen_space_mouse_delta {{0.0f, 0.0f}}
        , is_cursor_attached {true}
    {
        // Initialize GLFW
        {
            util::assertFatal(
                glfwSetErrorCallback(Window::errorCallback) == nullptr,
                "Error callback was set multiple times! Multiple windows were "
                "spawned!");

            util::assertFatal(
                glfwInit() == GLFW_TRUE, "Failed to initialize GLFW!");
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }

        // Spawn Window
        {
            this->window = glfwCreateWindow(
                static_cast<int>(size.width),
                static_cast<int>(size.height),
                name,
                nullptr,
                nullptr);

            util::assertFatal(
                this->window != nullptr, "Failed to create GLFW window!");

            // move the window to the corner so I can see my debug logs
            glfwSetWindowPos(this->window, 100, 100);
        }

        // Populate Keybinds
        {
            // assert given keybinds are valid
            magic_enum::enum_for_each<Action>(
                [&](Action a)
                {
                    util::assertFatal(
                        keyInformationMap.contains(a),
                        "Action {} was not populated",
                        magic_enum::enum_name(a));
                });

            for (const auto& [action, information] : keyInformationMap)
            {
                this->key_to_actions_map[information.key].push_back(action);
                this->action_active_map[action].store(
                    false, std::memory_order_release);
                this->action_interaction_map[action] = information.method;
            }
        }

        // Configure Callbacks
        {
            glfwSetWindowUserPointer(this->window, static_cast<void*>(this));

            std::ignore = glfwSetFramebufferSizeCallback(
                this->window, Window::frameBufferResizeCallback);

            std::ignore =
                glfwSetKeyCallback(this->window, Window::keypressCallback);

            std::ignore = glfwSetWindowFocusCallback(
                this->window, Window::windowFocusCallback);

            std::ignore = glfwSetMouseButtonCallback(
                this->window, Window::mouseButtonCallback);
        }

        // Configure framebuffer size, it may be different than the actual
        // window size (scaled displays)
        {
            int width  = -1;
            int height = -1;
            glfwGetFramebufferSize(this->window, &width, &height);

            this->framebuffer_size.store(
                vk::Extent2D {
                    static_cast<std::uint32_t>(width),
                    static_cast<std::uint32_t>(height)},
                std::memory_order_release);
        }

        util::logTrace("Initalized window");
    }

    Window::~Window()
    {
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }

    Window::Delta Window::getScreenSpaceMouseDelta() const
    {
        if (this->is_cursor_attached.load(std::memory_order_acquire)
            && this->mouse_ignore_frames.load(std::memory_order_acquire) == 0)
        {
            const vk::Extent2D framebufferSizePixels =
                this->framebuffer_size.load(std::memory_order_acquire);
            const Delta mouseDeltaPixels =
                this->mouse_delta_pixels.load(std::memory_order_acquire);

            return Delta {
                .x {mouseDeltaPixels.x
                    / static_cast<float>(framebufferSizePixels.width)},
                .y {mouseDeltaPixels.y
                    / static_cast<float>(framebufferSizePixels.height)},
            };
        }
        else
        {
            return Delta {.x {0.0f}, .y {0.0f}};
        }
    }

    bool Window::isActionActive(Action action, bool ignoreCursorAttached) const
    {
        if (!ignoreCursorAttached)
        {
            if (!this->is_cursor_attached.load(std::memory_order_acquire))
            {
                return false;
            }
        }

        if (this->action_interaction_map.at(action)
            == InteractionMethod::SinglePress)
        {
            return const_cast<std::atomic<bool>&>( // NOLINT: this is an atomic
                       this->action_active_map.at(action))
                .exchange(false, std::memory_order_acq_rel);
        }
        else
        {
            return this->action_active_map.at(action).load(
                std::memory_order_acquire);
        }
    }

    float Window::getDeltaTimeSeconds() const
    {
        return this->last_frame_duration.load(std::memory_order_acquire)
            .count();
    }

    vk::Extent2D Window::getFramebufferSize() const
    {
        return this->framebuffer_size.load(std::memory_order_acquire);
    }

    vk::UniqueSurfaceKHR Window::createSurface(vk::Instance instance)
    {
        VkSurfaceKHR maybeSurface = nullptr;

        const VkResult result = glfwCreateWindowSurface(
            static_cast<VkInstance>(instance),
            this->window,
            nullptr,
            &maybeSurface);

        util::assertFatal(
            result == VK_SUCCESS,
            "Failed to create window surface | {}",
            vk::to_string(vk::Result {result}));

        util::assertFatal(
            static_cast<bool>(maybeSurface), "Returned surface was a nullptr!");

        return vk::UniqueSurfaceKHR {vk::SurfaceKHR {maybeSurface}, instance};
    }

    bool Window::shouldClose()
    {
        return static_cast<bool>(glfwWindowShouldClose(this->window));
    }

    void Window::blockThisThreadWhileMinimized()
    {
        vk::Extent2D currentSize =
            this->framebuffer_size.load(std::memory_order_acquire);

        while (currentSize.width == 0 || currentSize.height == 0)
        {
            glfwWaitEvents();
            std::this_thread::yield();
            currentSize =
                this->framebuffer_size.load(std::memory_order_acquire);
        }
    }

    void Window::attachCursor()
    {
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        this->is_cursor_attached.store(true, std::memory_order_release);
    }

    void Window::detachCursor()
    {
        const vk::Extent2D currentSize =
            this->framebuffer_size.load(std::memory_order_acquire);

        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPos(
            this->window,
            static_cast<double>(
                currentSize.width / 2), // NOLINT: Desired behavior
            static_cast<double>(
                currentSize.height / 2)); // NOLINT: Desired behavior

        this->is_cursor_attached.store(false, std::memory_order_release);
    }

    void Window::endFrame()
    {
        // fire callbacks
        glfwPollEvents();

        // this is only ever decremented here and this function is already
        // in a critical section, i.e the render loop
        if (this->mouse_ignore_frames > 0)
        {
            --this->mouse_ignore_frames;
        }

        // Mouse processing
        std::pair<double, double> currentMousePositionDoubles {NAN, NAN};

        glfwGetCursorPos(
            this->window,
            &currentMousePositionDoubles.first,
            &currentMousePositionDoubles.second);

        const Delta currentMousePosition {
            static_cast<float>(currentMousePositionDoubles.first),
            static_cast<float>(currentMousePositionDoubles.second)};
        const Delta previousMousePosition {
            this->previous_mouse_position.load(std::memory_order_acquire)};

        this->mouse_delta_pixels.store(
            {currentMousePosition.x - previousMousePosition.x,
             currentMousePosition.y - previousMousePosition.y},
            std::memory_order_release);

        this->previous_mouse_position.store(
            currentMousePosition, std::memory_order_release);

        // Delta time processing
        const auto currentTime = std::chrono::steady_clock::now();

        this->last_frame_duration.store(
            currentTime - this->last_frame_end_time, std::memory_order_release);

        this->last_frame_end_time = currentTime;
    }

    void Window::keypressCallback(
        GLFWwindow*          glfwWindow,
        int                  key,         // NOLINT
        [[maybe_unused]] int scancode,    // NOLINT
        int                  inputAction, // NOLINT
        [[maybe_unused]] int mods)        // NOLINT
    {
        gfx::Window* const window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        if (!window->key_to_actions_map.contains(key))
        {
            util::logTrace("Unknown key pressed {}", key);
            return;
        }

        std::span<const Action> actionsFromKey =
            window->key_to_actions_map.at(key);

        switch (inputAction)
        {
        case GLFW_RELEASE:
            for (Action a : actionsFromKey)
            {
                window->action_active_map.at(a).store(
                    false, std::memory_order_release);
            }

            break;

        case GLFW_PRESS:
            for (Action a : actionsFromKey)
            {
                window->action_active_map.at(a).store(
                    true, std::memory_order_release);
            }
            break;

        case GLFW_REPEAT:
            break;

        default:
            util::panic("Unexpected action! {}", inputAction);
        }
    }

    void Window::frameBufferResizeCallback(
        GLFWwindow* glfwWindow, int newWidth, int newHeight)
    {
        gfx::Window* window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        window->framebuffer_size.store(
            vk::Extent2D {
                .width {static_cast<std::uint32_t>(newWidth)},
                .height {static_cast<std::uint32_t>(newHeight)}},
            std::memory_order_release);
    }

    void Window::mouseButtonCallback(
        GLFWwindow*          glfwWindow,
        int                  button,    // NOLINT
        int                  action,    // NOLINT
        [[maybe_unused]] int modifiers) // NOLINT
    {
        gfx::Window* window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        std::atomic<bool>& buttonToModify =
            window->mouse_buttons_pressed_state.at(
                static_cast<std::size_t>(button));

        switch (action)
        {
        case GLFW_RELEASE:
            if (!buttonToModify.exchange(false))
            {
                util::logWarn("Mouse button #{} already released!", button);
            }
            break;

        case GLFW_PRESS:
            if (buttonToModify.exchange(true))
            {
                util::logWarn("Mouse button #{} already depressed!", button);
            }
            break;

        case GLFW_REPEAT:
            buttonToModify.store(true);
            break;

        default:
            util::panic("Unexpected action! {}", action);
        }
    }

    void Window::windowFocusCallback(GLFWwindow* glfwWindow, int isFocused)
    {
        gfx::Window* window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        if (static_cast<bool>(isFocused))
        {
            window->attachCursor();
        }
    }

    void Window::errorCallback(int errorCode, const char* message)
    {
        util::logWarn("GLFW Error raised! | Code: {} | {}", errorCode, message);
    }

} // namespace gfx
