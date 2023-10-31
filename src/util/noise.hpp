#ifndef SRC_UTIL_NOISE_HPP
#define SRC_UTIL_NOISE_HPP

#include "misc.hpp"
#include <cmath>
#include <concepts>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <limits>
#include <random>

///
/// This entire implementation is unceremoniously sz`tolen from the wikipedia
/// page on perlin noise
/// https://en.wikipedia.org/w/index.php?title=Perlin_noise&oldid=1148235423
///

namespace util
{
    // TODO: N dimensional noise
    template<Integer I>
    class FastMCG
    {
    public:
        // implementation of UniformRandomBitGenerator
        // https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator

        // This allows for interfacing with standard RandomNumberDistribution
        // such as std::uniform_real_distribution

        using result_type = I;

        static constexpr I min() noexcept
        {
            return std::numeric_limits<I>::min();
        }

        static constexpr I max() noexcept
        {
            return std::numeric_limits<I>::max();
        }

        constexpr I operator() () noexcept
        {
            return this->next();
        }

    public:

        constexpr explicit FastMCG(I seed_)
            : seed {seed_}
            , multiplier {6364136223846793005ULL}
            , increment {1442695040888963407ULL}
        {
            std::ignore = this->next();
        }
        constexpr ~FastMCG() = default;

        constexpr explicit FastMCG(const FastMCG&)        = default; // NOLINT
        constexpr FastMCG(FastMCG&&) noexcept             = default; // NOLINT
        constexpr FastMCG& operator= (const FastMCG&)     = delete;
        constexpr FastMCG& operator= (FastMCG&&) noexcept = default; // NOLINT

        constexpr I next()
        {
            constexpr I width {8 * sizeof(I)};
            constexpr I offset {width / 2};

            const I prev = this->seed;

            this->seed = (this->multiplier * this->seed + this->increment);

            I output = this->seed;

            output ^= prev << offset | prev >> (width - offset);

            return output;
        }

    private:
        I seed;
        I multiplier;
        I increment;
    };

    /// Interpolates between [value1, rightBound] given a weight [0.0, 1.0]
    /// along the range
    template<std::floating_point T>
    constexpr inline T quarticInterpolate(T leftBound, T rightBound, T weight)
    {
        constexpr T Six {static_cast<T>(6.0)};
        constexpr T Fifteen {static_cast<T>(15.0)};
        constexpr T Ten {static_cast<T>(10.0)};

        return (rightBound - leftBound)
                 * ((weight * (weight * Six - Fifteen) + Ten) * weight * weight
                    * weight)
             + leftBound;
    }

    template<std::size_t N> // TODO: replace with some AbsolutePosition
    constexpr inline Vec2
    randomGradient(Vector<std::int64_t, N> vector, std::uint64_t seed)
    {
        std::size_t workingSeed {0};
        for (std::int64_t i : vector.data)
        {
            util::hashCombine(workingSeed, static_cast<std::size_t>(i));
        }

        FastMCG<std::uint64_t> engine {workingSeed};

        float random = std::uniform_real_distribution<float> {
            0.0f, std::numbers::pi_v<float> * 2}(engine);

        // gcem is ~3-5x slower than std, try and avoid it
        if consteval
        {
            return Vec2 {gcem::cos(random), gcem::sin(random)};
        }
        else
        {
            return Vec2 {std::cos(random), std::sin(random)};
        }
    }

    template<std::size_t N>
    constexpr inline float dotGridGradient(
        Vector<std::int64_t, N> position,
        Vector<float, N>        perlinGridGranularity,
        std::uint64_t           seed)
    {
        // Get gradient from integer coordinates
        const Vec2 gradient = randomGradient(position, seed);

        // Compute the distance vector
        const Vector<float, N> offset {
            perlinGridGranularity - static_cast<Vector<float, N>>(position)};

        return offset.dot(gradient);
    }

    // Compute Perlin noise at coordinates x, y
    // TODO: replace with some AbsolutePosition with an int64 + a float
    // TODO: add seeds
    constexpr inline float
    perlin(Vec2 vector, std::uint64_t seed) // TODO: make generic
    {
        const Vector<std::int64_t, 2> BottomLeftGrid {
            static_cast<std::int64_t>(std::floor(vector.x())),
            static_cast<std::int64_t>(std::floor(vector.y())),
        };

        const Vector<std::int64_t, 2> TopRightGrid {BottomLeftGrid + 1};

        const Vector<std::int64_t, 2> BottomRightGrid {
            BottomLeftGrid.x(), TopRightGrid.y()};

        const Vector<std::int64_t, 2> TopLeftGrid {
            TopRightGrid.x(), BottomLeftGrid.y()};

        const Vec2 OffsetIntoGrid {vector - static_cast<Vec2>(BottomLeftGrid)};

        const float LeftGradient = quarticInterpolate(
            dotGridGradient(BottomLeftGrid, vector, seed),
            dotGridGradient(TopLeftGrid, vector, seed),
            OffsetIntoGrid.x());

        const float RightGradient = quarticInterpolate(
            dotGridGradient(BottomRightGrid, vector, seed),
            dotGridGradient(TopRightGrid, vector, seed),
            OffsetIntoGrid.x());

        return quarticInterpolate(
            LeftGradient, RightGradient, OffsetIntoGrid.y());
    }

} // namespace util

#endif // SRC_UTIL_NOISE_HPP