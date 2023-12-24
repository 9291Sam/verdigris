#include "bricked_volume.hpp"
#include <gfx/vulkan/allocator.hpp>
#include <glm/gtx/string_cast.hpp>

namespace gfx::vulkan::voxel
{
    namespace
    {
        template<class T, std::size_t S1, std::size_t S2, std::size_t S3>
        void fill3DArray(
            std::array<std::array<std::array<T, S1>, S2>, S3> arr, T val)
        {
            for (std::array<std::array<T, S1>, S2>& yz : arr)
            {
                for (std::array<T, S1>& z : yz)
                {
                    for (T& t : z)
                    {
                        t = val;
                    }
                }
            }
        }

        struct PositionIndicies
        {
            std::uint32_t brick_pointer_index;
            Position      voxel_internal_position;
        };

        PositionIndicies convertVoxelPositionToIndicies(
            Position position, std::size_t edgeLengthBricks)
        {
            const std::size_t invalidVoxelIndexBound =
                edgeLengthBricks * Brick::EdgeLength;

            util::assertFatal(
                glm::all(glm::greaterThanEqual(position, Position {0, 0, 0}))
                    && glm::all(glm::lessThan(
                        position,
                        Position {
                            invalidVoxelIndexBound,
                            invalidVoxelIndexBound,
                            invalidVoxelIndexBound})),
                "Tried to access invalid position {} in structure of bound {}",
                glm::to_string(position),
                invalidVoxelIndexBound);

            Position internalVoxelPosition = position % Brick::EdgeLength;

            Position brickPosition = position / Brick::EdgeLength;

            return PositionIndicies {
                .brick_pointer_index {static_cast<std::uint32_t>(
                    brickPosition.x * edgeLengthBricks * edgeLengthBricks
                    + brickPosition.y * edgeLengthBricks + brickPosition.z)},
                .voxel_internal_position {internalVoxelPosition}};
        }
    } // namespace

    BrickedVolume::BrickedVolume(
        Device* device, Allocator* allocator, std::size_t edgeLengthVoxels)
        : edge_length_bricks {edgeLengthVoxels / Brick::EdgeLength}
        , locked_data {util::Mutex {LockedData {
              .allocator {// TODO: move elsewhere?
                          this->edge_length_bricks * this->edge_length_bricks
                          * this->edge_length_bricks}}}}
    {
        util::assertFatal(
            edgeLengthVoxels % Brick::EdgeLength == 0,
            "Edge length of {} is not a multiple of {}",
            edgeLengthVoxels,
            Brick::EdgeLength);

        const std::size_t numberOfBricks = this->edge_length_bricks
                                         * this->edge_length_bricks
                                         * this->edge_length_bricks;

        this->locked_data.lock(
            [&](LockedData& data)
            {
                data.brick_pointer_buffer = vulkan::Buffer {
                    allocator,
                    sizeof(VoxelOrIndex) * numberOfBricks,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eTransferDst,
                    vk::MemoryPropertyFlagBits::eDeviceLocal};

                data.brick_pointer_data.resize(numberOfBricks, VoxelOrIndex {});

                data.brick_buffer = vulkan::Buffer {
                    allocator,
                    sizeof(Brick) * numberOfBricks,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eTransferDst,
                    vk::MemoryPropertyFlagBits::eDeviceLocal};

                const vk::FenceCreateInfo fenceCreateInfo {
                    .sType {vk::StructureType::eFenceCreateInfo},
                    .pNext {nullptr},
                    .flags {},
                };

                vk::UniqueFence endTransferFence =
                    device->asLogicalDevice().createFenceUnique(
                        fenceCreateInfo);

                device->accessQueue(
                    vk::QueueFlagBits::eTransfer,
                    [&](vk::Queue queue, vk::CommandBuffer commandBuffer)
                    {
                        const vk::CommandBufferBeginInfo beginInfo {
                            .sType {vk::StructureType::eCommandBufferBeginInfo},
                            .pNext {nullptr},
                            .flags {
                                vk::CommandBufferUsageFlagBits::eOneTimeSubmit},
                            .pInheritanceInfo {nullptr},
                        };

                        commandBuffer.begin(beginInfo);

                        // fill with invalid brick pointers
                        data.brick_pointer_buffer.fill(
                            commandBuffer, std::uint32_t {0});

                        // fill volumes with invalid Voxel (clear)
                        data.brick_buffer.fill(
                            commandBuffer, std::uint32_t {0});

                        commandBuffer.end();

                        const vk::SubmitInfo submitInfo {
                            .sType {vk::StructureType::eSubmitInfo},
                            .pNext {nullptr},
                            .waitSemaphoreCount {0},
                            .pWaitSemaphores {nullptr},
                            .pWaitDstStageMask {nullptr},
                            .commandBufferCount {1},
                            .pCommandBuffers {&commandBuffer},
                            .signalSemaphoreCount {0},
                            .pSignalSemaphores {nullptr},
                        };

                        queue.submit(submitInfo, *endTransferFence);
                    });

                const vk::Result result =
                    device->asLogicalDevice().waitForFences(
                        *endTransferFence,
                        static_cast<vk::Bool32>(true),
                        ~std::uint64_t {0});

                util::assertFatal(
                    result == vk::Result::eSuccess,
                    "Failed to wait for BrickedVolume initialization");
            });

        util::logLog(
            "Initalized BrickedVolume of size {}^3",
            this->edge_length_bricks * Brick::EdgeLength);

        this->locked_data.lock(
            [&](LockedData& data)
            {
                util::assertFatal(
                    *data.brick_buffer != nullptr,
                    "Brick buffer was not initalized");

                util::assertFatal(
                    *data.brick_pointer_buffer != nullptr,
                    "Brick buffer was not initalized");

                util::logTrace(
                    "BrickedVolume buffers {} {}",
                    (void*)*data.brick_buffer,
                    (void*)*data.brick_pointer_buffer);
            });
    }

    void BrickedVolume::writeVoxel(Position position, Voxel voxelToWrite) const
    {
        PositionIndicies indicies =
            convertVoxelPositionToIndicies(position, this->edge_length_bricks);

        util::assertWarn(
            !this->voxel_changes.contains(position),
            "Multiple Voxel changes are queued for position {}",
            glm::to_string(position));

        util::assertWarn(
            !this->brick_changes.contains(indicies.brick_pointer_index),
            "Queued Voxel change @ {} overlaps with Brick change at {}",
            glm::to_string(position),
            indicies.brick_pointer_index);

        this->voxel_changes.insert({position, voxelToWrite});
    }

    void BrickedVolume::writeVoxel(Position position, const Brick& brick) const
    {
        // this does bounds checking and throws otherwise
        PositionIndicies indicies =
            convertVoxelPositionToIndicies(position, this->edge_length_bricks);

        util::assertFatal(
            indicies.voxel_internal_position == Position {0, 0, 0},
            "Tried to write unaligned brick at offset {}",
            glm::to_string(indicies.voxel_internal_position));

        util::assertWarn(
            !this->brick_changes.contains(indicies.brick_pointer_index),
            "Queued multiple writes to brick {}",
            indicies.brick_pointer_index);

        this->brick_changes.insert({indicies.brick_pointer_index, brick});
    }

    // TODO: do stuff with a dedicated transfer queue
    void BrickedVolume::flushToGPU(vk::CommandBuffer commandBuffer)
    {
        if (this->brick_changes.empty() && this->voxel_changes.empty())
        {
            return;
        }

        util::logTrace(
            "{} Brick changes and {} voxel changes are queued ",
            this->brick_changes.size(),
            this->voxel_changes.size());

        // TODO: seperate into brick and voxel updates
        const std::size_t maxPerFrameUpdates = 256;

        this->locked_data.lock(
            [&](LockedData& data)
            {
                // TODO: need to 100% verify that writes dont overlap!
                std::size_t commandsQueued = 0;

                auto shouldEarlyReturn = [&]
                {
                    return commandsQueued > maxPerFrameUpdates;
                };

                this->brick_changes.erase_if(
                    [&](const std::pair<std::uint32_t, Brick>& kv) -> bool
                    {
                        if (shouldEarlyReturn())
                        {
                            return false;
                        }

                        const auto& [index, brickToWrite] = kv;
                        VoxelOrIndex& maybeBrickPointer =
                            data.brick_pointer_data[index];

                        if (!maybeBrickPointer.isValid())
                        {
                            // The brick pointer we have is invalid, that
                            // means we need to populate it
                            maybeBrickPointer =
                                BrickedVolume::allocateNewBrick(data.allocator);

                            commandsQueued++;

                            // propagate this change to the gpu
                            commandBuffer.updateBuffer(
                                *data.brick_pointer_buffer,
                                static_cast<vk::DeviceSize>(
                                    &maybeBrickPointer
                                    - data.brick_pointer_data.data())
                                    * sizeof(VoxelOrIndex),
                                sizeof(VoxelOrIndex),
                                &maybeBrickPointer);
                        }

                        // We now have a valid mapped pointer, so we can
                        // write the new data to it

                        commandsQueued++;

                        commandBuffer.updateBuffer(
                            *data.brick_buffer,
                            maybeBrickPointer.getIndex() * sizeof(Brick),
                            sizeof(Brick),
                            &brickToWrite);

                        return true;
                    });

                this->voxel_changes.erase_if(
                    [&](const std::pair<Position, Voxel>& kv) -> bool
                    {
                        if (shouldEarlyReturn())
                        {
                            return false;
                        }

                        const auto& [position, voxelToWrite] = kv;

                        PositionIndicies indicies =
                            convertVoxelPositionToIndicies(
                                position, this->edge_length_bricks);

                        VoxelOrIndex& maybeBrickPointer =
                            data.brick_pointer_data[indicies
                                                        .brick_pointer_index];

                        if (!maybeBrickPointer.isValid())
                        {
                            // The brick pointer we have is invalid, that
                            // means we need to populate it
                            maybeBrickPointer =
                                BrickedVolume::allocateNewBrick(data.allocator);

                            ++commandsQueued;

                            // propagate this change to the gpu
                            commandBuffer.updateBuffer(
                                *data.brick_pointer_buffer,
                                static_cast<vk::DeviceSize>(
                                    &maybeBrickPointer
                                    - data.brick_pointer_data.data())
                                    * sizeof(VoxelOrIndex),
                                sizeof(VoxelOrIndex),
                                &maybeBrickPointer);
                        }

                        // From now on maybeBrickPointer is either an valid
                        // index or a voxel

                        // We need to allocate a new volume and bow instead
                        // of it being empty, we need to fill it
                        if (maybeBrickPointer.isVoxel())
                        {
                            Voxel voxelToFillVolume =
                                maybeBrickPointer.getVoxel();

                            maybeBrickPointer =
                                BrickedVolume::allocateNewBrick(data.allocator);

                            // Two changes need to be made to the gpu.
                            // 1. update the pointer buffer
                            // 2. update the brick buffer with a solid brick

                            commandsQueued++;

                            // 1
                            commandBuffer.updateBuffer(
                                *data.brick_pointer_buffer,
                                static_cast<vk::DeviceSize>(
                                    &maybeBrickPointer
                                    - data.brick_pointer_data.data())
                                    * sizeof(VoxelOrIndex),
                                sizeof(VoxelOrIndex),
                                &maybeBrickPointer);

                            // 2
                            Brick brickToWrite {};
                            fill3DArray(brickToWrite.voxels, voxelToFillVolume);

                            commandsQueued++;
                            commandBuffer.updateBuffer(
                                *data.brick_buffer,
                                maybeBrickPointer.getIndex() * sizeof(Brick),
                                sizeof(Brick),
                                &brickToWrite);
                        }

                        // From now on maybeBrickPointer is a valid index to
                        // a brick It contains Nothing (if the original
                        // pointer was invalid) A solid brick of the same
                        // voxels (was voxel) or otherwise, what was already
                        // there.
                        //
                        // In any case, it's now a valid place to write to,
                        // so we can do so and place our _single_ voxel in
                        // memory

                        const std::size_t brickOffset =
                            maybeBrickPointer.getIndex() * sizeof(Brick);

                        const std::size_t voxelOffset =
                            (indicies.voxel_internal_position.x
                                 * Brick::EdgeLength * Brick::EdgeLength
                             + indicies.voxel_internal_position.y
                                   * Brick::EdgeLength
                             + indicies.voxel_internal_position.z)
                            * sizeof(Voxel);

                        const std::size_t voxelOffsetInBrickBuffer =
                            brickOffset + voxelOffset;

                        // We now have a valid mapped pointer, so we can
                        // write the voxel to it and ignore whatever else
                        // was already there
                        ++commandsQueued;

                        commandBuffer.updateBuffer(
                            *data.brick_buffer,
                            voxelOffsetInBrickBuffer,
                            sizeof(Voxel),
                            &voxelToWrite);

                        return true;
                    });

                util::logTrace("Processed {} changes", commandsQueued);
            });
    }

    BrickedVolume::DrawingBuffers BrickedVolume::getBuffers()
    {
        DrawingBuffers retval {};

        this->locked_data.lock(
            [&](LockedData& data)
            {
                retval = DrawingBuffers {
                    .brick_pointer_buffer {*data.brick_pointer_buffer},
                    .brick_buffer {*data.brick_buffer},
                };
            });

        return retval;
    }

    VoxelOrIndex
    BrickedVolume::allocateNewBrick(util::BlockAllocator& allocator)
    {
        std::uint32_t maybeValidNewIndex =
            static_cast<std::uint32_t>(allocator.allocate());

        // the index must be non zero
        if (maybeValidNewIndex == 0)
        {
            // leak 0 and try again
            maybeValidNewIndex = allocator.allocate();
        }

        return VoxelOrIndex {maybeValidNewIndex};
    }

} // namespace gfx::vulkan::voxel
