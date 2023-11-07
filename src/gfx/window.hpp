#ifndef SRC_GFX_WINDOW_HPP
#define SRC_GFX_WINDOW_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

struct GLFWwindow;

namespace gfx
{
    class ImGuiMenu;

    /// Handles creation of the window
    /// Exposes surface to the renderer
    /// Handles user input
    class Window
    {
    public:
        struct Delta
        {
            float x;
            float y;
        };

        using GlfwKeyType = int;

        enum class Action : std::uint_fast8_t // bike shedding
        {
            PlayerMoveForward      = 0,
            PlayerMoveBackward     = 1,
            PlayerMoveLeft         = 2,
            PlayerMoveRight        = 3,
            PlayerMoveUp           = 5,
            PlayerMoveDown         = 6,
            PlayerSprint           = 7,
            ToggleConsole          = 8,
            ToggleCursorAttachment = 9,
        };

        enum class InteractionMethod
        {
            /// Only fires for one frame, no matter how long you hold the button
            /// down for. Useful for a toggle switch,
            /// i.e opening the developer console
            /// opening an inventory menu
            SinglePress,
            /// Fires every frame, as long as the button is pressed
            /// Useful for movement keys
            EveryFrame,
        };

        struct ActionInformation
        {
            GlfwKeyType       key;
            InteractionMethod method;
        };
    public:

        Window(
            const std::map<Action, ActionInformation>&,
            vk::Extent2D windowSize,
            const char*  name);
        ~Window();

        Window(const Window&)             = delete;
        Window(Window&&)                  = delete;
        Window& operator= (const Window&) = delete;
        Window& operator= (Window&&)      = delete;

        [[nodiscard]] bool
        isActionActive(Action, bool IgnoreCursorAttached = false) const;
        [[nodiscard]] Delta        getScreenSpaceMouseDelta() const;
        [[nodiscard]] float        getDeltaTimeSeconds() const;
        [[nodiscard]] vk::Extent2D getFramebufferSize() const;

        // None of these functions are threadsafe
        [[nodiscard]] vk::UniqueSurfaceKHR createSurface(vk::Instance);
        [[nodiscard]] bool                 shouldClose();

        void blockThisThreadWhileMinimized();
        void attachCursor();
        void detachCursor();
        void endFrame();
        // TODO: void updateKeyBindings();

    private:
        static void keypressCallback(GLFWwindow*, int, int, int, int);
        static void frameBufferResizeCallback(GLFWwindow*, int, int);
        static void mouseButtonCallback(GLFWwindow*, int, int, int);
        static void windowFocusCallback(GLFWwindow*, int);
        static void errorCallback(int, const char*);

        friend class ::gfx::ImGuiMenu;

        GLFWwindow*               window;
        std::atomic<std::size_t>  ignore_frames;
        std::atomic<vk::Extent2D> framebuffer_size;

        std::atomic<Delta> screen_space_mouse_delta;

        std::map<GlfwKeyType, std::vector<Action>> key_to_actions_map;
        std::map<
            Action,
            std::atomic<std::chrono::time_point<std::chrono::steady_clock>>>
                                            action_to_maybe_active_time_map;
        std::map<Action, InteractionMethod> action_interaction_map;
        std::array<std::atomic<bool>, 8> mouse_buttons_pressed_state; // NOLINT

        std::chrono::time_point<std::chrono::steady_clock> last_frame_end_time;
        std::atomic<std::chrono::duration<float>>          last_frame_duration;

        std::atomic<Delta> previous_mouse_position;
        std::atomic<Delta> mouse_delta_pixels;
        std::atomic<bool>  is_cursor_attached;
    };
} // namespace gfx

#endif // SRC_GFX_WINDOW_HPP