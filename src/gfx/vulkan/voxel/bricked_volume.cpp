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
            std::size_t brick_pointer_index;
            Position    voxel_internal_position;
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
                .brick_pointer_index {
                    brickPosition.x * edgeLengthBricks * edgeLengthBricks
                    + brickPosition.y * edgeLengthBricks + brickPosition.z},
                .voxel_internal_position {internalVoxelPosition}};
        }
    } // namespace

    BrickedVolume::BrickedVolume(
        Device* device, Allocator* allocator, std::size_t edgeLengthVoxels)
        : edge_length_bricks {edgeLengthVoxels / Brick::EdgeLength}
        , locked_data {LockedData {}}
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
                    vk::BufferUsageFlagBits::eStorageBuffer,
                    vk::MemoryPropertyFlagBits::eDeviceLocal};

                data.brick_buffer = vulkan::Buffer {
                    allocator,
                    sizeof(Brick) * numberOfBricks,
                    vk::BufferUsageFlagBits::eStorageBuffer,
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
                        *endTransferFence, static_cast<vk::Bool32>(true), -1);

                util::assertFatal(
                    result == vk::Result::eSuccess,
                    "Failed to wait for BrickedVolume initialization");
            });
    }

    void BrickedVolume::writeVoxel(Position position, Voxel voxelToWrite) const
    {
        this->locked_data.lock(
            [&](LockedData& data)
            {
                // this does bounds checking and throws otherwise
                std::ignore = convertVoxelPositionToIndicies(
                    position, this->edge_length_bricks);

                // TODO: use a compute pass to find empty and/or all identical
                // blocks.

                // One of three things
                // Voxel (the brick is all the same voxel)
                // Valid Index
                // Invalid Index
                VoxelOrIndex& maybeBrickPointer =
                    data.brick_pointer_span[accessPosition.brick_pointer_index];

                if (!maybeBrickPointer.isValid())
                {
                    // The brick pointer we have is invalid, that means we need
                    // to populate it
                    maybeBrickPointer = this->allocateNewBrick();
                }

                // From now on maybeBrickPointer is either an valid index or a
                // voxel

                // We need to allocate a new volume and fill it
                if (maybeBrickPointer.isVoxel())
                {
                    Voxel voxelToFillVolume = maybeBrickPointer.getVoxel();

                    maybeBrickPointer = this->allocateNewBrick();

                    fill3DArray(
                        data.brick_span[maybeBrickPointer.getIndex()].voxels,
                        voxelToFillVolume);
                }

                // TODO: sort out the overlap between invalid brick and a brick
                // filled with air

                // From now on maybeBrickPointer is a valid index to a brick
                // containing either nothing                   (if the
                // brickPointer was invalid) all filled with one voxel (if the
                // pointer was a voxel) what was already there    (otherwise)
                std::uint32_t brickPointer = maybeBrickPointer.getIndex();
                data.brick_pointer_span
                    [brickPointer][accessPosition.voxel_internal_position] =
                    voxelToWrite;
            });

        // ok now we need to update the gpu...
    }

    Voxel BrickedVolume::readVoxel(Position position) const
    {
        Voxel retval {};

        this->locked_data.lock(
            [&](LockedData& data)
            {
                PositionIndicies accessPosition =
                    convertVoxelPositionToIndicies(
                        position, this->edge_length_bricks);

                VoxelOrIndex maybeBrickPointer = data.brick_pointer_data.at(
                    accessPosition.brick_pointer_index);

                util::assertFatal(
                    maybeBrickPointer.isIndex(),
                    "Tried to access invalid Voxel");

                retval =
                    data.brick_data[maybeBrickPointer.getIndex()]
                                   [accessPosition.voxel_internal_position];
            });

        return retval;
    }

    void BrickedVolume::updateGPU()
    {
        util::logTrace("Flushing {} bricks to gpu");

        for (const auto& [updateIndex, brick] : this->changes)
        {
            commandBuffer.
        }

        this->locked_data.lock(
            [&](LockedData& data)
            {
                data.brick_pointer_buffer.write(
                    std::as_bytes(std::span {data.brick_pointer_data}));

                data.brick_buffer.write(
                    std::as_bytes(std::span {data.brick_data}));
            });
    }

    BrickedVolume::DrawingArrays BrickedVolume::draw()
    {
        DrawingArrays retval {};

        this->locked_data.lock(
            [&](LockedData& data)
            {
                return DrawingArrays {
                    .brick_pointer_buffer {&data.brick_pointer_buffer},
                    .brick_buffer {&data.brick_buffer},
                };
            });

        return retval;
    }

    VoxelOrIndex BrickedVolume::allocateNewBrick() const
    {
        this->locked_data.lock(
            [&](LockedData& data)
            {
                util::panic("Unfinished");
                // if we need to realloc
                if (data.next_free_brick_index + 1 == data.brick_data.size())
                {}
                else
                {
                    VoxelOrIndex result {
                        static_cast<std::uint32_t>(data.next_free_brick_index)};

                    // increment index for next
                    ++data.next_free_brick_index;

                    return result;
                }
            });
    }

} // namespace gfx::vulkan::voxel