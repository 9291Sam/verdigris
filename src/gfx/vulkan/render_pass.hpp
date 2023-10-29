#ifndef SRC_GFX_VULKAN_RENDER__PASS_HPP
#define SRC_GFX_VULKAN_RENDER__PASS_HPP

#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;
    class Image2D;
    class Swapchain;

    class RenderPass
    {
    public:

        RenderPass(vk::Device, const Swapchain&, const Image2D& depthBuffer);
        ~RenderPass() = default;

        RenderPass()                              = delete;
        RenderPass(const RenderPass&)             = delete;
        RenderPass(RenderPass&&)                  = delete;
        RenderPass& operator= (const RenderPass&) = delete;
        RenderPass& operator= (RenderPass&&)      = delete;

        [[nodiscard]] vk::RenderPass operator* () const;

    private:
        vk::UniqueRenderPass render_pass;
    }; // class RenderPass

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_RENDER__PASS_HPP
