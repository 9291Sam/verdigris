#ifndef SRC_GFX_VULKAN_VOXEL_GPU_LINEAR_ALLOCATOR_HPP
#define SRC_GFX_VULKAN_VOXEL_GPU_LINEAR_ALLOCATOR_HPP

#include <compare>
#include <span>
#include <vector>

namespace gfx::vulkan::voxel
{
    class LinearBrickAllocator;

    struct BrickHandle
    {
    public:

        explicit BrickHandle();
        ~BrickHandle() = default;

        BrickHandle(const BrickHandle&) = default;
        BrickHandle(BrickHandle&&) noexcept;
        BrickHandle& operator= (const BrickHandle&) = default;
        BrickHandle& operator= (BrickHandle&&) noexcept;

        [[nodiscard]] bool isValid() const;

        [[nodiscard]] std::strong_ordering
                           operator<=> (const BrickHandle&) const = default;
        [[nodiscard]] bool operator== (const BrickHandle&) const  = default;
        [[nodiscard]] bool operator!= (const BrickHandle&) const  = default;

    private:
        friend LinearBrickAllocator;
        explicit BrickHandle(std::uint32_t);

        std::uint32_t index;
    };

    class LinearBrickAllocator
    {
    public:

    public:
        LinearBrickAllocator(std::size_t brickSize, std::size_t numberOfBricks);

        [[nodiscard]] std::span<std::byte> lookupAllocation(BrickHandle) const;

        [[nodiscard]] BrickHandle allocate();
        void                      free(BrickHandle);


    private:
        std::size_t brick_size;
        std::size_t number_of_bricks;

        std::vector<std::byte>   data;
        std::vector<BrickHandle> extra_handles;

        std::size_t index_of_rest_free;
    };

} // namespace gfx::vulkan::voxel

#endif // SRC_GFX_VULKAN_VOXEL_GPU_LINEAR_ALLOCATOR_HPP