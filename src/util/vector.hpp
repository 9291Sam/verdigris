#ifndef SRC_UTIL_VECTOR_HPP
#define SRC_UTIL_VECTOR_HPP

#include "misc.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <gcem.hpp> // TODO: remove once big three have constexpr <cmath> support
#include <numbers>

namespace util
{
    // TODO: rotors && special matricies
    template<Arithmetic T, std::size_t N>
    class alignas(8) Vector final // NOLINT
    {
    public:
        static constexpr std::size_t Length {N};
        using ElementType = T;
    public:

        [[nodiscard]] constexpr Vector() noexcept
            : data {}
        {
            std::fill(this->data.begin(), this->data.end(), static_cast<T>(INFINITY));
        }
        [[nodiscard]] constexpr explicit Vector(std::same_as<T> auto... args) noexcept
            requires (sizeof...(args) == N)
            : data {args...}
        {}
        constexpr ~Vector() = default;

        constexpr Vector(const Vector&)                 = default;
        constexpr Vector(Vector&&) noexcept             = default;
        constexpr Vector& operator= (const Vector&)     = default;
        constexpr Vector& operator= (Vector&&) noexcept = default;

        ///
        /// Conversion operators
        ///

        // noexcept false -> throwing
        template<class J>
        [[nodiscard]] constexpr explicit operator Vector<J, N> () const
            noexcept(noexcept(std::is_nothrow_convertible_v<T, J>))
            requires std::is_convertible_v<T, J>
        {
            Vector<J, N> output {};

            for (std::size_t i = 0; i < N; ++i)
            {
                output[i] = static_cast<J>(this->data[i]);
            }

            return output;
        }

        ///
        /// Comparison operators
        ///

        [[nodiscard]] constexpr std::partial_ordering
        operator<=> (const Vector& other) const noexcept
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                std::partial_ordering order =
                    std::partial_order(this->data[i], other.data[i]); // NOLINT

                if (order != std::partial_ordering::equivalent)
                {
                    return order;
                }
            }

            return std::partial_ordering::equivalent;
        }

        [[nodiscard]] constexpr bool operator== (const Vector& other) const noexcept
        {
            return ((*this) <=> (other)) == std::partial_ordering::equivalent;
        }

        [[nodiscard]] constexpr bool operator!= (const Vector& other) const noexcept
        {
            return !((*this) == other);
        }

        ///
        /// Access operators in the form of rgba and xyzw TODO: deducing
        /// this
        ///
        [[nodiscard]] constexpr T& r() noexcept
            requires (N > 0)
        {
            return this->data[0];
        }

        [[nodiscard]] constexpr T r() const noexcept
            requires (N > 0)
        {
            return this->data[0];
        }

        [[nodiscard]] constexpr T& g() noexcept
            requires (N > 1)
        {
            return this->data[1];
        }

        [[nodiscard]] constexpr T g() const noexcept
            requires (N > 1)
        {
            return this->data[1];
        }

        [[nodiscard]] constexpr T& b() noexcept
            requires (N > 2)
        {
            return this->data[2];
        }

        [[nodiscard]] constexpr T b() const noexcept
            requires (N > 2)
        {
            return this->data[2];
        }

        [[nodiscard]] constexpr T& a() noexcept
            requires (N > 3)
        {
            return this->data[3];
        }

        [[nodiscard]] constexpr T a() const noexcept
            requires (N > 3)
        {
            return this->data[3];
        }

        [[nodiscard]] constexpr T& x() noexcept
            requires (N > 0)
        {
            return this->data[0];
        }

        [[nodiscard]] constexpr T x() const noexcept
            requires (N > 0)
        {
            return this->data[0];
        }

        [[nodiscard]] constexpr T& y() noexcept
            requires (N > 1)
        {
            return this->data[1];
        }

        [[nodiscard]] constexpr T y() const noexcept
            requires (N > 1)
        {
            return this->data[1];
        }

        [[nodiscard]] constexpr T& z() noexcept
            requires (N > 2)
        {
            return this->data[2];
        }

        [[nodiscard]] constexpr T z() const noexcept
            requires (N > 2)
        {
            return this->data[2];
        }

        [[nodiscard]] constexpr T& w() noexcept
            requires (N > 3)
        {
            return this->data[3];
        }

        [[nodiscard]] constexpr T w() const noexcept
            requires (N > 3)
        {
            return this->data[3];
        }

        [[nodiscard]] constexpr T operator[] (std::size_t idx) const noexcept
        {
            return this->data[idx];
        }

        [[nodiscard]] constexpr T& operator[] (std::size_t idx) noexcept
        {
            return this->data[idx];
        }

        ///
        /// Arethmetic operators
        ///

        [[nodiscard]] constexpr Vector operator+ () const noexcept
        {
            return *this;
        }

        [[nodiscard]] constexpr Vector operator- () const noexcept
        {
            // C++'s copy semantics are perfect and have no flaws whatsoever
            Vector output {*this};

            for (T& t : output.data)
            {
                t = -t;
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator+ (const Vector& other) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] += other.data[i]; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator+ (T value) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] += value; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator- (const Vector& other) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] -= other.data[i]; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator- (T value) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] -= value; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator* (const Vector& other) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] *= other.data[i]; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator* (T value) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] *= value; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator/ (const Vector& other) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] /= other.data[i]; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator/ (T value) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                output.data[i] /= value; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator% (const Vector& other) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                if consteval
                {
                    output.data[i] = gcem::fmod(output.data[i], other.data[i]); // NOLINT
                }
                else
                {
                    output.data[i] = std::fmod(output.data[i], other.data[i]); // NOLINT
                }
            }

            return output;
        }

        [[nodiscard]] constexpr Vector operator% (T value) const noexcept
        {
            Vector output {*this};

            for (std::size_t i = 0; i < N; ++i)
            {
                if consteval
                {
                    output.data[i] = gcem::fmod(output.data[i], value); // NOLINT
                }
                else
                {
                    output.data[i] = std::fmod(output.data[i], value); // NOLINT
                }
            }

            return output;
        }

        ///
        /// Arethemetic Assignment operators
        /// TODO: tests
        ///

        constexpr Vector& operator+= (const Vector& other) noexcept
        {
            (*this) = (*this) + other;

            return *this;
        }

        constexpr Vector& operator+= (T value) noexcept
        {
            (*this) = (*this) + value;

            return *this;
        }

        constexpr Vector& operator-= (const Vector& other) noexcept
        {
            (*this) = (*this) - other;

            return *this;
        }

        constexpr Vector& operator-= (T value) noexcept
        {
            (*this) = (*this) - value;

            return *this;
        }

        constexpr Vector& operator*= (const Vector& other) noexcept
        {
            (*this) = (*this) * other;

            return *this;
        }

        constexpr Vector& operator*= (T value) noexcept
        {
            (*this) = (*this) * value;

            return *this;
        }

        constexpr Vector& operator/= (const Vector& other) noexcept
        {
            (*this) = (*this) / other;

            return *this;
        }

        constexpr Vector& operator/= (T value) noexcept
        {
            (*this) = (*this) / value;

            return *this;
        }

        constexpr Vector& operator%= (const Vector& other) noexcept
        {
            (*this) = (*this) % other;

            return *this;
        }

        constexpr Vector& operator%= (T value) noexcept
        {
            (*this) = (*this) % value;

            return *this;
        }

        ///
        /// Vector specific operations
        ///
        [[nodiscard]] constexpr T dot(const Vector& other) const noexcept
        {
            T output {static_cast<T>(0.0)};

            for (std::size_t i = 0; i < N; ++i)
            {
                output += (this->data[i] * other.data[i]); // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr Vector cross(const Vector& other) const noexcept
            requires (N == 3)
        {
            return Vector {
                this->data[1] * other.data[2] - this->data[2] * other.data[1],
                this->data[2] * other.data[0] - this->data[0] * other.data[2],
                this->data[0] * other.data[1] - this->data[1] * other.data[0]};
        }

        [[nodiscard]] constexpr T magnitude() const noexcept
        {
            T output {static_cast<T>(0.0)};

            for (std::size_t i = 0; i < N; ++i)
            {
                output += (this->data[i] * this->data[i]); // NOLINT
            }

            return gcem::sqrt(output);
        }

        [[nodiscard]] constexpr Vector normalize() const noexcept
        {
            return (*this) / this->magnitude();
        }

        [[nodiscard]] constexpr T angleTo(const Vector& other) const noexcept
        {
            const T intermediate = (this->dot(other)) / (this->magnitude() * other.magnitude());
            if consteval
            {
                return gcem::acos(intermediate);
            }
            else
            {
                return std::acos(intermediate);
            }
        }

        /// Iterator interfacing
        // TODO: test iterators in matrix and vector classes
        [[nodiscard]] constexpr std::array<T, N>::iterator begin() noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<T, N>::const_iterator begin() const noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<T, N>::const_iterator cbegin() const noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<T, N>::iterator end() noexcept
        {
            return this->data.end();
        }

        [[nodiscard]] constexpr std::array<T, N>::const_iterator end() const noexcept
        {
            return this->data.cend();
        }

        [[nodiscard]] constexpr std::array<T, N>::const_iterator cend() const noexcept
        {
            return this->data.cend();
        }

    private:
        std::array<T, N> data;
    };

    using Vec2 = Vector<float, 2>;
    using Vec3 = Vector<float, 3>;
    using Vec4 = Vector<float, 4>;

    // NOLINTBEGIN
    consteval bool testVector()
    {
        // construction
        {
            util::Vector<float, 4> vec0 {};
        }

        {
            // reading test
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};

            static_assert(vec0.r() == 1.0f);
            static_assert(vec0.g() == 2.0f);
            static_assert(vec0.b() == 3.0f);
            static_assert(vec0.a() == 4.0f);

            static_assert(vec0.x() == 1.0f);
            static_assert(vec0.y() == 2.0f);
            static_assert(vec0.z() == 3.0f);
            static_assert(vec0.w() == 4.0f);

            // write back rgba
            util::Vector<float, 4> vec1 {0.0f, 0.0f, 0.0f, 0.0f};

            vec1.r() = 1.0f;
            vec1.g() = 2.0f;
            vec1.b() = 3.0f;
            vec1.a() = 4.0f;

            if (vec1.r() != 1.0f)
            {
                return false;
            }

            if (vec1.g() != 2.0f)
            {
                return false;
            }

            if (vec1.b() != 3.0f)
            {
                return false;
            }

            if (vec1.a() != 4.0f)
            {
                return false;
            }

            if (vec1.x() != 1.0f)
            {
                return false;
            }

            if (vec1.y() != 2.0f)
            {
                return false;
            }

            if (vec1.z() != 3.0f)
            {
                return false;
            }

            if (vec1.w() != 4.0f)
            {
                return false;
            }
        }

        // urnary minus
        {
            constexpr util::Vector<float, 3> vec0 {1.0f, 2.0f, 3.0f};
            constexpr util::Vector<float, 3> vec1 = -vec0;

            static_assert(vec1.r() == -1.0f);
            static_assert(vec1.g() == -2.0f);
            static_assert(vec1.b() == -3.0f);
        }

        // addition
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec1 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec2 {vec0 + vec1};

            static_assert(vec2.r() == 2.0f);
            static_assert(vec2.g() == 4.0f);
            static_assert(vec2.b() == 6.0f);
            static_assert(vec2.a() == 8.0f);

            util::Vector<float, 4> vec3 {1.0f, 2.0f, 3.0f, 4.0f};
            util::Vector<float, 4> vec4 {1.0f, 2.0f, 3.0f, 4.0f};

            vec4 += vec3;

            if (vec4 != util::Vector<float, 4> {2.0f, 4.0f, 6.0f, 8.0f})
            {
                return false;
            }
        }

        // subtraction
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec1 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec2 {vec0 - vec1};

            static_assert(vec2.r() == 0.0f);
            static_assert(vec2.g() == 0.0f);
            static_assert(vec2.b() == 0.0f);
            static_assert(vec2.a() == 0.0f);

            util::Vector<float, 4> vec3 {1.0f, 2.0f, 3.0f, 4.0f};
            util::Vector<float, 4> vec4 {1.0f, 2.0f, 3.0f, 4.0f};

            vec4 -= vec3;

            if (vec4 != util::Vector<float, 4> {0.0f, 0.0f, 0.0f, 0.0f})
            {
                return false;
            }
        }

        // multiplication
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec1 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec2 {vec0 * vec1};

            static_assert(vec2.r() == 1.0f);
            static_assert(vec2.g() == 4.0f);
            static_assert(vec2.b() == 9.0f);
            static_assert(vec2.a() == 16.0f);

            constexpr util::Vector<float, 4> vec3 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec4 {vec3 * 3.0f};

            static_assert(vec4.r() == 3.0f);
            static_assert(vec4.g() == 6.0f);
            static_assert(vec4.b() == 9.0f);
            static_assert(vec4.a() == 12.0f);

            util::Vector<float, 4> vec5 {1.0f, 2.0f, 3.0f, 4.0f};
            util::Vector<float, 4> vec6 {1.0f, 2.0f, 3.0f, 4.0f};

            vec5 *= vec6;

            if (vec5 != util::Vector<float, 4> {1.0f, 4.0f, 9.0f, 16.0f})
            {
                return false;
            }
        }

        // division
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec1 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec2 {vec0 / vec1};

            static_assert(vec2.r() == 1.0f);
            static_assert(vec2.g() == 1.0f);
            static_assert(vec2.b() == 1.0f);
            static_assert(vec2.a() == 1.0f);

            constexpr util::Vector<float, 4> vec3 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec4 {vec3 / 3.0f};

            static_assert(vec4.r() == 0.333333333333333f);
            static_assert(vec4.g() == 0.666666666666666f);
            static_assert(vec4.b() == 1.0f);
            static_assert(vec4.a() == 1.333333333333333f);

            util::Vector<float, 4> vec5 {1.0f, 2.0f, 3.0f, 4.0f};
            util::Vector<float, 4> vec6 {1.0f, 2.0f, 3.0f, 4.0f};

            vec5 /= vec6;

            if (vec5 != util::Vector<float, 4> {1.0f, 1.0f, 1.0f, 1.0f})
            {
                return false;
            }
        }

        // modulo
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec1 {3.0f, 2.0f, 4.0f, 5.0f};
            constexpr util::Vector<float, 4> vec2 {vec0 % vec1};

            static_assert(vec2.r() == 1.0f);
            static_assert(vec2.g() == 0.0f);
            static_assert(vec2.b() == 3.0f);
            static_assert(vec2.a() == 4.0f);

            constexpr util::Vector<float, 4> vec3 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr util::Vector<float, 4> vec4 {vec3 % 3.0f};

            static_assert(vec4.r() == 1.0f);
            static_assert(vec4.g() == 2.0f);
            static_assert(vec4.b() == 0.0f);
            static_assert(vec4.a() == 1.0f);

            util::Vector<float, 4> vec5 {1.0f, 2.0f, 3.0f, 4.0f};
            util::Vector<float, 4> vec6 {3.0f, 2.0f, 4.0f, 5.0f};

            vec5 %= vec6;

            if (vec5 != util::Vector<float, 4> {1.0f, 0.0f, 3.0f, 4.0f})
            {
                return false;
            }
        }

        // mag norm angle

        // Dot
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 5.0f};
            constexpr util::Vector<float, 4> vec1 {5.0f, 3.0f, 4.0f, 5.0f};

            constexpr float DotProduct {vec0.dot(vec1)};

            static_assert(vec0.dot(vec1) == 48.0f);
        }

        // cross
        {
            constexpr util::Vector<float, 3> vec0 {1.0f, 0.0f, 0.0f};
            constexpr util::Vector<float, 3> vec1 {0.0f, 1.0f, 0.0f};

            static_assert(vec0.cross(vec1) == util::Vector<float, 3> {0.0f, 0.0f, 1.0f});
        }

        // magnitude
        {
            constexpr util::Vector<float, 4> vec0 {1.0f, 2.0f, 3.0f, 4.0f};

            static_assert(vec0.magnitude() == gcem::sqrt(30.0f));
        }

        // normaization
        {
            constexpr util::Vector<float, 3> vec0 {2.0f, 2.0f, 2.0f};
            constexpr util::Vector<float, 3> vec1 {vec0.normalize()};

            static_assert(vec1.r() == std::numbers::inv_sqrt3_v<float>);
            static_assert(vec1.g() == std::numbers::inv_sqrt3_v<float>);
            static_assert(vec1.b() == std::numbers::inv_sqrt3_v<float>);
        }

        // angle
        {
            constexpr util::Vector<float, 2> vec0 {1.0f, 1.0f};
            constexpr util::Vector<float, 2> vec1 {1.0f, 0.0f};

            static_assert(
                vec0.angleTo(vec1) - std::numbers::pi_v<float> / 4
                < std::numeric_limits<float>::epsilon());
        }

        // alignment and size
        {
            static_assert(sizeof(util::Vector<float, 2>) == sizeof(float) * 2);

            static_assert(sizeof(util::Vector<float, 3>) == sizeof(float) * 4);

            static_assert(sizeof(util::Vector<float, 4>) == sizeof(float) * 4);

            static_assert(alignof(util::Vector<float, 2>) == 8);

            static_assert(alignof(util::Vector<float, 3>) == 8);

            static_assert(alignof(util::Vector<float, 4>) == 8);
        }

        return true;
    }

    static_assert(testVector());
    // NOLINTEND
} // namespace util

#endif // SRC_UTIL_VECTOR_HPP
