#include "chunk.hpp"
#include "game/world/chunk.hpp"
#include "game/world/sparse_volume.hpp"
#include "gfx/object.hpp"
#include "gfx/renderer.hpp"
#include "util/log.hpp"
#include "util/misc.hpp"
#include "util/threads.hpp"
#include <chrono>
#include <memory>
#include <ranges>
#include <thread>
#include <util/noise.hpp>

namespace game::world
{
    enum class ChunkStates : std::uint8_t
    {
        WaitingForVolume = 0,
        WaitingForObject = 1,
        Initalized       = 2,
        Invalid          = 255,
    };

    Chunk::Chunk()
        : lod {3}
        , state {ChunkStates::Invalid}
    {}

    Chunk::Chunk(
        Position position_,
        util::Fn<std::int32_t(std::int32_t, std::int32_t) noexcept>
            generationFunc)
        : location {position_}
        , lod {5}
        , state {ChunkStates::WaitingForVolume}
        , volume {nullptr}
        , object {nullptr}
        , future_volume {std::nullopt}
        , future_object {std::nullopt}
    {
        util::logTrace(
            "Constructed Chunk @ {} | {}",
            static_cast<std::string>(position_),
            static_cast<std::string>(this->location));

        const std::int32_t localMinPollingX =
            SparseVoxelVolume::VoxelMinimum + this->location.x;
        const std::int32_t localMaxPollingX =
            SparseVoxelVolume::VoxelMaximum + this->location.x;

        const std::int32_t localMinPollingZ =
            SparseVoxelVolume::VoxelMinimum + this->location.z;
        const std::int32_t localMaxPollingZ =
            SparseVoxelVolume::VoxelMaximum + this->location.z;

        this->future_volume = std::async(
            std::launch::async,
            // TODO: find why replacing position = this->location with just
            // a default `=` capture and calling it directely causes a
            // `stack-buffer-overrun` i.e a read after free
            [position = this->location,
             generationFunc,
             localMinPollingX,
             localMaxPollingX,
             localMinPollingZ,
             localMaxPollingZ]
            {
                util::logTrace(
                    "SparseVolume creation started @ {}",
                    static_cast<std::string>(position));

                auto start = std::chrono::high_resolution_clock::now();

                std::shared_ptr<SparseVoxelVolume> workingVolume =
                    std::make_shared<SparseVoxelVolume>();

                for (std::int32_t pollingX :
                     std::views::iota(localMinPollingX, localMaxPollingX))
                {
                    for (std::int32_t pollingZ :
                         std::views::iota(localMinPollingZ, localMaxPollingZ))
                    {
                        const float normalizedX =
                            static_cast<float>(util::map<double>(
                                static_cast<double>(pollingX),
                                static_cast<double>(localMinPollingX),
                                static_cast<double>(localMaxPollingX),
                                0.0,
                                1.0));

                        const float normalizedZ =
                            static_cast<float>(util::map<double>(
                                static_cast<double>(pollingZ),
                                static_cast<double>(localMinPollingZ),
                                static_cast<double>(localMaxPollingZ),
                                0.0,
                                1.0));

                        glm::vec4 color {
                            0.0f,
                            // util::map(normalizedX, -1.0f, 1.0f, 0.0f, 1.0f),
                            // util::map(normalizedZ, -1.0f, 1.0f, 0.0f, 1.0f),
                            normalizedX,
                            normalizedZ,
                            1.0f};

                        // glm::vec4 color {
                        //     0.0f,
                        //     4 * normalizedX - 4 * normalizedX *
                        //     normalizedX, 4 * normalizedZ - 4 *
                        //     normalizedZ * normalizedZ, 1.0f};

                        // glm::vec4 color {
                        //     0.0f,
                        //     std::exp(
                        //         -12.0 * (normalizedX - 0.5)
                        //         * (normalizedX - 0.5)),
                        //     std::exp(
                        //         -12.0 * (normalizedZ - 0.5)
                        //         * (normalizedZ - 0.5)),
                        //     1.0f};

                        Position polled {
                            pollingX,
                            generationFunc(pollingX, pollingZ),
                            pollingZ};

                        Position accessPosition = polled - position;

                        // util::logTrace(
                        //     "accessPosition {} | Polled {} | loc {}",
                        //     static_cast<std::string>(accessPosition),
                        //     static_cast<std::string>(polled),
                        //     static_cast<std::string>(position));

                        workingVolume->accessFromLocalPosition(accessPosition) =
                            world::Voxel {
                                .r {util::convertLinearToSRGB(color.r)},
                                .g {util::convertLinearToSRGB(color.g)},
                                .b {util::convertLinearToSRGB(color.b)},
                                .a {static_cast<std::uint8_t>(color.a * 255)}};
                    }
                }

                auto end = std::chrono::high_resolution_clock::now();

                util::logTrace(
                    "Generated chunk in {}ms",
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        end - start)
                        .count());

                return workingVolume;
            });
    }

    void Chunk::updateDrawState(const gfx::Renderer& renderer)
    {
        switch (this->state)
        {
        case ChunkStates::WaitingForVolume:

            // NOLINTNEXTLINE: Checked by state machine
            if (this->future_volume->valid())
            {
                // NOLINTNEXTLINE: Checked by state machine
                this->volume = this->future_volume->get();

                this->future_object = std::async(
                    std::launch::async,
                    [lambdaLocation  = this->location,
                     lambdaVolume    = this->volume,
                     &lambdaRenderer = renderer]
                    -> std::shared_ptr<gfx::SimpleTriangulatedObject>
                    {
                        auto start = std::chrono::high_resolution_clock::now();

                        util::assertFatal(
                            lambdaVolume != nullptr, "Volume was nullptr!");

                        auto [vertices, indices] =
                            lambdaVolume->draw(lambdaLocation);

                        auto end = std::chrono::high_resolution_clock::now();

                        util::logTrace(
                            "Triangulated chunk in {}ms",
                            std::chrono::duration_cast<
                                std::chrono::milliseconds>(end - start)
                                .count());

                        return gfx::SimpleTriangulatedObject::create(
                            lambdaRenderer,
                            std::move(vertices),
                            std::move(indices));
                    });

                this->state = ChunkStates::WaitingForObject;

                this->future_volume = std::nullopt;
            }

            return;

        case ChunkStates::WaitingForObject:

            // NOLINTNEXTLINE: Checked by state machine
            if (this->future_object->valid())
            {
                // NOLINTNEXTLINE: Checked by state machine
                this->object = this->future_object->get();

                this->state = ChunkStates::Initalized;

                this->future_object = std::nullopt;
            }

            return;

        case ChunkStates::Initalized:

            // if this object is default constructed or moved from this->object
            // is nullptr, check this and print a warning
            if (this->object == nullptr)
            {
                util::logWarn(
                    "Chunk::draw() called on default constructed or moved from "
                    "object!");
            }

            return;

        case ChunkStates::Invalid:
            util::panic("Chunk::draw() called when state was Invalid");

            return;

            // default:
            //     util::panic(
            //         "Chunk::draw() called with invalid state {}",
            //         std::to_underlying(this->state));

            //     return;
        }

        util::panic("Left control flow in Chunk::draw()");
    }
} // namespace game::world
