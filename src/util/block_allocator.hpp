#ifndef SRC_UTIL_BLOCK_ALLOCATOR_HPP
#define SRC_UTIL_BLOCK_ALLOCATOR_HPP

#include <boost/container/flat_set.hpp>
#include <cstddef>

namespace util
{
    /// Allocates unique, single integers.
    /// Useful for allocating fixed sized chunks of memory
    class BlockAllocator
    {
    public:

        class OutOfBlocks : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override;
        };

        class FreeOfUntrackedValue : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override;
        };

        class DoubleFree : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override;
        };

    public:

        // Creates a new BlockAllocator that can allocate blocks at indicies in
        // the range [0, blocks)
        // BlockAllocator(128) -> [0, 127]
        explicit BlockAllocator(std::size_t blocks);
        ~BlockAllocator() = default;

        BlockAllocator(const BlockAllocator&) = delete;
        BlockAllocator(BlockAllocator&&) noexcept;
        BlockAllocator& operator= (const BlockAllocator&) = delete;
        BlockAllocator& operator= (BlockAllocator&&) noexcept;

        std::size_t allocate();
        void        free(std::size_t);

    private:
        boost::container::flat_set<std::size_t> free_block_list;
        std::size_t                             next_available_block;
        std::size_t                             max_number_of_blocks;
    };
} // namespace util

#endif // SRC_UTIL_BLOCK_ALLOCATOR_HPP
