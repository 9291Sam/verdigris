#include "allocator.hpp"
#include "device.hpp"
#include "instance.hpp"
#include <util/log.hpp>
#include <vk_mem_alloc.h>

namespace gfx::vulkan
{
    Allocator::Allocator(const vulkan::Instance& instance, Device* device_)
        : device {device_}
        , allocator {nullptr}
        , pool {
              device_->asLogicalDevice(),
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
            .physicalDevice {this->device->asPhysicalDevice()},
            .device {this->device->asLogicalDevice()},
            .preferredLargeHeapBlockSize {0}, // chosen by VMA
            .pAllocationCallbacks {nullptr},
            .pDeviceMemoryCallbacks {nullptr},
            .pHeapSizeLimit {nullptr},
            .pVulkanFunctions {&vulkanFunctions},
            .instance {*instance},
            // .vulkanApiVersion {instance.getVulkanVersion()},
            .vulkanApiVersion {VK_API_VERSION_1_0},
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

    Device* Allocator::getOwningDevice() const
    {
        return this->device;
    }
} // namespace gfx::vulkan
