#include "buffer.hpp"
#include "allocator.hpp"
#include <chrono>
#include <util/log.hpp>
#include <util/threads.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_enums.hpp>

namespace gfx::vulkan
{
    Buffer::Buffer()
        : allocator {nullptr}
        , buffer {nullptr}
        , allocation {nullptr}
        , size_bytes {~std::size_t {0}}
        , mapped_memory {nullptr}
    {}

    Buffer::Buffer(
        Allocator*              allocator_,
        std::size_t             sizeBytes,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags memoryPropertyFlags)
        : allocator {**allocator_}
        , buffer {nullptr}
        , allocation {nullptr}
        , size_bytes {sizeBytes}
        , mapped_memory {nullptr}
    {
        util::assertFatal(
            !(memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent),
            "Tried to create coherent buffer!");

        const VkBufferCreateInfo bufferCreateInfo {
            .sType {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO},
            .pNext {nullptr},
            .flags {},
            .size {this->size_bytes},
            .usage {static_cast<VkBufferUsageFlags>(usage)},
            .sharingMode {VK_SHARING_MODE_EXCLUSIVE},
            .queueFamilyIndexCount {0},
            .pQueueFamilyIndices {nullptr},
        };

        const VmaAllocationCreateInfo allocationCreateInfo {
            .flags {VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT},
            .usage {VMA_MEMORY_USAGE_AUTO},
            .requiredFlags {
                static_cast<VkMemoryPropertyFlags>(memoryPropertyFlags)},
            .preferredFlags {},
            .memoryTypeBits {},
            .pool {nullptr},
            .pUserData {},
            .priority {},
        };

        VkBuffer outputBuffer = nullptr;

        const vk::Result result {::vmaCreateBuffer(
            this->allocator,
            &bufferCreateInfo,
            &allocationCreateInfo,
            &outputBuffer,
            &this->allocation,
            nullptr)};

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to allocate buffer {} | Size: {}",
            vk::to_string(vk::Result {result}),
            this->size_bytes);

        this->buffer = outputBuffer;
    }

    Buffer::~Buffer()
    {
        this->free();
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : allocator {other.allocator}
        , buffer {other.buffer}
        , allocation {other.allocation}
        , size_bytes {other.size_bytes}
        , mapped_memory {
              other.mapped_memory.exchange(nullptr, std::memory_order_seq_cst)}
    {
        other.allocator  = nullptr;
        other.buffer     = nullptr;
        other.allocation = nullptr;
        other.size_bytes = 0;
    }

    Buffer& Buffer::operator= (Buffer&& other) noexcept
    {
        this->free();

        this->allocator  = other.allocator;
        this->buffer     = other.buffer;
        this->allocation = other.allocation;
        this->size_bytes = other.size_bytes;
        this->mapped_memory.store(
            other.mapped_memory.exchange(nullptr, std::memory_order_seq_cst),
            std::memory_order_seq_cst);

        other.buffer     = nullptr;
        other.allocation = nullptr;
        other.size_bytes = 0;

        return *this;
    }

    vk::Buffer Buffer::operator* () const
    {
        return vk::Buffer {this->buffer};
    }

    std::size_t Buffer::sizeBytes() const
    {
        return this->size_bytes;
    }

    void* Buffer::getMappedPtr() const
    {
        if (this->mapped_memory == nullptr)
        {
            void* outputMappedMemory = nullptr;

            VkResult result = ::vmaMapMemory(
                this->allocator, this->allocation, &outputMappedMemory);

            util::assertFatal(
                result == VK_SUCCESS,
                "Failed to map buffer memory {}",
                vk::to_string(vk::Result {result}));

            util::assertFatal(
                outputMappedMemory != nullptr,
                "Mapped ptr was nullptr! | {}",
                vk::to_string(vk::Result {result}));

            this->mapped_memory.store(
                static_cast<std::byte*>(outputMappedMemory),
                std::memory_order_seq_cst);
        }

        return this->mapped_memory;
    }

    void Buffer::write(std::span<const std::byte> byteSpan) const
    {
        auto start = std::chrono::high_resolution_clock::now();

        util::assertFatal(
            byteSpan.size_bytes() == this->size_bytes,
            "Tried to write {} Bytes of data to a buffer of size {}",
            byteSpan.size_bytes(),
            this->size_bytes);

        if (this->size_bytes < 1 * 1024 * 1024) // NOLINT | 1Mb
        {
            std::memcpy(
                this->getMappedPtr(), byteSpan.data(), byteSpan.size_bytes());
        }
        else
        {
            util::threadedMemcpy(
                static_cast<std::byte*>(this->getMappedPtr()), byteSpan);
        }

        vk::Result result {vmaFlushAllocation(
            this->allocator, this->allocation, 0, this->size_bytes)};

        util::assertFatal(
            result == vk::Result::eSuccess,
            "Failed to flush buffer {}",
            vk::to_string(result));

        auto end = std::chrono::high_resolution_clock::now();

        util::logTrace(
            "Wrote buffer of size {} in {}us",
            this->size_bytes,
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count());
    }

    void
    Buffer::copyFrom(const Buffer& other, vk::CommandBuffer commandBuffer) const
    {
        util::assertFatal(
            this->size_bytes == other.size_bytes,
            "Tried to write a buffer of size {} to a buffer of size {}",
            other.size_bytes,
            this->size_bytes);

        // TODO: this should be things with the same memory pool
        // if not require the pcie extension and throw a warning because bro wtf
        util::assertFatal(
            this->allocator == other.allocator, "Allocators were not the same");

        const vk::BufferCopy bufferCopyParameters {
            .srcOffset {0},
            .dstOffset {0},
            .size {this->size_bytes},
        };

        // SRC -> DST
        commandBuffer.copyBuffer(
            other.buffer, this->buffer, bufferCopyParameters);
    }

    void Buffer::free()
    {
        if (this->allocator == nullptr)
        {
            return;
        }

        if (this->mapped_memory != nullptr)
        {
            ::vmaUnmapMemory(this->allocator, this->allocation);
            this->mapped_memory = nullptr;
        }

        ::vmaDestroyBuffer(this->allocator, this->buffer, this->allocation);
    }

} // namespace gfx::vulkan
