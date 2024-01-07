// #include "voxel.hpp"

// namespace gfx::vulkan::voxel
// {
//     VoxelOrIndex::VoxelOrIndex()
//         : voxel {}
//     {}

//     VoxelOrIndex::VoxelOrIndex(Voxel newVoxel)
//         : voxel {newVoxel}
//     {}

//     VoxelOrIndex::VoxelOrIndex(std::uint32_t newIndex)
//         : _padding {0}
//         , index {newIndex}
//     {
//         if (newIndex == 0)
//         {
//             util::panic("Tried to set a VoxelOrIndex with index 0");
//         }
//     }

//     void VoxelOrIndex::setIndex(std::uint32_t newIndex)
//     {
//         if (newIndex == 0)
//         {
//             util::panic("Tried to set a VoxelOrIndex with index 0");
//         }

//         this->_padding = 0;
//         this->index    = newIndex;
//     }

//     void VoxelOrIndex::setVoxel(Voxel newVoxel)
//     {
//         this->voxel = newVoxel;
//     }

//     std::uint32_t VoxelOrIndex::getIndex() const
//     {
//         if (engine::getSettings()
//                 .lookupSetting<engine::Setting::EnableAppValidation>())
//         {
//             util::assertFatal(
//                 this->isIndex(),
//                 "Tried to access VoxelOrIndex in an invalid state!");
//         }

//         return this->index;
//     }

//     Voxel VoxelOrIndex::getVoxel() const
//     {
//         if (engine::getSettings()
//                 .lookupSetting<engine::Setting::EnableAppValidation>())
//         {
//             util::assertFatal(
//                 this->isVoxel(),
//                 "Tried to access VoxelOrIndex in invalid state!");
//         }

//         return this->voxel;
//     }

//     bool VoxelOrIndex::isVoxel() const
//     {
//         return this->getState() == State::Voxel;
//     }

//     bool VoxelOrIndex::isIndex() const
//     {
//         return this->getState() == State::Index;
//     }

//     bool VoxelOrIndex::isValid() const
//     {
//         return this->getState() != State::Invalid;
//     }

//     VoxelOrIndex::State VoxelOrIndex::getState() const
//     {
//         if (std::bit_cast<std::array<std::byte, sizeof(*this)>>(
//                 *this)[offsetof(Voxel, alpha_or_emissive)]
//             != std::byte {0})
//         {
//             return State::Voxel;
//         }
//         else
//         {
//             if (this->index == 0)
//             {
//                 return State::Invalid;
//             }
//             else
//             {
//                 return State::Index;
//             }
//         }
//     }

// } // namespace gfx::vulkan::voxel
