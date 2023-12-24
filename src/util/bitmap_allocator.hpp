#ifndef SRC_UTIL_BITMAP_ALLOCATOR_HPP
#define SRC_UTIL_BITMAP_ALLOCATOR_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace util
{
    class BitmapBlockAllocator
    {
    public:

        using StorageType = std::uint64_t;
        static constexpr std::size_t StorageBits =
            std::bit_width(std::numeric_limits<StorageType>::max());

        class AllocationFailed : public std::bad_alloc
        {};

        class DoubleFree : public std::bad_alloc
        {};

    public:

        explicit BitmapBlockAllocator(std::size_t blocks);
        ~BitmapBlockAllocator() = default;

        BitmapBlockAllocator(const BitmapBlockAllocator&)             = default;
        BitmapBlockAllocator(BitmapBlockAllocator&&)                  = default;
        BitmapBlockAllocator& operator= (const BitmapBlockAllocator&) = default;
        BitmapBlockAllocator& operator= (BitmapBlockAllocator&&)      = default;

        /// Returns the index of the new block
        std::size_t allocateBlock();

        /// Marks the block as free, it can be used again
        void freeBlock(std::size_t);

    private:
        std::vector<StorageType> block_registry;
        // std::size_t              number_of_blocks;
    };
} // namespace util

#endif // SRC_UTIL_BITMAP_ALLOCATOR_HPP