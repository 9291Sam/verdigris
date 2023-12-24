#include "block_allocator.hpp"

namespace util
{
    BlockAllocator::BlockAllocator(std::size_t blocks)
        : next_available_block {0}
        , max_number_of_blocks {blocks}
    {}

    std::size_t BlockAllocator::allocate()
    {
        if (this->free_block_list.empty())
        {
            std::size_t nextFreeBlock = this->next_available_block;

            ++this->next_available_block;

            if (this->next_available_block - 1 >= this->max_number_of_blocks)
            {
                throw OutOfBlocks {};
            }

            return nextFreeBlock;
        }
        else
        {
            std::size_t freeListBlock = *this->free_block_list.rbegin();

            this->free_block_list.erase(freeListBlock);

            return freeListBlock;
        }
    }

    void BlockAllocator::free(std::size_t blockToFree)
    {
        if (blockToFree >= this->max_number_of_blocks)
        {
            throw FreeOfUntrackedValue {};
        }

        const auto& [maybeIterator, valid] =
            this->free_block_list.insert_unique(blockToFree);

        if (!valid)
        {
            throw DoubleFree {};
        }
    }

} // namespace util