#ifndef SRC_GFX_RECORDABLES_DEBUG_MENU_HPP
#define SRC_GFX_RECORDABLES_DEBUG_MENU_HPP

#include "recordable.hpp"

namespace gfx
{
    class Window;
}

namespace gfx::vulkan
{
    class Device;
    class Instance;
} // namespace gfx::vulkan

namespace gfx::recordables
{
    class DebugMenu final : public Recordable
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

        static std::shared_ptr<DebugMenu> create(
            const gfx::Renderer&,
            gfx::vulkan::Instance&,
            gfx::vulkan::Device&,
            gfx::Window&,
            vk::RenderPass);
        ~DebugMenu() override;

        void updateFrameState() const override;
        void record(vk::CommandBuffer, const Camera&) const override;

        void setVisibility(bool visible) const;

    private:
        mutable State            state;
        vk::UniqueDescriptorPool pool;
        mutable std::string_view current_logging_level_selection;

        DebugMenu(
            const gfx::Renderer&,
            gfx::vulkan::Instance&,
            gfx::vulkan::Device&,
            gfx::Window&);
    };
} // namespace gfx::recordables

#endif // SRC_GFX_RECORDABLES_DEBUG_MENU_HPP
