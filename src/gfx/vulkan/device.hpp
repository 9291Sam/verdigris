#ifndef SRC_GFX_VULKAN_DEVICE_HPP
#define SRC_GFX_VULKAN_DEVICE_HPP

#include <functional>
#include <map>
#include <util/misc.hpp>
#include <vulkan/vulkan_format_traits.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace gfx::vulkan
{
    class Instance;
    class Queue;

    class Device
    {
    public:

        Device(vk::Instance, vk::SurfaceKHR);
        ~Device();

        Device(const Device&)             = delete;
        Device(Device&&)                  = delete;
        Device& operator= (const Device&) = delete;
        Device& operator= (Device&&)      = delete;

        [[nodiscard]] bool shouldBuffersStage() const;

        [[nodiscard]] vk::Device         asLogicalDevice() const;
        [[nodiscard]] vk::PhysicalDevice asPhysicalDevice() const;

        void accessQueue(
            vk::QueueFlags, std::function<void(vk::Queue, vk::CommandBuffer)>);

    private:
        vk::Instance   instance;
        vk::SurfaceKHR surface;

        vk::UniqueDevice   logical_device;
        vk::PhysicalDevice physical_device;

        std::map<vk::QueueFlags, std::vector<std::shared_ptr<Queue>>> queues;
        bool should_buffers_stage;
    };

    class Queue
    {
    public:

        Queue(
            vk::Device,
            vk::Queue,
            vk::QueueFlags,
            bool          supportsSurface,
            std::uint32_t queueFamilyIndex);
        ~Queue() = default;

        Queue(const Queue&)             = delete;
        Queue(Queue&&)                  = delete;
        Queue& operator= (const Queue&) = delete;
        Queue& operator= (Queue&&)      = delete;

        bool tryAccess(std::function<void(vk::Queue, vk::CommandBuffer)>) const;
        bool isInUse() const;
        std::size_t    getNumberOfOperationsSupported() const;
        vk::QueueFlags getFlags() const;
        bool           getSurfaceSupport() const;

        std::strong_ordering operator<=> (const Queue& o) const;

    private:
        vk::QueueFlags        flags;
        bool                  supports_surface;
        vk::UniqueCommandPool command_pool;
        std::unique_ptr<util::Mutex<vk::Queue, vk::UniqueCommandBuffer>>
            queue_buffer_mutex;
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_DEVICE_HPP