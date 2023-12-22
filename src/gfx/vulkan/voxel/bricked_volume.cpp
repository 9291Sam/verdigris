#include "bricked_volume.hpp"
#include <gfx/vulkan/allocator.hpp>

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
        Allocator* allocator, std::size_t edgeLengthVoxels)
        : edge_length_bricks {edgeLengthVoxels / Brick::EdgeLength}
        , next_free_brick_index {0}
    {
        util::assertFatal(
            edgeLengthVoxels % Brick::EdgeLength == 0,
            "Edge length of {} is not a multiple of {}",
            edgeLengthVoxels,
            Brick::EdgeLength);

        const std::size_t numberOfBricks = this->edge_length_bricks
                                         * this->edge_length_bricks
                                         * this->edge_length_bricks;

        {
            this->brick_pointer_buffer = vulkan::Buffer {
                allocator,
                sizeof(VoxelOrIndex) * numberOfBricks,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
                    | vk::MemoryPropertyFlagBits::eHostVisible};

            this->brick_pointer_data.resize(
                numberOfBricks, VoxelOrIndex {~std::uint32_t {0}});

            this->brick_pointer_buffer.write(
                std::as_bytes(std::span {this->brick_pointer_data}));
        }

        {
            this->brick_buffer = vulkan::Buffer {
                allocator,
                sizeof(Brick) * numberOfBricks,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eDeviceLocal
                    | vk::MemoryPropertyFlagBits::eHostVisible};

            this->brick_data.resize(numberOfBricks, Brick {Voxel {}});

            this->brick_buffer.write(
                std::as_bytes(std::span {this->brick_data}));
        }

        util::panic("add asserts and esure throws for VoxelOrIndex");
    }

    // TODO: make this const and concurrent, this can totally be done with
    // atomics :)
    void BrickedVolume::writeVoxel(Position position, Voxel voxelToWrite)
    {
        PositionIndicies accessPosition =
            convertVoxelPositionToIndicies(position, this->edge_length_bricks);

        // TODO: use a compute pass to find empty and/or all identical blocks.

        // One of three things
        // Voxel (the brick is all the same voxel)
        // Valid Index
        // Invalid Index
        VoxelOrIndex& maybeBrickPointer =
            this->brick_pointer_data.at(accessPosition.brick_pointer_index);

        if (!maybeBrickPointer.isValid())
        {
            // The brick pointer we have is invalid, that means we need to
            // populate it
            maybeBrickPointer = VoxelOrIndex {
                static_cast<std::uint32_t>(this->next_free_brick_index)};

            // increment index for next
            ++this->next_free_brick_index;
        }

        // From now on maybeBrickPointer is either an valid index or a voxel

        // We need to allocate a new volume and fill it
        if (maybeBrickPointer.isVoxel())
        {
            Voxel voxelToFillVolume = maybeBrickPointer.getVoxel();

            maybeBrickPointer = VoxelOrIndex {
                static_cast<std::uint32_t>(this->next_free_brick_index)};

            // increment index for next
            ++this->next_free_brick_index;

            // TODO: make function for allocate new brick! (have fallability!)

            fill3DArray(
                this->brick_data[maybeBrickPointer.getIndex()].voxels,
                voxelToFillVolume);
        }

        // TODO: sort out the overlap between invalid brick and a brick filled
        // with air

        // From now on maybeBrickPointer is a valid index to a brick containing
        // either
        // nothing                   (if the brickPointer was invalid)
        // all filled with one voxel (if the pointer was a voxel)
        // what was already there    (otherwise)
        std::uint32_t brickPointer = maybeBrickPointer.getIndex();
        this->brick_data[brickPointer][accessPosition.voxel_internal_position] =
            voxelToWrite;

        // ok now we need to update the gpu...
    }

    Voxel BrickedVolume::readVoxel(Position position) const
    {
        PositionIndicies accessPosition =
            convertVoxelPositionToIndicies(position, this->edge_length_bricks);

        VoxelOrIndex maybeBrickPointer =
            this->brick_pointer_data.at(accessPosition.brick_pointer_index);

        util::assertFatal(
            maybeBrickPointer.isIndex(), "Tried to access invalid Voxel");

        return this->brick_data[maybeBrickPointer.getIndex()]
                               [accessPosition.voxel_internal_position];
    }

    void BrickedVolume::updateGPU()
    {
        this->brick_pointer_buffer.write(
            std::as_bytes(std::span {this->brick_pointer_data}));

        this->brick_buffer.write(std::as_bytes(std::span {this->brick_data}));
    }

    BrickedVolume::DrawingArrays BrickedVolume::draw()
    {
        return DrawingArrays {
            .brick_pointer_buffer {&this->brick_pointer_buffer},
            .brick_buffer {&this->brick_buffer},
        };
    }

} // namespace gfx::vulkan::voxel