#ifndef SRC_GFX_VULKAN_RENDER__PASS_HPP
#define SRC_GFX_VULKAN_RENDER__PASS_HPP

#include <expected>
#include <functional>
#include <gfx/camera.hpp>
#include <gfx/object.hpp>
#include <span>
#include <util/misc.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class ImGuiMenu;
}

namespace gfx::vulkan
{
    class Device;
    class Image2D;
    class Swapchain;
    class Frame;
    class PipelineManager;

    namespace voxel
    {
        class ComputeRenderer;
    }

    class RenderPass
    {
    public:

        RenderPass(Device*, Swapchain*, const Image2D& depthBuffer);
        ~RenderPass() = default;

        RenderPass()                              = delete;
        RenderPass(const RenderPass&)             = delete;
        RenderPass(RenderPass&&)                  = delete;
        RenderPass& operator= (const RenderPass&) = delete;
        RenderPass& operator= (RenderPass&&)      = delete;

        [[nodiscard]] vk::RenderPass operator* () const;
        [[nodiscard]] Frame&         getNextFrame();

    private:
        vk::UniqueRenderPass render_pass;
        Device*              device;

        /// Because of how vulkan's image acquisition works, we're pigeonholed
        /// into doing this design and there's not much you can do about it.
        std::vector<vk::UniqueFramebuffer> framebuffers;
        std::size_t                        next_frame_index;
        std::vector<Frame>                 frames;
    }; // class RenderPass

    class Frame
    {
    public:
        struct ResizeNeeded
        {};
    public:

        Frame();
        Frame(
            std::vector<vk::UniqueFramebuffer>*,
            Device*,
            Swapchain*,
            vk::RenderPass);
        ~Frame() = default;

        Frame(const Frame&)                 = delete;
        Frame(Frame&&) noexcept             = default;
        Frame& operator= (const Frame&)     = delete;
        Frame& operator= (Frame&&) noexcept = default;

        // @return {true}, is resize needed
        [[nodiscard]] std::expected<void, ResizeNeeded> render(
            Camera,
            std::span<const Object*>,
            voxel::ComputeRenderer*,
            ImGuiMenu*,
            std::function<void()>
                postFrameUploadFunc); // TODO: move only function

    private:
        Device*                             device;
        Swapchain*                          swapchain;
        vk::RenderPass                      render_pass;
        std::vector<vk::UniqueFramebuffer>* framebuffers;

        vk::UniqueSemaphore image_available;
        vk::UniqueSemaphore render_finished;
        vk::UniqueFence     frame_in_flight;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_RENDER__PASS_HPP
