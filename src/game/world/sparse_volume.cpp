#include "sparse_volume.hpp"
#include "game/world/sparse_volume.hpp"
#include "glm/gtx/string_cast.hpp"
#include "util/misc.hpp"
#include <ranges>
#include <util/log.hpp>
#include <variant>

namespace
{
    template<util::Integer I>
    I cyclicMod(I in, I mod)
    {
        I val = in % mod;

        return val < 0 ? val + mod : val;
    }

    template<util::Integer I>
    I flooringDiv(I dividend, I divisor)
    {
        I quotient  = dividend / divisor;
        I remainder = dividend % divisor;

        if ((divisor < 0) != (remainder < 0))
        {
            quotient--;
        }

        return quotient;
    }
} // namespace

namespace game::world
{
    Position game::world::Position::operator- () const
    {
        return Position {-this->x, -this->y, -this->z};
    }

    Position Position::operator- (Position other) const
    {
        return (*this) + -(other);
    }

    Position Position::operator+ (Position other) const
    {
        return Position {
            this->x + other.x, this->y + other.y, this->z + other.z};
    }

    Position Position::operator/ (std::int32_t number) const
    {
        return Position {
            this->x / number,
            this->y / number,
            this->z / number,
        };
    }

    Position::operator glm::vec3 () const
    {
        return glm::vec3 {
            static_cast<float>(this->x),
            static_cast<float>(this->y),
            static_cast<float>(this->z)};
    }

    Position::operator std::string () const
    {
        return fmt::format(
            "Position X: {} | Y: {} | Z: {}", this->x, this->y, this->z);
    }

    glm::vec4 Voxel::getColor() const
    {
        return glm::vec4 {
            util::convertSRGBToLinear(this->r),
            util::convertSRGBToLinear(this->g),
            util::convertSRGBToLinear(this->b),
            static_cast<float>(this->a) / 255.0f};
    }

    Voxel::operator std::string () const
    {
        return fmt::format(
            "R: {} | G: {} | B: {} | A: {}",
            this->r,
            this->g,
            this->b,
            this->a);
    }

    bool Voxel::shouldDraw() const
    {
        return this->a != 0;
    }

    VoxelVolume::VoxelVolume()
    {
        std::memset(&this->storage, '\0', sizeof(this->storage));
    }

    VoxelVolume::VoxelVolume(Voxel fillVoxel)
    {
        for (auto& xArray : this->storage)
        {
            for (auto& yArray : xArray)
            {
                for (auto& voxel : yArray)
                {
                    voxel = fillVoxel;
                }
            }
        }
    }

    Voxel& VoxelVolume::accessFromLocalPosition(Position localPosition)
    {
        if constexpr (VERDIGRIS_ENABLE_VALIDATION)
        {
            util::assertFatal(
                localPosition.x >= 0 && localPosition.x < VoxelVolume::Extent,
                "X: {} is out of bounds!",
                localPosition.x);
            util::assertFatal(
                localPosition.y >= 0 && localPosition.y < VoxelVolume::Extent,
                "Y: {} is out of bounds!",
                localPosition.y);
            util::assertFatal(
                localPosition.z >= 0 && localPosition.z < VoxelVolume::Extent,
                "Z: {} is out of bounds!",
                localPosition.z);
        }

        return this->storage[localPosition.x][localPosition.y][localPosition.z];
    }

    void VoxelVolume::drawToVectors(
        std::vector<gfx::vulkan::Vertex>& outputVertices,
        std::vector<gfx::vulkan::Index>&  outputIndices,
        Position                          localOffset)
    {
        auto iterator = std::views::iota(0, static_cast<std::int32_t>(Extent));

        for (std::int32_t localX : iterator)
        {
            for (std::int32_t localY : iterator)
            {
                for (std::int32_t localZ : iterator)
                {
                    const Voxel voxel = this->accessFromLocalPosition(
                        Position {localX, localY, localZ});

                    if (!voxel.shouldDraw())
                    {
                        continue;
                    }

                    static constexpr std::array<gfx::vulkan::Vertex, 8>
                        cubeVertices {
                            gfx::vulkan::Vertex {
                                .position {-1.0f, -1.0f, -1.0f},
                                .color {0.0f, 0.0f, 0.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {-1.0f, -1.0f, 1.0f},
                                .color {0.0f, 0.0f, 1.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {-1.0f, 1.0f, -1.0f},
                                .color {0.0f, 1.0f, 0.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {-1.0f, 1.0f, 1.0f},
                                .color {0.0f, 1.0f, 1.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {1.0f, -1.0f, -1.0f},
                                .color {1.0f, 0.0f, 0.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {1.0f, -1.0f, 1.0f},
                                .color {1.0f, 0.0f, 1.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {1.0f, 1.0f, -1.0f},
                                .color {1.0f, 1.0f, 0.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                            gfx::vulkan::Vertex {
                                .position {1.0f, 1.0f, 1.0f},
                                .color {1.0f, 1.0f, 1.0f, 1.0f},
                                .normal {},
                                .uv {},
                            },
                        };

                    constexpr std::array<gfx::vulkan::Index, 36> cubeIndices {
                        6, 2, 7, 2, 3, 7, 0, 4, 5, 1, 0, 5, 0, 2, 6, 4, 0, 6,
                        3, 1, 7, 1, 5, 7, 2, 0, 3, 0, 1, 3, 4, 6, 7, 5, 4, 7};

                    const std::size_t IndicesOffset = outputVertices.size();

                    // Update positions to be aligned with the world and insert
                    // into output
                    for (gfx::vulkan::Vertex v : cubeVertices)
                    {
                        v.position /= 2.0f;

                        v.position += static_cast<glm::vec3>(localOffset);

                        v.position += static_cast<glm::vec3>(
                            Position {localX, localY, localZ});

                        v.color *= voxel.getColor();

                        outputVertices.push_back(v);
                    }

                    // update indices to actually point to the correct index
                    for (gfx::vulkan::Index i : cubeIndices)
                    {
                        i += static_cast<std::uint32_t>(IndicesOffset);

                        outputIndices.push_back(i);
                    }
                }
            }
        }
    }

    SparseVoxelVolume::SparseVoxelVolume()
    {
        // TODO: optimize!
        for (auto& xArray : this->data)
        {
            for (auto& yArray : xArray)
            {
                for (auto& voxel : yArray)
                {
                    voxel = Voxel {};
                }
            }
        }
    }

    Voxel& SparseVoxelVolume::accessFromLocalPosition(Position sparsePosition)
    {
        if constexpr (VERDIGRIS_ENABLE_VALIDATION)
        {
            util::assertFatal(
                sparsePosition.x >= SparseVoxelVolume::VoxelMinimum
                    && sparsePosition.x <= SparseVoxelVolume::VoxelMaximum,
                "X: {} is outside of {}->{}",
                sparsePosition.x,
                SparseVoxelVolume::VoxelMinimum,
                SparseVoxelVolume::VoxelMaximum);

            util::assertFatal(
                sparsePosition.y >= SparseVoxelVolume::VoxelMinimum
                    && sparsePosition.y <= SparseVoxelVolume::VoxelMaximum,
                "Y: {} is outside of {}->{}",
                sparsePosition.y,
                SparseVoxelVolume::VoxelMinimum,
                SparseVoxelVolume::VoxelMaximum);

            util::assertFatal(
                sparsePosition.z >= SparseVoxelVolume::VoxelMinimum
                    && sparsePosition.z <= SparseVoxelVolume::VoxelMaximum,
                "Z: {} is outside of {}->{}",
                sparsePosition.z,
                SparseVoxelVolume::VoxelMinimum,
                SparseVoxelVolume::VoxelMaximum);
        }

        const Position LocalVolumePosition {
            flooringDiv(sparsePosition.x, VoxelVolume::Extent)
                + SparseVoxelVolume::Extent / 2,
            flooringDiv(sparsePosition.y, VoxelVolume::Extent)
                + SparseVoxelVolume::Extent / 2,
            flooringDiv(sparsePosition.z, VoxelVolume::Extent)
                + SparseVoxelVolume::Extent / 2,
        };

        const Position volumeInternalPosition {
            cyclicMod(sparsePosition.x, VoxelVolume::Extent),
            cyclicMod(sparsePosition.y, VoxelVolume::Extent),
            cyclicMod(sparsePosition.z, VoxelVolume::Extent)};

        std::variant<std::unique_ptr<VoxelVolume>, Voxel>& maybeLocalVolume {
            this->data[LocalVolumePosition.x][LocalVolumePosition.y]
                      [LocalVolumePosition.z]};

        // If the volume doesn't exist then create it
        if (Voxel* voxel = std::get_if<Voxel>(&maybeLocalVolume))
        {
            maybeLocalVolume = std::make_unique<VoxelVolume>(*voxel);
        }

        return (*std::get_if<std::unique_ptr<VoxelVolume>>(&maybeLocalVolume))
            ->accessFromLocalPosition(volumeInternalPosition);
    }

    std::pair<std::vector<gfx::vulkan::Vertex>, std::vector<gfx::vulkan::Index>>
    SparseVoxelVolume::draw(Position localOffset)
    {
        std::vector<gfx::vulkan::Vertex> vertices;
        vertices.reserve(3'000'000);
        std::vector<gfx::vulkan::Index> indices;
        vertices.reserve(9'000'000);
        // for (std::size_t x = 0; x < SparseVoxelVolume) }

        for (std::size_t xIdx = 0; xIdx < this->data.size(); ++xIdx)
        {
            for (std::size_t yIdx = 0; yIdx < this->data[xIdx].size(); ++yIdx)
            {
                for (std::size_t zIdx = 0; zIdx < this->data[xIdx][yIdx].size();
                     ++zIdx)
                {
                    auto& voxel = this->data[xIdx][yIdx][zIdx];

                    std::visit(
                        util::VariantHelper {
                            [&](const std::unique_ptr<VoxelVolume>& volume)
                            {
                                const Position LocalVolumeOffset {
                                    localOffset
                                    + Position {
                                        (static_cast<std::int32_t>(xIdx)
                                         - static_cast<std::int32_t>(
                                               SparseVoxelVolume::Extent)
                                               / 2)
                                            * static_cast<std::int32_t>(
                                                VoxelVolume::Extent),
                                        (static_cast<std::int32_t>(yIdx)
                                         - static_cast<std::int32_t>(
                                               SparseVoxelVolume::Extent)
                                               / 2)
                                            * static_cast<std::int32_t>(
                                                VoxelVolume::Extent),
                                        (static_cast<std::int32_t>(zIdx)
                                         - static_cast<std::int32_t>(
                                               SparseVoxelVolume::Extent)
                                               / 2)
                                            * static_cast<std::int32_t>(
                                                VoxelVolume::Extent),
                                    }};

                                volume->drawToVectors(
                                    vertices, indices, LocalVolumeOffset);
                            },
                            [](Voxel v) -> void
                            {
                                if (v.shouldDraw())
                                {
                                    util::logTrace(
                                        "TODO: implement {}",
                                        glm::to_string(v.getColor()));
                                }

                                return;
                            }},
                        voxel);
                }
            }
        }

        // vertices.shrink_to_fit();
        // indices.shrink_to_fit();

        return std::make_pair(std::move(vertices), std::move(indices));
    }
} // namespace game::world