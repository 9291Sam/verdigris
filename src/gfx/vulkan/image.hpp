#ifndef SRC_GFX_VULKAN_IMAGE_HPP
#define SRC_GFX_VULKAN_IMAGE_HPP

#include <memory>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

namespace gfx::vulkan
{
    class Allocator;
    class Device;
    class Buffer;

    class Image2D
    {
    public:

        Image2D(
            Allocator*,
            vk::Device,
            vk::Extent2D,
            vk::Format,
            vk::ImageUsageFlags,
            vk::ImageAspectFlags,
            vk::ImageTiling,
            vk::MemoryPropertyFlags);
        ~Image2D();

        Image2D(const Image2D&)             = delete;
        Image2D(Image2D&&)                  = delete;
        Image2D& operator= (const Image2D&) = delete;
        Image2D& operator= (Image2D&&)      = delete;

        [[nodiscard]] vk::ImageView   operator* () const;
        [[nodiscard]] vk::Format      getFormat() const;
        [[nodiscard]] vk::ImageLayout getLayout() const;

        void transitionLayout(
            vk::CommandBuffer,
            vk::ImageLayout        from,
            vk::ImageLayout        to,
            vk::PipelineStageFlags sourceStage,
            vk::PipelineStageFlags destinationStage,
            vk::AccessFlags        sourceAccess,
            vk::AccessFlags        destinationAccess);
        void copyFromBuffer(vk::CommandBuffer, const Buffer&) const;

    private:
        VmaAllocator allocator;

        vk::Extent2D         extent;
        vk::Format           format;
        vk::ImageAspectFlags aspect;
        vk::ImageLayout      layout;

        vk::Image           image;
        VmaAllocation       memory;
        vk::UniqueImageView view;
    }; // class Image2D

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_IMAGE_HPP
