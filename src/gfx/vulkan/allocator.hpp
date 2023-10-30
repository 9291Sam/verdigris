#ifndef SRC_GFX_VULKAN_ALLOCATOR_HPP
#define SRC_GFX_VULKAN_ALLOCATOR_HPP

#include "descriptors.hpp"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

VK_DEFINE_HANDLE(VmaAllocator)

namespace gfx::vulkan
{
    class Instance;
    class Device;
    class DescriptorPool;

    class Allocator
    {
    public:

        Allocator(const vulkan::Instance&, const vulkan::Device&);
        ~Allocator();

        Allocator(const Allocator&)             = delete;
        Allocator(Allocator&&)                  = delete;
        Allocator& operator= (const Allocator&) = delete;
        Allocator& operator= (Allocator&&)      = delete;

        VmaAllocator  operator* () const;
        DescriptorSet allocateDescriptorSet(DescriptorSetType);

        // not the best place to put this, but whatever, it needs to be
        // somewhere
        DescriptorSetLayout& getDescriptorSetLayout(DescriptorSetType);

    private:
        VmaAllocator   allocator;
        DescriptorPool pool;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_ALLOCATOR_HPP