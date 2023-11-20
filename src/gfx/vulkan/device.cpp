#include "device.hpp"
#include <magic_enum_all.hpp>
#include <util/log.hpp>
#include <util/threads.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_beta.h>

namespace gfx::vulkan
{
    namespace
    {
        struct QueueCreateInfoAndMetadata
        {
            vk::DeviceQueueCreateInfo queue_create_info;
            std::uint32_t             family_index;
            vk::QueueFlags            flags;
            bool                      supports_surface;
        };
    } // namespace

    Device::Device(vk::Instance instance_, vk::SurfaceKHR surface_) // NOLINT
        : instance {instance_}
        , surface {surface_}
        , logical_device {nullptr}
        , physical_device {nullptr}
        , queues {} // NOLINT
        , should_buffers_stage {true}
    {
        // Initialize physical device
        {
            const std::vector<vk::PhysicalDevice> physicalDevices =
                this->instance.enumeratePhysicalDevices();

            auto rateDevice = [&](vk::PhysicalDevice device) -> std::size_t
            {
                std::size_t score = 0;

                const auto deviceLimits {device.getProperties().limits};

                score += deviceLimits.maxImageDimension2D;
                score += deviceLimits.maxImageDimension3D;

                return score;
            };

            this->physical_device = *std::max_element(
                physicalDevices.begin(),
                physicalDevices.end(),
                [&](vk::PhysicalDevice deviceL, vk::PhysicalDevice deviceR)
                {
                    return rateDevice(deviceL) < rateDevice(deviceR);
                });
        }

        constexpr std::size_t                maxNumberOfQueues {128};
        std::array<float, maxNumberOfQueues> queuePriorites {};
        std::fill(queuePriorites.begin(), queuePriorites.end(), 1.0f);

        // Collect queue information
        std::vector<QueueCreateInfoAndMetadata> metaDataQueueCreateInfos {};
        {
            std::size_t idx = 0;
            for (vk::QueueFamilyProperties p :
                 this->physical_device.getQueueFamilyProperties())
            {
                if (p.queueCount > queuePriorites.size())
                {
                    util::panic(
                        "Found device with too many queues | Found: {} | Max: "
                        "{}",
                        p.queueCount,
                        queuePriorites.size());
                }

                metaDataQueueCreateInfos.push_back(QueueCreateInfoAndMetadata {
                    .queue_create_info {vk::DeviceQueueCreateInfo {
                        .sType {vk::StructureType::eDeviceQueueCreateInfo},
                        .pNext {nullptr},
                        .flags {},
                        .queueFamilyIndex {static_cast<std::uint32_t>(idx)},
                        .queueCount {p.queueCount},
                        .pQueuePriorities {queuePriorites.data()},
                    }},
                    .family_index {static_cast<std::uint32_t>(idx)},
                    .flags {p.queueFlags},
                    .supports_surface {static_cast<bool>(
                        this->physical_device.getSurfaceSupportKHR(
                            static_cast<std::uint32_t>(idx), surface))}});

                ++idx;
            }
        }

        std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos {};
        {
            for (const QueueCreateInfoAndMetadata& q : metaDataQueueCreateInfos)
            {
                deviceQueueCreateInfos.push_back(q.queue_create_info);
            }
        }

        // Create device
        {
            const std::vector<const char*> deviceExtensions {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
                VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif // __APPLE__
            };

            vk::PhysicalDeviceFeatures deviceFeatures = {};
            deviceFeatures.samplerAnisotropy          = vk::True;
            deviceFeatures.fillModeNonSolid           = vk::True;
            deviceFeatures.shaderInt64                = vk::True;

            vk::PhysicalDeviceVulkan12Features device12Features {};
            device12Features.shaderInt8                        = vk::True;
            device12Features.uniformAndStorageBuffer8BitAccess = vk::True;

            vk::DeviceCreateInfo deviceCreateInfo {
                .sType {vk::StructureType::eDeviceCreateInfo},
                .pNext {static_cast<void*>(&device12Features)},
                .flags {},
                .queueCreateInfoCount {
                    static_cast<std::uint32_t>(deviceQueueCreateInfos.size())},
                .pQueueCreateInfos {deviceQueueCreateInfos.data()},
                .enabledLayerCount {0}, // deprecated
                .ppEnabledLayerNames {nullptr},
                .enabledExtensionCount {
                    static_cast<std::uint32_t>(deviceExtensions.size())},
                .ppEnabledExtensionNames {deviceExtensions.data()},
                .pEnabledFeatures {&deviceFeatures},
            };

            this->logical_device =
                this->physical_device.createDeviceUnique(deviceCreateInfo);

            VULKAN_HPP_DEFAULT_DISPATCHER.init(
                this->instance, *this->logical_device);
        }

        // Collect created queues into their respective places
        bool isFirstIteration = true;
        for (const QueueCreateInfoAndMetadata& q : metaDataQueueCreateInfos)
        {
            for (std::size_t i = 0; i < q.queue_create_info.queueCount; ++i)
            {
                constexpr std::array<vk::QueueFlags, 3> testableFlags {
                    vk::QueueFlagBits::eGraphics,
                    vk::QueueFlagBits::eCompute,
                    vk::QueueFlagBits::eTransfer};

                // TODO: surely this can be done better

                std::shared_ptr<Queue> queuePtr = std::make_shared<Queue>(
                    *this->logical_device,
                    this->logical_device->getQueue(
                        q.family_index, static_cast<std::uint32_t>(i)),
                    q.flags,
                    q.supports_surface,
                    q.family_index);

                if (i == 0 && isFirstIteration)
                {
                    for (vk::QueueFlags f : testableFlags)
                    {
                        util::assertFatal(
                            static_cast<bool>(queuePtr->getFlags() & f),
                            "queue 00 must be {} | {}",
                            vk::to_string(f),
                            i);
                    }

                    util::logTrace(
                        "found main grapgics queue {}, i",
                        vk::to_string(
                            testableFlags[0] | testableFlags[1]
                            | testableFlags[2]));

                    this->main_graphics_queue = queuePtr;
                }
                else
                {
                    for (vk::QueueFlags f : testableFlags)
                    {
                        if (queuePtr->getFlags() & f)
                        {
                            util::logTrace(
                                "Added queue of type {}", vk::to_string(f));
                            this->queues[f].push_back(queuePtr);
                        }
                    }
                }

                util::assertWarn(
                    queuePtr.use_count() > 1,
                    "Queue was not allocated | Flags: {}",
                    vk::to_string(queuePtr->getFlags()));
            }

            isFirstIteration = false;
        }

        // sort so that theyre in the least unique order
        for (auto& [flags, queueVector] : this->queues)
        {
            std::ranges::sort(queueVector);
        }

        this->should_buffers_stage = [this]
        {
            auto memoryProperties = this->physical_device.getMemoryProperties();

            std::vector<vk::MemoryType> memoryTypes;
            std::vector<vk::MemoryHeap> memoryHeaps;

            std::optional<std::size_t> idx_of_gpu_main_memory = std::nullopt;

            for (std::size_t i = 0; i < memoryProperties.memoryTypeCount; i++)
            {
                memoryTypes.push_back(memoryProperties.memoryTypes.at(i));
            }

            for (std::size_t i = 0; i < memoryProperties.memoryHeapCount; i++)
            {
                memoryHeaps.push_back(memoryProperties.memoryHeaps.at(i));
            }

            const vk::MemoryPropertyFlags desiredFlags =
                vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent;

            for (auto t : memoryTypes)
            {
                if ((t.propertyFlags & desiredFlags) == desiredFlags)
                {
                    util::assertWarn(
                        !idx_of_gpu_main_memory.has_value(),
                        "There should only be one memory pool with "
                        "`desiredFlags`!");

                    idx_of_gpu_main_memory = t.heapIndex;
                }
            }

            if (idx_of_gpu_main_memory.has_value())
            {
                if (memoryProperties.memoryHeaps
                        .at(idx_of_gpu_main_memory.value())
                        .size
                    > 257 * 1024 * 1024) // NOLINT
                {
                    return false;
                }
            }
            return true;
        }();

        if (!this->should_buffers_stage)
        {
            util::logLog("Resizable BAR support detected");
        }
    }

    Device::~Device() = default;

    bool Device::shouldBuffersStage() const
    {
        return this->should_buffers_stage;
    }

    vk::Device Device::asLogicalDevice() const
    {
        return *this->logical_device;
    }
    vk::PhysicalDevice Device::asPhysicalDevice() const
    {
        return this->physical_device;
    }

    void Device::accessQueue(
        vk::QueueFlags                                           flags,
        const std::function<void(vk::Queue, vk::CommandBuffer)>& func) const
    {
        const std::vector<std::shared_ptr<Queue>>& queueVector =
            this->queues.at(flags);

        while (true)
        {
            for (const std::shared_ptr<Queue>& q : queueVector)
            {
                if (q->tryAccess(func))
                {
                    return;
                }
            }

            util::logWarn(
                "All queues of type {} are full!", vk::to_string(flags));
        }
    }

    const Queue& Device::getMainGraphicsQueue() const
    {
        return *this->main_graphics_queue;
    }

    Queue::Queue(
        vk::Device     device,
        vk::Queue      queue,
        vk::QueueFlags flags_,
        bool           supportsSurface,
        std::uint32_t  queueFamilyIndex)
        : family_index {queueFamilyIndex}
        , flags {flags_}
        , supports_surface {supportsSurface}
        , command_pool {nullptr}
        , queue_buffer_mutex {nullptr}
    {
        const vk::CommandPoolCreateInfo commandPoolCreateInfo {
            .sType {vk::StructureType::eCommandPoolCreateInfo},
            .pNext {nullptr},
            .flags {vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
            .queueFamilyIndex {queueFamilyIndex}};

        this->command_pool =
            device.createCommandPoolUnique(commandPoolCreateInfo);

        const vk::CommandBufferAllocateInfo commandBufferAllocateInfo {
            .sType {vk::StructureType::eCommandBufferAllocateInfo},
            .pNext {nullptr},
            .commandPool {*this->command_pool},
            .level {vk::CommandBufferLevel::ePrimary},
            .commandBufferCount {1}};

        this->queue_buffer_mutex =
            std::make_unique<util::Mutex<vk::Queue, vk::UniqueCommandBuffer>>(
                std::move(queue), // NOLINT: rvalue is required
                std::move(
                    device
                        .allocateCommandBuffersUnique(commandBufferAllocateInfo)
                        .at(0)));
    }

    bool Queue::tryAccess(
        const std::function<void(vk::Queue, vk::CommandBuffer)>& func,
        bool resetCommandBuffer) const
    {
        return this->queue_buffer_mutex->try_lock(
            [&](vk::Queue& queue, vk::UniqueCommandBuffer& commandBuffer)
            {
                if (resetCommandBuffer)
                {
                    commandBuffer->reset();
                }

                func(queue, *commandBuffer);
            });
    }

    std::size_t Queue::getNumberOfOperationsSupported() const
    {
        std::size_t number = 0;

        if (this->supports_surface)
        {
            ++number;
        }

        if (this->flags & vk::QueueFlagBits::eCompute)
        {
            ++number;
        }

        if (this->flags & vk::QueueFlagBits::eGraphics)
        {
            ++number;
        }

        if (this->flags & vk::QueueFlagBits::eTransfer)
        {
            ++number;
        }

        if (this->flags & vk::QueueFlagBits::eSparseBinding)
        {
            ++number;
        }

        return number;
    }

    vk::QueueFlags Queue::getFlags() const
    {
        return this->flags;
    }

    bool Queue::getSurfaceSupport() const
    {
        return this->supports_surface;
    }

    std::uint32_t Queue::getFamilyIndex() const
    {
        return this->family_index;
    }

    std::strong_ordering Queue::operator<=> (const Queue& o) const
    {
        return std::strong_order(
            this->getNumberOfOperationsSupported(),
            o.getNumberOfOperationsSupported());
    }
} // namespace gfx::vulkan