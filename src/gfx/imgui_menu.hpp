#ifndef SRC_GFX_IMGUI_MENU_HPP
#define SRC_GFX_IMGUI_MENU_HPP

#include <glm/vec3.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    namespace vulkan
    {
        class Instance;
        class Device;
        class RenderPass;
    } // namespace vulkan
    class Window;

    class ImGuiMenu
    {
    public:
        ImGuiMenu(
            const Window&, vk::Instance, const vulkan::Device&, vk::RenderPass);
        ~ImGuiMenu() noexcept;

        ImGuiMenu(const ImGuiMenu&)             = delete;
        ImGuiMenu(ImGuiMenu&&)                  = delete;
        ImGuiMenu& operator= (const ImGuiMenu&) = delete;
        ImGuiMenu& operator= (ImGuiMenu&&)      = delete;

        void setPlayerPosition(glm::vec3) const;
        void setFps(float) const;

        void render();
        void draw(vk::CommandBuffer);

    private:
        // rendering things
        vk::UniqueDescriptorPool pool;

        // menu items
        mutable std::atomic<glm::vec3> player_position;
        mutable std::atomic<float>     fps;
        std::string                    string;
    };
} // namespace gfx

#endif // SRC_GFX_IMGUI_MENU_HPP