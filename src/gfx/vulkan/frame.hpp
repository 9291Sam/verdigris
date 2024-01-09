#ifndef SRC_GFX_VULKAN_FRAME_HPP
#define SRC_GFX_VULKAN_FRAME_HPP

#include <expected>
#include <gfx/renderer.hpp>
#include <map>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx
{
    class Recordable;
} // namespace gfx

namespace gfx::vulkan
{
    class RenderPass;
    class Device;

    class FrameManager
    {};

    class FlyingFrame
    {
    public:
        struct ResizeNeeded
        {};
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
            const std::map<
                Renderer::DrawStage,
                std::pair<
                    std::optional<vulkan::RenderPass*>,
                    std::span<const Recordable*>>>&,
            vk::SwapchainKHR         presentSwapchain,
            std::uint32_t            presentSwapchainFramebufferIndex,
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
