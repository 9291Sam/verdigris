#include <array>
#include <cstdint>

struct Voxel
{
    std::uint64_t a;
};

struct Brick
{
    std::array<Voxel, 8UZ * 8 * 8> brick_data;
};

static_assert(sizeof(Brick) == 4096);

std::array<Brick, 128 * 128 * 128> brick_storage;
// 8gb

static_assert(sizeof(brick_storage) == 2);

struct BrickPointer
{
    std::size_t idx; // TODO: make u32? and then use the other sparseness for
                     // for multiple arrays for infinite distance
};

std::array<std::array<BrickPointer, 8>, 128 * 128 * 128> brick_registrar;

static_assert(sizeof(brick_registrar) == 1); // 134mb