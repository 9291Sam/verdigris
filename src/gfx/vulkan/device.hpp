#ifndef SRC_GFX_VULKAN_DEVICE_HPP
#define SRC_GFX_VULKAN_DEVICE_HPP

#include <functional>
#include <map>
#include <util/threads.hpp>
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

        [[nodiscard]] bool               shouldBuffersStage() const;
        [[nodiscard]] vk::PhysicalDevice asPhysicalDevice() const;
        [[nodiscard]] vk::Device         asLogicalDevice() const;

        void accessQueue(
            vk::QueueFlags,
            const std::function<void(vk::Queue, vk::CommandBuffer)>&) const;
        [[nodiscard]] const Queue& getMainGraphicsQueue() const;

    private:
        vk::Instance   instance;
        vk::SurfaceKHR surface;

        vk::UniqueDevice   logical_device;
        vk::PhysicalDevice physical_device;

        std::map<vk::QueueFlags, std::vector<std::shared_ptr<Queue>>> queues;
        std::shared_ptr<Queue> main_graphics_queue;
        bool                   should_buffers_stage;
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

        // TODO: this abstraction is shit, passing false to this is just yuck
        [[nodiscard]] bool tryAccess(
            const std::function<void(vk::Queue, vk::CommandBuffer)>&,
            bool resetCommandBuffer = true) const;
        [[nodiscard]] bool           isInUse() const;
        [[nodiscard]] std::size_t    getNumberOfOperationsSupported() const;
        [[nodiscard]] vk::QueueFlags getFlags() const;
        [[nodiscard]] bool           getSurfaceSupport() const;
        [[nodiscard]] std::uint32_t  getFamilyIndex() const;

        std::strong_ordering operator<=> (const Queue&) const;

    private:
        vk::Device            device;
        std::uint32_t         family_index;
        vk::QueueFlags        flags;
        bool                  supports_surface;
        vk::UniqueCommandPool command_pool;
        std::unique_ptr<util::Mutex<vk::Queue, vk::UniqueCommandBuffer>>
            queue_buffer_mutex;
    };
} // namespace gfx::vulkan

#endif // SRC_GFX_VULKAN_DEVICE_HPP
