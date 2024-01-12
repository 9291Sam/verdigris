#ifndef SRC_GFX_VULKAN_RENDER_PASS_HPP
#define SRC_GFX_VULKAN_RENDER_PASS_HPP

#include <array>
#include <expected>
#include <functional>
#include <gfx/camera.hpp>
#include <util/misc.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class ImGuiMenu;
    class Recordable;
} // namespace gfx

namespace gfx::vulkan
{
    class Device;
    class Image2D;
    class Swapchain;
    class Frame;

    namespace voxel
    {
        class ComputeRenderer;
    } // namespace voxel

    class RenderPass
    {
    public:

        RenderPass(
            vk::Device,
            vk::RenderPassCreateInfo,
            std::optional<std::pair<vk::Framebuffer, vk::Extent2D>>,
            std::span<vk::ClearValue>,
            const std::string& name);
        ~RenderPass() = default;

        RenderPass()                              = delete;
        RenderPass(const RenderPass&)             = delete;
        RenderPass(RenderPass&&)                  = delete;
        RenderPass& operator= (const RenderPass&) = delete;
        RenderPass& operator= (RenderPass&&)      = delete;

        void setFramebuffer(vk::Framebuffer, vk::Extent2D) const;
        void recordWith(
            vk::CommandBuffer commandBuffer, std::invocable<> auto&& func) const
            requires std::is_nothrow_invocable_v<decltype(func)>
        {
            vk::RenderPassBeginInfo renderPassBeginInfo {
                .sType {vk::StructureType::eRenderPassBeginInfo},
                .pNext {nullptr},
                .renderPass {*this->render_pass},
                .framebuffer {this->framebuffer},
                .renderArea {vk::Rect2D {
                    .offset {0, 0},
                    .extent {this->framebuffer_extent},
                }},
                .clearValueCount {
                    static_cast<std::uint32_t>(this->clear_values_size)},
                .pClearValues {this->maybe_clear_values.data()},
            };

            commandBuffer.beginRenderPass(
                renderPassBeginInfo, vk::SubpassContents::eInline);
            {
                func();
            }
            commandBuffer.endRenderPass();
        }

        // TODO: remove and give imgui its own renderpass!
        [[nodiscard]] vk::RenderPass operator* () const;
        [[nodiscard]] vk::Extent2D   getExtent() const;



    private:
        vk::UniqueRenderPass          render_pass;
        mutable vk::Framebuffer       framebuffer;
        mutable vk::Extent2D          framebuffer_extent;
        std::size_t                   clear_values_size;
        std::array<vk::ClearValue, 2> maybe_clear_values;
    }; // class RenderPass

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_RENDER_PASS_HPP
