#ifndef SRC_GFX_VULKAN_SWAPCHAIN_HPP
#define SRC_GFX_VULKAN_SWAPCHAIN_HPP

#include <vector>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Device;

    class Swapchain
    {
    public:
        Swapchain(const Device&, vk::SurfaceKHR, vk::Extent2D);
        ~Swapchain() = default;

        Swapchain()                             = delete;
        Swapchain(const Swapchain&)             = delete;
        Swapchain(Swapchain&&)                  = delete;
        Swapchain& operator= (const Swapchain&) = delete;
        Swapchain& operator= (Swapchain&&)      = delete;

        [[nodiscard]] vk::SwapchainKHR operator* () const;

        [[nodiscard]] vk::Extent2D         getExtent() const;
        [[nodiscard]] vk::SurfaceFormatKHR getSurfaceFormat() const;

        [[nodiscard]] std::span<const vk::UniqueImageView> getRenderTargets() const;

    private:
        vk::Device device;

        vk::Extent2D         extent;
        vk::SurfaceFormatKHR format;

        vk::UniqueSwapchainKHR           swapchain;
        std::vector<vk::Image>           images;
        std::vector<vk::UniqueImageView> image_views;

    }; // class Swapchain

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_SWAPCHAIN_HPP
