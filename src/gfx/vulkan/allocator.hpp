#ifndef SRC_GFX_VULKAN_ALLOCATOR_HPP
#define SRC_GFX_VULKAN_ALLOCATOR_HPP

#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Allocator
    {
    public:

        Allocator(vk::Instance, vk::Device);
        ~Allocator();

        Allocator(const Allocator&)             = delete;
        Allocator(Allocator&&)                  = delete;
        Allocator& operator= (const Allocator&) = delete;
        Allocator& operator= (Allocator&&)      = delete;

        [[nodiscard]] bool shouldBufferStage() const;
        VmaAllocator       operator* () const;



    private:
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_ALLOCATOR_HPP