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
    class PipelineCache;

    class Allocator
    {
    public:

        Allocator(const vulkan::Instance&, Device*);
        ~Allocator();

        Allocator(const Allocator&)             = delete;
        Allocator(Allocator&&)                  = delete;
        Allocator& operator= (const Allocator&) = delete;
        Allocator& operator= (Allocator&&)      = delete;

        [[nodiscard]] VmaAllocator          operator* () const;
        [[nodiscard]] const DescriptorPool& getDescriptorPool() const;
        [[nodiscard]] const PipelineCache&  getPipelineCache() const;

    private:
        Device*        device;
        VmaAllocator   allocator;
        DescriptorPool pool;
    };

} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_ALLOCATOR_HPP
