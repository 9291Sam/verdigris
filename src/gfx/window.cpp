#include "window.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_structs.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <util/log.hpp>
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE // NOLINT

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#define VMA_IMPLEMENTATION           1
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.h"

#pragma clang diagnostic pop

    namespace gfx
{
    using std::chrono_literals::operator""ms;
    using Order = std::memory_order;

    constexpr std::chrono::time_point<std::chrono::steady_clock> NullTime {};
    constexpr std::chrono::duration<float> MaximumSingleFireTime {0.01ms};

    Window::Window(
        const std::map<Action, ActionInformation>& key_information_map,
        vk::Extent2D                               size,
        const char*                                name)
        : window {nullptr}
        , ignore_frames {3}
        , framebuffer_size {size}
        , screen_space_mouse_delta {{0.0f, 0.0f}}
        , key_to_actions_map {}              // NOLINT
        , action_to_maybe_active_time_map {} // NOLINT
        , action_interaction_map {}          // NOLINT
        , last_frame_end_time {std::chrono::steady_clock::now()}
        , last_frame_duration {16ms} // NOLINT
        , previous_mouse_position {{0.0f, 0.0f}}
        , mouse_delta_pixels {{0.0f, 0.0f}}
        , is_cursor_attached {true}
    {
        // Glfw initalization
        util::assertFatal(
            glfwSetErrorCallback(Window::errorCallback) == nullptr,
            "Error callback was set multiple times! Multiple windows were "
            "spawned!");

        util::assertFatal(
            glfwInit() == GLFW_TRUE, "Failed to initialize GLFW!");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        this->window = glfwCreateWindow( // NOLINT
            static_cast<int>(size.width),
            static_cast<int>(size.height),
            name,
            nullptr,
            nullptr);

        util::assertFatal(
            this->window != nullptr, "Failed to create GLFW window!");

        // populate key maps
        for (const auto [action, information] : key_information_map)
        {
            this->key_to_actions_map[information.key].push_back(action);
            this->action_to_maybe_active_time_map[action].store(
                NullTime, Order::release);
            this->action_interaction_map[action] = information.method;
        }

        util::assertFatal(
            key_information_map.size()
                == static_cast<std::size_t>(Action::MaxEnumValue) - 1,
            "Not all keys populated! {} {}",
            key_information_map.size(),
            std::to_underlying(Action::MaxEnumValue) - 1);

        // update other callbacks
        glfwSetWindowUserPointer(this->window, static_cast<void*>(this));

        std::ignore = glfwSetFramebufferSizeCallback(
            this->window, Window::frameBufferResizeCallback);

        std::ignore =
            glfwSetKeyCallback(this->window, Window::keypressCallback);

        std::ignore = glfwSetWindowFocusCallback(
            this->window, Window::windowFocusCallback);

        std::ignore = glfwSetMouseButtonCallback(
            this->window, Window::mouseButtonCallback);

        // the requested framebuffer size may be different than the window size
        int width  = -1;
        int height = -1;
        glfwGetFramebufferSize(this->window, &width, &height);

        this->framebuffer_size.store(
            vk::Extent2D {
                static_cast<std::uint32_t>(width),
                static_cast<std::uint32_t>(height)},
            Order::release);

        util::logTrace("Initalized window");
    }

    Window::~Window()
    {
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }

    Window::Delta Window::getScreenSpaceMouseDelta() const
    {
        if (this->is_cursor_attached.load(Order::acquire)
            && this->ignore_frames.load(Order::acquire) == 0)
        {
            const vk::Extent2D FramebufferSizePixels =
                this->framebuffer_size.load(Order::acquire);
            const Delta MouseDeltaPixels =
                this->mouse_delta_pixels.load(Order::acquire);

            return Delta {
                .x {MouseDeltaPixels.x
                    / static_cast<float>(FramebufferSizePixels.width)},
                .y {MouseDeltaPixels.y
                    / static_cast<float>(FramebufferSizePixels.height)},
            };
        }
        else
        {
            return Delta {.x {0.0f}, .y {0.0f}};
        }
    }

    bool Window::isActionActive(Action action, bool ignoreCursorAttached) const
    {
        if (ignoreCursorAttached)
        {
            return this->action_to_maybe_active_time_map.at(action).load(
                       Order::acquire)
                != NullTime;
        }
        else
        {
            if (!this->is_cursor_attached.load(Order::acquire))
            {
                return false;
            }

            return this->action_to_maybe_active_time_map.at(action).load(
                       Order::acquire)
                != NullTime;
        }
    }

    float Window::getDeltaTimeSeconds() const
    {
        return this->last_frame_duration.load(Order::acquire).count();
    }

    vk::Extent2D Window::getFramebufferSize() const
    {
        return this->framebuffer_size.load(Order::acquire);
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
        vk::Extent2D currentSize = this->framebuffer_size.load(Order::acquire);

        while (currentSize.width == 0 || currentSize.height == 0)
        {
            glfwWaitEvents();
            std::this_thread::yield();
            currentSize = this->framebuffer_size.load(Order::acquire);
        }
    }

    void Window::attachCursor()
    {
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        this->is_cursor_attached.store(true, Order::release);
    }

    void Window::detachCursor()
    {
        const vk::Extent2D currentSize =
            this->framebuffer_size.load(Order::acquire);

        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetCursorPos(
            this->window,
            static_cast<double>(
                currentSize.width / 2), // NOLINT: Desired behavior
            static_cast<double>(
                currentSize.height / 2)); // NOLINT: Desired behavior

        this->is_cursor_attached.store(false, Order::release);
    }

    void Window::endFrame()
    {
        const std::chrono::time_point<std::chrono::steady_clock>
            StartEndFrameTime = std::chrono::steady_clock::now();

        // cull InteractionMethod::SinglePress that exist for more than the
        // Frame time
        // nasty condition :|
        // clang-format off
        for (auto& [action, actionTime] : this->action_to_maybe_active_time_map)
        {
            if (this->action_interaction_map.at(action) == InteractionMethod::SinglePress
                && actionTime.load(Order::acquire) - StartEndFrameTime < MaximumSingleFireTime)
            {
                actionTime.store(NullTime, Order::release);
            }
        }
        // clang-format on

        // fire callbacks
        glfwPollEvents();

        // this is only ever decremented here and this function is already
        // in a critical section, i.e the render loop
        if (this->ignore_frames > 0)
        {
            --this->ignore_frames;
        }

        // Mouse processing
        std::pair<double, double> currentMousePositionDoubles {NAN, NAN};

        glfwGetCursorPos(
            this->window,
            &currentMousePositionDoubles.first,
            &currentMousePositionDoubles.second);

        const Delta CurrentMousePosition {
            static_cast<float>(currentMousePositionDoubles.first),
            static_cast<float>(currentMousePositionDoubles.second)};
        const Delta PreviousMousePosition {
            this->previous_mouse_position.load(Order::acquire)};

        this->mouse_delta_pixels.store(
            {CurrentMousePosition.x - PreviousMousePosition.x,
             CurrentMousePosition.y - PreviousMousePosition.y},
            Order::release);

        this->previous_mouse_position.store(
            CurrentMousePosition, Order::release);

        // Delta time processing
        const auto currentTime = std::chrono::steady_clock::now();

        this->last_frame_duration.store(
            currentTime - this->last_frame_end_time, Order::release);

        this->last_frame_end_time = currentTime;
    }

    void Window::keypressCallback(
        GLFWwindow * glfwWindow,
        int                  key,
        [[maybe_unused]] int scancode,
        int                  inputAction,
        [[maybe_unused]] int mods)
    {
        gfx::Window* window =
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
                window->action_to_maybe_active_time_map.at(a).store(
                    NullTime, Order::release);
            }

            break;

        case GLFW_PRESS:
            for (Action a : actionsFromKey)
            {
                window->action_to_maybe_active_time_map.at(a).store(
                    std::chrono::steady_clock::now(), Order::release);
            }
            break;

        case GLFW_REPEAT:
            break;

        default:
            util::panic("Unexpected action! {}", inputAction);
        }
    }

    void Window::frameBufferResizeCallback(
        GLFWwindow * glfwWindow, int newWidth, int newHeight)
    {
        gfx::Window* window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        window->framebuffer_size.store(
            vk::Extent2D {
                .width {static_cast<std::uint32_t>(newWidth)},
                .height {static_cast<std::uint32_t>(newHeight)}},
            Order::release);
    }

    void Window::mouseButtonCallback(
        GLFWwindow * glfwWindow, int button, int action, int)
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

    void Window::windowFocusCallback(GLFWwindow * glfwWindow, int isFocused)
    {
        gfx::Window* window =
            static_cast<gfx::Window*>(glfwGetWindowUserPointer(glfwWindow));

        if (isFocused)
        {
            window->attachCursor();
        }
    }

    void Window::errorCallback(int errorCode, const char* message)
    {
        util::logWarn("GLFW Error raised! | Code: {} | {}", errorCode, message);
    }

} // namespace gfx
