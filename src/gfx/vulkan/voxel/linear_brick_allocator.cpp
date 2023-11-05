#include "linear_brick_allocator.hpp"
#include <util/log.hpp>

namespace gfx::vulkan::voxel
{
    static constexpr std::uint32_t BrickHandleInvalidIndex =
        std::numeric_limits<std::uint32_t>::max();

    BrickHandle::BrickHandle()
        : index {BrickHandleInvalidIndex}
    {}

    BrickHandle::BrickHandle(BrickHandle&& other) noexcept
        : index {other.index}
    {
        other.index = BrickHandleInvalidIndex;
    }

    BrickHandle& BrickHandle::operator= (BrickHandle&& other) noexcept
    {
        this->index = other.index;
        other.index = BrickHandleInvalidIndex;
    }

    bool BrickHandle::isValid() const
    {
        return this->index != BrickHandleInvalidIndex;
    }

    BrickHandle::BrickHandle(std::uint32_t index_)
        : index {index_}
    {}

    LinearBrickAllocator::LinearBrickAllocator(
        std::size_t brickSize, std::size_t numberOfBricks) // NOLINT
        : brick_size {brickSize}
        , number_of_bricks {numberOfBricks}
        , data {this->brick_size * this->number_of_bricks, std::byte {'\0'}}
        , extra_handles {std::vector<BrickHandle> {}}
        , index_of_rest_free {0}
    {}

    BrickHandle LinearBrickAllocator::allocate()
    {
        if (this->extra_handles.empty())
        {
            if (this->index_of_rest_free
                > this->brick_size * this->number_of_bricks)
            {
                util::panic("too not enough room!");
            }

            BrickHandle output {
                static_cast<std::uint32_t>(this->index_of_rest_free)};

            this->index_of_rest_free += this->brick_size;

            return output;
        }
        else
        {
            BrickHandle output {this->extra_handles.back()};

            this->extra_handles.pop_back();

            return output;
        }
    }
    void LinearBrickAllocator::free(BrickHandle handle)
    {
        this->extra_handles.push_back(handle);
    }

    std::span<std::byte>
    LinearBrickAllocator::lookupAllocation(BrickHandle handle) const
    {
        if (!handle.isValid())
        {
            util::panic("Failed to lookup allocation!");
        }

        return std::span<std::byte> {
            const_cast<std::byte*>(&this->data[handle.index]), // NOLINT
            this->brick_size};
    }
} // namespace gfx::vulkan::voxel