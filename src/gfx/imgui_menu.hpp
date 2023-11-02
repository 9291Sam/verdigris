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
        class Image2D;
    } // namespace vulkan
    class Window;

    class ImGuiMenu
    {
    public:

        struct State
        {
            glm::vec3   player_position;
            float       fps;
            float       tps;
            std::string string;
        };

    public:
        ImGuiMenu(
            const Window&, vk::Instance, const vulkan::Device&, vk::RenderPass);
        ~ImGuiMenu() noexcept;

        ImGuiMenu(const ImGuiMenu&)             = delete;
        ImGuiMenu(ImGuiMenu&&)                  = delete;
        ImGuiMenu& operator= (const ImGuiMenu&) = delete;
        ImGuiMenu& operator= (ImGuiMenu&&)      = delete;

        void bindImage(const vulkan::Image2D&);
        void render(State&);
        void draw(vk::CommandBuffer);

    private:
        vk::UniqueDescriptorPool pool;
        /// Unfortunately, imgui is entirely based off of global state.

        vk::Extent2D      display_image_size;
        vk::UniqueSampler sampler;
        vk::DescriptorSet image_descriptor;
    };
} // namespace gfx

#endif // SRC_GFX_IMGUI_MENU_HPP