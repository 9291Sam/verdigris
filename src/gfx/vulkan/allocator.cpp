#include "allocator.hpp"
#include "device.hpp"
#include "instance.hpp"
#include <util/log.hpp>
#include <vk_mem_alloc.h>

namespace gfx::vulkan
{
    Allocator::Allocator(
        const vulkan::Instance& instance, const vulkan::Device& device)
        : allocator {nullptr}
        , pool {
              device.asLogicalDevice(),
              std::unordered_map<vk::DescriptorType, std::uint32_t> {
                  {vk::DescriptorType::eStorageBuffer, 3},
                  {vk::DescriptorType::eUniformBuffer, 3},
                  {vk::DescriptorType::eStorageImage, 3},
              }}
    {
        VmaVulkanFunctions vulkanFunctions {};
        vulkanFunctions.vkGetInstanceProcAddr =
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr =
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

        const VmaAllocatorCreateInfo allocatorCreateInfo {
            .flags {},
            .physicalDevice {device.asPhysicalDevice()},
            .device {device.asLogicalDevice()},
            .preferredLargeHeapBlockSize {0}, // chosen by VMA
            .pAllocationCallbacks {nullptr},
            .pDeviceMemoryCallbacks {nullptr},
            .pHeapSizeLimit {nullptr},
            .pVulkanFunctions {&vulkanFunctions},
            .instance {*instance},
            .vulkanApiVersion {instance.getVulkanVersion()},
            .pTypeExternalMemoryHandleTypes {nullptr},
        };

        const vk::Result result {
            ::vmaCreateAllocator(&allocatorCreateInfo, &this->allocator)};

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to create allocator | {}",
            vk::to_string(result));
    }

    Allocator::~Allocator()
    {
        ::vmaDestroyAllocator(this->allocator);
    }

    VmaAllocator Allocator::operator* () const
    {
        return this->allocator;
    }

    DescriptorSet Allocator::allocateDescriptorSet(DescriptorSetType setType)
    {
        return this->pool.allocate(setType);
    }

    DescriptorSetLayout&
    Allocator::getDescriptorSetLayout(DescriptorSetType setType)
    {
        return this->pool.lookupOrAddLayoutFromCache(setType);
    }
} // namespace gfx::vulkan