#include "block_allocator.hpp"

namespace util
{
    const char* BlockAllocator::OutOfBlocks::what() const noexcept
    {
        return "BlockAllocator::OutOfBlocks";
    }

    const char* BlockAllocator::FreeOfUntrackedValue::what() const noexcept
    {
        return "BlockAllocator::FreeOfUntrackedValue";
    }

    const char* BlockAllocator::DoubleFree::what() const noexcept
    {
        return "BlockAllocator::DoubleFree";
    }

    BlockAllocator::BlockAllocator(std::size_t blocks)
        : next_available_block {0}
        , max_number_of_blocks {blocks}
    {}

    BlockAllocator::BlockAllocator(BlockAllocator&& other) noexcept
        : free_block_list {std::move(other.free_block_list)}
        , next_available_block {other.next_available_block}
        , max_number_of_blocks {other.max_number_of_blocks}
    {
        other.free_block_list      = {};
        other.next_available_block = -1;
        other.max_number_of_blocks = 0;
    }

    BlockAllocator& BlockAllocator::operator= (BlockAllocator&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        this->~BlockAllocator();

        new (this) BlockAllocator {std::move(other)};

        return *this;
    }

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
