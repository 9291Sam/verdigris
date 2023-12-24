#include "bitmap_allocator.hpp"
#include <util/log.hpp>
#include <util/misc.hpp>

namespace util
{

    BitmapBlockAllocator::BitmapBlockAllocator(std::size_t blocks)
    {
        this->block_registry.resize(
            util::ceilingDivide(blocks, StorageBits), 0);

        util::assertFatal(
            blocks % StorageBits == 0,
            "Blocks must be a multiple of {}",
            StorageBits);
    }

    std::size_t BitmapBlockAllocator::allocateBlock()
    {
        // rIdx = registryIndex;
        for (std::size_t rIdx = 0; rIdx < this->block_registry.size(); ++rIdx)
        {
            StorageType& block = this->block_registry[rIdx];

            if (block != ~(StorageType {}))
            {
                // There's an unoccupied thing in this registry

                // bIdx blockIndex
                for (std::size_t bIdx = 0; bIdx < StorageBits; ++bIdx)
                {
                    bool isBitSet = !!static_cast<bool>((block >> bIdx) & 1UZ);

                    if (!isBitSet) // there's a zero here, this one is free
                    {
                        // set the bit as now used
                        block |= (1UZ << bIdx);

                        return rIdx * StorageBits + bIdx;
                    }
                }
            }
        }

        throw AllocationFailed {};
    }

    void BitmapBlockAllocator::freeBlock(std::size_t idxToFree)
    {
        std::size_t blockIdx = idxToFree / StorageBits;
        std::size_t localIdx = idxToFree % StorageBits;

        StorageType& storageBlock = this->block_registry[blockIdx];
        bool         isBlockOccupied =
            !!static_cast<bool>((storageBlock >> localIdx) & 1UZ);

        if (isBlockOccupied)
        {
            // set the bit as now free
            storageBlock ^= (1UZ << localIdx);

            return;
        }
        else
        {
            throw DoubleFree {};
        }
    }
} // namespace util