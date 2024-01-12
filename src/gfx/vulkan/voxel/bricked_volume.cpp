// #include "bricked_volume.hpp"
// #include <gfx/vulkan/allocator.hpp>
// #include <glm/gtx/string_cast.hpp>

// namespace gfx::vulkan::voxel
// {
//     namespace
//     {
//         template<class T, std::size_t S1, std::size_t S2, std::size_t S3>
//         void fill3DArray(
//             std::array<std::array<std::array<T, S1>, S2>, S3> arr, T val)
//         {
//             for (std::array<std::array<T, S1>, S2>& yz : arr)
//             {
//                 for (std::array<T, S1>& z : yz)
//                 {
//                     for (T& t : z)
//                     {
//                         t = val;
//                     }
//                 }
//             }
//         }

//         struct PositionIndicies
//         {
//             std::uint32_t brick_pointer_index;
//             Position      voxel_internal_position;
//         };

//         PositionIndicies convertVoxelPositionToIndicies(
//             Position position, std::size_t edgeLengthBricks)
//         {
//             const std::size_t invalidVoxelIndexBound =
//                 edgeLengthBricks * Brick::EdgeLength;

//             util::assertFatal(
//                 glm::all(glm::greaterThanEqual(position, Position {0, 0, 0}))
//                     && glm::all(glm::lessThan(
//                         position,
//                         Position {
//                             invalidVoxelIndexBound,
//                             invalidVoxelIndexBound,
//                             invalidVoxelIndexBound})),
//                 "Tried to access invalid position {} in structure of bound
//                 {}", glm::to_string(position), invalidVoxelIndexBound);

//             Position internalVoxelPosition =
//                 position % static_cast<std::uint64_t>(Brick::EdgeLength);

//             Position brickPosition =
//                 position / static_cast<std::uint64_t>(Brick::EdgeLength);

//             return PositionIndicies {
//                 .brick_pointer_index {static_cast<std::uint32_t>(
//                     brickPosition.x * edgeLengthBricks * edgeLengthBricks
//                     + brickPosition.y * edgeLengthBricks + brickPosition.z)},
//                 .voxel_internal_position {internalVoxelPosition}};
//         }
//     } // namespace

//     BrickedVolume::BrickedVolume(
//         Device* device, Allocator* allocator_, std::size_t edgeLengthVoxels)
//         : allocator {allocator_}
//         , edge_length_bricks {edgeLengthVoxels / Brick::EdgeLength}
//     {
//         util::assertFatal(
//             edgeLengthVoxels % Brick::EdgeLength == 0,
//             "Edge length of {} is not a multiple of {}",
//             edgeLengthVoxels,
//             Brick::EdgeLength);

//         const std::size_t totalNumberOfBricks = this->edge_length_bricks
//                                               * this->edge_length_bricks
//                                               * this->edge_length_bricks;

//         std::size_t brickBufferBricks = std::min(
//             this->edge_length_bricks * this->edge_length_bricks,
//             totalNumberOfBricks);

//         this->locked_data.lock(
//             [&](LockedData& data)
//             {
//                 data = LockedData {
//                     .brick_pointer_buffer {
//                         allocator,
//                         sizeof(VoxelOrIndex) * totalNumberOfBricks,
//                         vk::BufferUsageFlagBits::eStorageBuffer
//                             | vk::BufferUsageFlagBits::eTransferDst,
//                         vk::MemoryPropertyFlagBits::eDeviceLocal,
//                         "Brick Pointer Buffer"},
//                     .brick_pointer_data {totalNumberOfBricks, VoxelOrIndex
//                     {}}, .brick_buffer {
//                         allocator,
//                         brickBufferBricks * sizeof(Brick),
//                         vk::BufferUsageFlagBits::eStorageBuffer
//                             | vk::BufferUsageFlagBits::eTransferSrc
//                             | vk::BufferUsageFlagBits::eTransferDst,
//                         vk::MemoryPropertyFlagBits::eDeviceLocal,
//                         "Brick Buffer"},
//                     .allocator {brickBufferBricks}};
//             });

//         const vk::FenceCreateInfo fenceCreateInfo {
//             .sType {vk::StructureType::eFenceCreateInfo},
//             .pNext {nullptr},
//             .flags {},
//         };

//         vk::UniqueFence endTransferFence =
//             device->asLogicalDevice().createFenceUnique(fenceCreateInfo);

//         device->accessQueue(
//             vk::QueueFlagBits::eTransfer,
//             [&](vk::Queue queue, vk::CommandBuffer commandBuffer)
//             {
//                 const vk::CommandBufferBeginInfo beginInfo {
//                     .sType {vk::StructureType::eCommandBufferBeginInfo},
//                     .pNext {nullptr},
//                     .flags {vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
//                     .pInheritanceInfo {nullptr},
//                 };

//                 commandBuffer.begin(beginInfo);

//                 this->locked_data.lock(
//                     [&](LockedData& data)
//                     {
//                         // fill with invalid brick pointers
//                         data.brick_pointer_buffer.fill(
//                             commandBuffer, std::uint32_t {0});

//                         // fill volumes with invalid Voxel (clear)
//                         data.brick_buffer.fill(
//                             commandBuffer, std::uint32_t {0});
//                     });

//                 commandBuffer.end();

//                 const vk::SubmitInfo submitInfo {
//                     .sType {vk::StructureType::eSubmitInfo},
//                     .pNext {nullptr},
//                     .waitSemaphoreCount {0},
//                     .pWaitSemaphores {nullptr},
//                     .pWaitDstStageMask {nullptr},
//                     .commandBufferCount {1},
//                     .pCommandBuffers {&commandBuffer},
//                     .signalSemaphoreCount {0},
//                     .pSignalSemaphores {nullptr},
//                 };

//                 queue.submit(submitInfo, *endTransferFence);
//             });

//         const vk::Result result = device->asLogicalDevice().waitForFences(
//             *endTransferFence,
//             vk::True,
//             ~std::uint64_t {0});

//         util::assertFatal(
//             result == vk::Result::eSuccess,
//             "Failed to wait for BrickedVolume initialization");
//     }

//     void BrickedVolume::writeVoxel(Position position, Voxel voxelToWrite)
//     const
//     {
//         PositionIndicies indicies =
//             convertVoxelPositionToIndicies(position,
//             this->edge_length_bricks);

//         util::assertWarn(
//             !this->voxel_changes.contains(position),
//             "Multiple Voxel changes are queued for position {}",
//             glm::to_string(position));

//         util::assertWarn(
//             !this->brick_changes.contains(indicies.brick_pointer_index),
//             "Queued Voxel change @ {} overlaps with Brick change at {}",
//             glm::to_string(position),
//             indicies.brick_pointer_index);

//         this->voxel_changes.insert({position, voxelToWrite});
//     }

//     void BrickedVolume::writeVoxel(Position position, const Brick& brick)
//     const
//     {
//         // this does bounds checking and throws otherwise
//         PositionIndicies indicies =
//             convertVoxelPositionToIndicies(position,
//             this->edge_length_bricks);

//         util::assertFatal(
//             indicies.voxel_internal_position == Position {0, 0, 0},
//             "Tried to write unaligned brick at offset {}",
//             glm::to_string(indicies.voxel_internal_position));

//         util::assertWarn(
//             !this->brick_changes.contains(indicies.brick_pointer_index),
//             "Queued multiple writes to brick {}",
//             indicies.brick_pointer_index);

//         this->brick_changes.insert({indicies.brick_pointer_index, brick});
//     }

//     std::size_t BrickedVolume::getEdgeLengthVoxels() const
//     {
//         return this->edge_length_bricks * Brick::EdgeLength;
//     }

//     //! TODO: wow this is a mess
//     bool BrickedVolume::flushToGPU(vk::CommandBuffer commandBuffer)
//     {
//         if (this->brick_changes.empty() && this->voxel_changes.empty())
//         {
//             return false;
//         }

//         util::logTrace(
//             "{} Brick changes and {} voxel changes are queued ",
//             this->brick_changes.size(),
//             this->voxel_changes.size());

//         std::size_t maxUpdates      = 4096;
//         std::size_t maxUpdatesBytes = 4UZ * 1024 * 1024; // 4Mb

//         std::size_t currentUpdatesBytes = 0;
//         std::size_t currentUpdates      = 0;
//         bool        isReallocRequired   = false;
//         bool        reallocOcurred      = false;

//         auto shouldEarlyReturn = [&]
//         {
//             if (isReallocRequired)
//             {
//                 return true;
//             }

//             if (currentUpdates > maxUpdates
//                 || currentUpdatesBytes > maxUpdatesBytes)
//             {
//                 return true;
//             }

//             return false;
//         };

//         auto handleRealloc = [&](LockedData& data)
//         {
//             util::logWarn("Realloc in frame!");

//             // TODO: do something smarter here than a flat 2x?
//             vulkan::Buffer newBrickBuffer {
//                 this->allocator,
//                 data.brick_buffer.sizeBytes() * 2,
//                 vk::BufferUsageFlagBits::eStorageBuffer
//                     | vk::BufferUsageFlagBits::eTransferSrc
//                     | vk::BufferUsageFlagBits::eTransferDst,
//                 vk::MemoryPropertyFlagBits::eDeviceLocal,
//                 "Brick Buffer realloc"};

//             newBrickBuffer.copyFrom(data.brick_buffer, commandBuffer);

//             newBrickBuffer.emitBarrier(
//                 commandBuffer,
//                 vk::AccessFlagBits::eTransferWrite,
//                 vk::AccessFlagBits::eShaderRead,
//                 vk::PipelineStageFlagBits::eTransfer,
//                 vk::PipelineStageFlagBits::eComputeShader);

//             data.maybe_prev_brick_buffer = std::move(data.brick_buffer);
//             data.brick_buffer            = std::move(newBrickBuffer);

//             data.allocator.updateAvailableBlockAmount(
//                 data.brick_buffer.sizeBytes() / sizeof(Brick));
//         };

//         auto propagateChanges = [&](LockedData& data)
//         {
//             // TODO: need to 100% verify that writes dont overlap!
//             auto emplaceNewBrickPointer =
//                 [&] [[nodiscard]] (VoxelOrIndex & maybeBrickPointer)
//             {
//                 util::assertFatal(
//                     !maybeBrickPointer.isIndex(),
//                     "Tried to emplace already functional brick pointer");

//                 std::expected<VoxelOrIndex,
//                 util::BlockAllocator::OutOfBlocks>
//                     maybeNewBrickPointer =
//                         BrickedVolume::allocateNewBrick(data.allocator);

//                 // memory allocation failed, exit the loop
//                 if (!maybeNewBrickPointer.has_value())
//                 {
//                     isReallocRequired = true;
//                     return false;
//                 }

//                 // update cpu side
//                 maybeBrickPointer = *maybeNewBrickPointer;

//                 // update gpu side
//                 commandBuffer.updateBuffer(
//                     *data.brick_pointer_buffer,
//                     static_cast<vk::DeviceSize>(
//                         &maybeBrickPointer - data.brick_pointer_data.data())
//                         * sizeof(VoxelOrIndex),
//                     sizeof(VoxelOrIndex),
//                     &maybeBrickPointer);

//                 currentUpdates += 1;
//                 currentUpdatesBytes += sizeof(VoxelOrIndex);

//                 return true;
//             };

//             auto emplaceBrick =
//                 [&] [[nodiscard]] (const Brick& brick, std::size_t gpuIndex)
//             {
//                 commandBuffer.updateBuffer(
//                     *data.brick_buffer,
//                     gpuIndex * sizeof(Brick),
//                     sizeof(Brick),
//                     &brick);

//                 currentUpdates += 1;
//                 currentUpdatesBytes += sizeof(Brick);
//             };

//             // Brick changes
//             this->brick_changes.erase_if(
//                 [&](const std::pair<std::uint32_t, Brick>& kv) -> bool
//                 {
//                     if (shouldEarlyReturn())
//                     {
//                         return false;
//                     }

//                     const auto& [index, brickToWrite] = kv;
//                     VoxelOrIndex& maybeBrickPointer =
//                         data.brick_pointer_data[index];

//                     // if we don't already have a mapped allocation, we need
//                     // to map one, conveniently since we're writing a whole
//                     // Block, if its a voxel we don't care
//                     if (!maybeBrickPointer.isIndex())
//                     {
//                         if (!emplaceNewBrickPointer(maybeBrickPointer))
//                         {
//                             return false;
//                         }
//                     }

//                     // maybeBrickPointer is a valid brick pointer, so we can
//                     // just jump to its brick and overwrite it
//                     emplaceBrick(brickToWrite, maybeBrickPointer.getIndex());

//                     return true;
//                 });

//             this->voxel_changes.erase_if(
//                 [&](const std::pair<Position, Voxel>& kv) -> bool
//                 {
//                     if (shouldEarlyReturn())
//                     {
//                         return false;
//                     }

//                     const auto& [position, voxelToWrite] = kv;

//                     PositionIndicies indicies =
//                     convertVoxelPositionToIndicies(
//                         position, this->edge_length_bricks);

//                     VoxelOrIndex& maybeBrickPointer =
//                         data.brick_pointer_data[indicies.brick_pointer_index];

//                     if (!maybeBrickPointer.isValid())
//                     {
//                         if (!emplaceNewBrickPointer(maybeBrickPointer))
//                         {
//                             return false;
//                         }
//                     }

//                     // From now on maybeBrickPointer is either an valid
//                     // index or a voxel

//                     // We need to allocate a new volume and bow instead
//                     // of it being empty, we need to fill it
//                     if (maybeBrickPointer.isVoxel())
//                     {
//                         Voxel voxelToFillVolume =
//                         maybeBrickPointer.getVoxel();

//                         if (!emplaceNewBrickPointer(maybeBrickPointer))
//                         {
//                             return false;
//                         }

//                         // Two changes need to be made to the gpu.
//                         // 1. update the pointer buffer (already done above)
//                         // 2. update the brick buffer with a solid brick

//                         // 2
//                         Brick brickToWrite {};
//                         fill3DArray(brickToWrite.voxels, voxelToFillVolume);

//                         emplaceBrick(
//                             brickToWrite, maybeBrickPointer.getIndex());
//                     }

//                     // From now on maybeBrickPointer is a valid index to
//                     // a brick It contains Nothing (if the original
//                     // pointer was invalid) A solid brick of the same
//                     // voxels (was voxel) or otherwise, what was already
//                     // there.
//                     //
//                     // In any case, it's now a valid place to write to,
//                     // so we can do so and place our _single_ voxel in
//                     // memory

//                     const std::size_t brickOffset =
//                         maybeBrickPointer.getIndex() * sizeof(Brick);

//                     const std::size_t voxelOffset =
//                         (indicies.voxel_internal_position.x *
//                         Brick::EdgeLength
//                              * Brick::EdgeLength
//                          + indicies.voxel_internal_position.y
//                                * Brick::EdgeLength
//                          + indicies.voxel_internal_position.z)
//                         * sizeof(Voxel);

//                     const std::size_t voxelOffsetInBrickBuffer =
//                         brickOffset + voxelOffset;

//                     // We now have a valid mapped pointer, so we can
//                     // write the voxel to it and overwrite whatever else
//                     // was already there
//                     commandBuffer.updateBuffer(
//                         *data.brick_buffer,
//                         voxelOffsetInBrickBuffer,
//                         sizeof(Voxel),
//                         &voxelToWrite);

//                     currentUpdates += 1;
//                     currentUpdatesBytes += sizeof(Voxel);

//                     return true;
//                 });

//             util::logTrace(
//                 "Processed {} updates for a total of {} bytes | Realloc?:
//                 {}", currentUpdates, currentUpdatesBytes, isReallocRequired);

//             data.brick_buffer.emitBarrier(
//                 commandBuffer,
//                 vk::AccessFlagBits::eTransferWrite,
//                 vk::AccessFlagBits::eShaderRead,
//                 vk::PipelineStageFlagBits::eTransfer,
//                 vk::PipelineStageFlagBits::eComputeShader);

//             data.brick_pointer_buffer.emitBarrier(
//                 commandBuffer,
//                 vk::AccessFlagBits::eTransferWrite,
//                 vk::AccessFlagBits::eShaderRead,
//                 vk::PipelineStageFlagBits::eTransfer,
//                 vk::PipelineStageFlagBits::eComputeShader);
//         };

//         this->locked_data.lock(
//             [&](LockedData& data)
//             {
//                 isReallocRequired = data.is_realloc_required;

//                 if (isReallocRequired)
//                 {
//                     handleRealloc(data);
//                     reallocOcurred = true;

//                     data.brick_buffer.emitBarrier(
//                         commandBuffer,
//                         vk::AccessFlagBits::eTransferWrite,
//                         vk::AccessFlagBits::eTransferWrite,
//                         vk::PipelineStageFlagBits::eTransfer,
//                         vk::PipelineStageFlagBits::eTransfer);

//                     isReallocRequired = false;
//                 }
//                 else
//                 {
//                     data.maybe_prev_brick_buffer = std::nullopt;
//                 }

//                 propagateChanges(data);

//                 data.is_realloc_required = isReallocRequired;
//             });

//         if (reallocOcurred) // NOLINT
//         {
//             return true; // NOLINT
//         }
//         else
//         {
//             return false;
//         }
//     }

//     BrickedVolume::DrawingBuffers BrickedVolume::getBuffers()
//     {
//         return this->locked_data.lock(
//             [&](LockedData& data)
//             {
//                 return DrawingBuffers {
//                     .brick_pointer_buffer {*data.brick_pointer_buffer},
//                     .brick_buffer {*data.brick_buffer},
//                 };
//             });
//     }

//     std::expected<VoxelOrIndex, util::BlockAllocator::OutOfBlocks>
//     BrickedVolume::allocateNewBrick(util::BlockAllocator& allocator)
//     {
//         std::expected<std::size_t, util::BlockAllocator::OutOfBlocks>
//             allocateResult = allocator.allocate();

//         if (!allocateResult.has_value())
//         {
//             return std::unexpected(std::move(allocateResult.error()));
//         }

//         std::uint32_t maybeValidNewIndex =
//             static_cast<std::uint32_t>(*allocateResult);

//         // the index must be non zero as 0 is used as a null value
//         if (maybeValidNewIndex == 0)
//         {
//             allocateResult = allocator.allocate();

//             if (!allocateResult.has_value())
//             {
//                 return std::unexpected(std::move(allocateResult.error()));
//             }

//             maybeValidNewIndex = static_cast<std::uint32_t>(*allocateResult);
//         }

//         // util::logTrace("Allocated block at {}", maybeValidNewIndex);

//         return VoxelOrIndex {maybeValidNewIndex};
//     }

// } // namespace gfx::vulkan::voxel
