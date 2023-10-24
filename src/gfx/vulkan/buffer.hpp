#ifndef SRC_GFX_VULKAN_BUFFER
#define SRC_GFX_VULKAN_BUFFER

#include <cstdint>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaAllocator)

namespace gfx::vulkan
{
    class Allocator;

    class Buffer
    {
    public:

        Buffer();
        Buffer(
            Allocator*  allocator,
            std::size_t sizeBytes,
            vk::BufferUsageFlags,
            vk::MemoryPropertyFlags);
        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator= (const Buffer&) = delete;
        Buffer& operator= (Buffer&&) noexcept;

        [[nodiscard]] vk::Buffer  operator* () const;
        [[nodiscard]] std::size_t sizeBytes() const;
        [[nodiscard]] void*       getMappedPtr() const;

        void write(std::span<const std::byte>) const;
        void copyFrom(const Buffer&, vk::CommandBuffer) const;

    private:
        void free();

        VmaAllocator                    allocator;
        vk::Buffer                      buffer;
        VmaAllocation                   allocation;
        std::size_t                     size_bytes;
        mutable std::atomic<std::byte*> mapped_memory;
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_BUFFER