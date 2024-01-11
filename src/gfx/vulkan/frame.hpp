#ifndef SRC_GFX_VULKAN_FRAME_HPP
#define SRC_GFX_VULKAN_FRAME_HPP

#include "image.hpp"
#include <expected>
#include <gfx/camera.hpp>
#include <gfx/draw_stages.hpp>
#include <map>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::recordables
{
    class Recordable;
} // namespace gfx::recordables

namespace gfx::vulkan
{
    class RenderPass;
    class Device;
    class FlyingFrame;
    class Image2D;
    class Swapchain;
    class Allocator;

    struct ResizeNeeded
    {};

    class FrameManager
    {
    public:

        explicit FrameManager(
            vk::Device,
            Allocator*,
            Swapchain*,
            vk::RenderPass finalRasterPass,
            std::size_t    numberOfFlyingFrames = 3);
        ~FrameManager();

        // attaches swapchain's framebuffer to final renderpass
        std::expected<void, ResizeNeeded> renderObjectsFromCamera(
            Camera,
            std::vector<std::pair<
                std::optional<const vulkan::RenderPass*>,
                std::vector<const recordables::Recordable*>>>);

    private:
        vk::Device device;

        Swapchain*                         swapchain;
        std::vector<vk::UniqueFramebuffer> swapchain_framebuffers;
        vulkan::Image2D                    depth_buffer;

        vk::RenderPass final_raster_pass;

        std::optional<vk::Fence> maybe_previous_frame_fence;
        std::vector<FlyingFrame> flying_frames;
        std::size_t              flying_frame_index;
        std::size_t              number_of_flying_frames;
    };

    class FlyingFrame
    {
    public:
        explicit FlyingFrame(Device*);
        ~FlyingFrame();

        FlyingFrame(const FlyingFrame&)                 = delete;
        FlyingFrame(FlyingFrame&&) noexcept             = default;
        FlyingFrame& operator= (const FlyingFrame&)     = delete;
        FlyingFrame& operator= (FlyingFrame&&) noexcept = default;

        [[nodiscard]] vk::Fence getExecutionFinishFence() const;

        [[nodiscard]] std::expected<void, ResizeNeeded> recordAndDisplay(
            Camera,
            std::vector<std::pair<
                std::optional<const vulkan::RenderPass*>,
                std::vector<const recordables::Recordable*>>>,
            vk::SwapchainKHR                       presentSwapchain,
            vk::RenderPass                         finalRasterPass,
            std::span<const vk::UniqueFramebuffer> framebuffers,
            vk::Extent2D                           framebufferExtent,
            std::optional<vk::Fence> maybePreviousFrameInFlightFence);

    private:
        Device* device;

        vk::UniqueSemaphore     image_available;
        vk::UniqueSemaphore     render_finished;
        vk::UniqueFence         frame_in_flight;
        vk::UniqueCommandPool   command_pool;
        vk::UniqueCommandBuffer command_buffer;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_FRAME_HPP
