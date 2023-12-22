#include "voxel.hpp"

namespace gfx::vulkan::voxel
{
    VoxelOrIndex::VoxelOrIndex(Voxel newVoxel)
        : voxel {newVoxel}
    {}

    VoxelOrIndex::VoxelOrIndex(std::uint32_t newIndex)
        : index {newIndex}
    {}

    void VoxelOrIndex::setIndex(std::uint32_t newIndex)
    {
        this->index = newIndex;
    }

    void VoxelOrIndex::setVoxel(Voxel newVoxel)
    {
        this->voxel = newVoxel;
    }

    std::uint32_t VoxelOrIndex::getIndex() const
    {
        if (engine::getSettings()
                .lookupSetting<engine::Setting::EnableAppValidation>())
        {
            util::assertFatal(
                !this->isVoxel(),
                "Tried to access VoxelOrIndex in invalid state!");
        }

        return this->index;
    }

    Voxel VoxelOrIndex::getVoxel() const
    {
        if (!std::is_constant_evaluated()
            && engine::getSettings()
                   .lookupSetting<engine::Setting::EnableAppValidation>())
        {
            util::assertFatal(
                this->isVoxel(),
                "Tried to access VoxelOrIndex in invalid state!");
        }

        return this->voxel;
    }

    bool VoxelOrIndex::isVoxel() const
    {
        // alpha_or_emissive or emissive is 0, this is an index
        if (std::bit_cast<std::array<std::byte, sizeof(*this)>>(
                *this)[offsetof(Voxel, alpha_or_emissive)]
            == std::byte {0})
        {
            return false; // NOLINT
        }
        else // alpha_or_emissive is non zero, this is a voxel
        {
            return true;
        }
    }
} // namespace gfx::vulkan::voxel