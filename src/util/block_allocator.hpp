#ifndef SRC_UTIL_BLOCK_ALLOCATOR_HPP
#define SRC_UTIL_BLOCK_ALLOCATOR_HPP

#include <boost/container/flat_set.hpp>
#include <cstddef>

namespace util
{
    class BlockAllocator
    {
    public:

        class OutOfBlocks : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override
            {
                return "BlockALlocator::OutOfBlocks";
            }
        };

        class FreeOfUntrackedValue : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override
            {
                return "BlockAllocator::FreeOfUntrackedValue";
            }
        };

        class DoubleFree : public std::bad_alloc
        {
            [[nodiscard]] const char* what() const noexcept override
            {
                return "BlockALlocator::DoubleFree";
            }
        };

    public:

        explicit BlockAllocator(std::size_t blocks);
        ~BlockAllocator() = default;

        BlockAllocator(const BlockAllocator&)             = delete;
        BlockAllocator(BlockAllocator&&)                  = default;
        BlockAllocator& operator= (const BlockAllocator&) = delete;
        BlockAllocator& operator= (BlockAllocator&&)      = default;

        std::size_t allocate();
        void        free(std::size_t);

    private:
        boost::container::flat_set<std::size_t> free_block_list;
        std::size_t                             next_available_block;
        std::size_t                             max_number_of_blocks;
    };
} // namespace util

#endif // SRC_UTIL_BLOCK_ALLOCATOR_HPP
