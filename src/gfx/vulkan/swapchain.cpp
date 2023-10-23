#include "swapchain.hpp"
#include "device.hpp"
#include <util/log.hpp>

namespace gfx::vulkan
{
    Swapchain::Swapchain(const Device& device_, vk::SurfaceKHR surface, vk::Extent2D extent_)
        : device {device_.asLogicalDevice()}
        , extent {extent_}
        , format {vk::SurfaceFormatKHR {
              .format {vk::Format::eB8G8R8A8Srgb},
              .colorSpace {vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear}}}
        , swapchain {nullptr}
        , images {}      // NOLINT
        , image_views {} // NOLINT
    {
        const auto availableSurfaces = device_.asPhysicalDevice().getSurfaceFormatsKHR(surface);

        if (std::ranges::find(availableSurfaces, this->format) == availableSurfaces.end())
        {
            util::panic("Failed to find ideal surface format!");
        }

        const vk::PresentModeKHR selectedPresentMode = [&]
        {
            const auto availablePresentModes =
                device_.asPhysicalDevice().getSurfacePresentModesKHR(surface);

            if (std::ranges::find(availablePresentModes, vk::PresentModeKHR::eMailbox)
                != availablePresentModes.cend())
            {
                return vk::PresentModeKHR::eMailbox;
            }
            else
            {
                return vk::PresentModeKHR::eFifo;
            }
        }();

        const vk::SurfaceCapabilitiesKHR surfaceCapabilities =
            device_.asPhysicalDevice().getSurfaceCapabilitiesKHR(surface);

        // clang-format off
        const vk::SwapchainCreateInfoKHR swapchainCreateInfoKHR
        {
            .sType         {vk::StructureType::eSwapchainCreateInfoKHR},
            .pNext         {nullptr},
            .flags         {},
            .surface       {surface},
            .minImageCount {// :eyes:
                surfaceCapabilities.maxImageCount == 0
                ? surfaceCapabilities.minImageCount + 1
                : surfaceCapabilities.maxImageCount
            },
            .imageFormat           {this->format.format},
            .imageColorSpace       {this->format.colorSpace},
            .imageExtent           {this->extent},
            .imageArrayLayers      {1},
            .imageUsage            {vk::ImageUsageFlagBits::eColorAttachment},
            .imageSharingMode      {vk::SharingMode::eExclusive},
            .queueFamilyIndexCount {0},
            .pQueueFamilyIndices   {nullptr},
            .preTransform          {surfaceCapabilities.currentTransform},
            .compositeAlpha        {vk::CompositeAlphaFlagBitsKHR::eOpaque},
            .presentMode           {selectedPresentMode},
            .clipped               {static_cast<vk::Bool32>(true)},
            .oldSwapchain          {nullptr}
        };
        // clang-format on

        this->swapchain = this->device.createSwapchainKHRUnique(swapchainCreateInfoKHR);
        this->images    = this->device.getSwapchainImagesKHR(*this->swapchain);

        // image view initialization
        this->image_views.reserve(this->images.size());

        for (const vk::Image i : this->images)
        {
            const vk::ImageViewCreateInfo imageCreateInfo {
                .sType {vk::StructureType::eImageViewCreateInfo},
                .pNext {nullptr},
                .flags {},
                .image {i},
                .viewType {vk::ImageViewType::e2D},
                .format {this->format.format},
                .components {
                    .r {vk::ComponentSwizzle::eIdentity},
                    .g {vk::ComponentSwizzle::eIdentity},
                    .b {vk::ComponentSwizzle::eIdentity},
                    .a {vk::ComponentSwizzle::eIdentity},
                },
                .subresourceRange {
                    .aspectMask {vk::ImageAspectFlagBits::eColor},
                    .baseMipLevel {0},
                    .levelCount {1},
                    .baseArrayLayer {0},
                    .layerCount {1},
                },
            };

            this->image_views.push_back(this->device.createImageViewUnique(imageCreateInfo));
        }
    }

    vk::SwapchainKHR Swapchain::operator* () const
    {
        return *this->swapchain;
    }

    vk::Extent2D Swapchain::getExtent() const
    {
        return this->extent;
    }

    vk::SurfaceFormatKHR Swapchain::getSurfaceFormat() const
    {
        return this->format;
    }

    std::span<const vk::UniqueImageView> Swapchain::getRenderTargets() const
    {
        return this->image_views;
    }
} // namespace gfx::vulkan