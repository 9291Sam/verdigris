#ifndef SRC_UTIL_MATRIX_HPP
#define SRC_UTIL_MATRIX_HPP

#include "vector.hpp"
#include <array>

namespace util
{

    // <T, R, C>
    template<Arithmetic T, std::size_t R, std::size_t C>
    class Matrix
    {
    public:
        using ColumnType  = Vector<T, R>;
        using RowType     = Vector<T, C>;
        using ElementType = T;
    public:

        [[nodiscard]] constexpr Matrix() noexcept
            : data {ColumnType {}}
        {}
        [[nodiscard]] explicit constexpr Matrix(std::same_as<T> auto... args) noexcept
            requires (sizeof...(args) == C * R)
            : data {ColumnType {}}
        {
            std::array<T, C * R> arrayOfArgs {args...};

            for (std::size_t c = 0; c < C; ++c)
            {
                for (std::size_t r = 0; r < R; ++r)
                {
                    this->data[c][r] = arrayOfArgs[c * R + r]; // NOLINT
                }
            }
        }
        [[nodiscard]] explicit constexpr Matrix(std::same_as<ColumnType> auto... args) noexcept
            requires (sizeof...(args) == C)
            : data {args...}
        {}
        constexpr ~Matrix() = default;

        [[nodiscard]] constexpr Matrix(const Matrix&)     = default;
        [[nodiscard]] constexpr Matrix(Matrix&&) noexcept = default;
        constexpr Matrix& operator= (const Matrix&)       = default;
        constexpr Matrix& operator= (Matrix&&) noexcept   = default;

        ///
        /// Comparison operators
        ///

        [[nodiscard]] constexpr std::partial_ordering
        operator<=> (const Matrix& other) const noexcept
        {
            for (std::size_t c = 0; c < C; ++c)
            {
                for (std::size_t r = 0; r < R; ++r)
                {
                    std::partial_ordering order =
                        std::partial_order(this->data[c][r], other.data[c][r]); // NOLINT

                    if (order != std::partial_ordering::equivalent)
                    {
                        return order;
                    }
                }
            }

            return std::partial_ordering::equivalent;
        }

        [[nodiscard]] constexpr bool operator== (const Matrix& other) const noexcept
        {
            return ((*this) <=> (other)) == std::partial_ordering::equivalent;
        }

        [[nodiscard]] constexpr bool operator!= (const Matrix& other) const noexcept
        {
            return !((*this) == other);
        }

        ///
        /// Access operators
        ///

        [[nodiscard]] constexpr Vector<T, C> getRow(std::size_t row) const noexcept
        {
            Vector<T, C> output {};

            for (std::size_t i = 0; i < C; ++i)
            {
                output[i] = this->data[i][row]; // NOLINT
            }

            return output;
        }

        [[nodiscard]] constexpr ColumnType operator[] (std::size_t idx) const noexcept
        {
            return this->data[idx]; // NOLINT
        }

        [[nodiscard]] constexpr ColumnType& operator[] (std::size_t idx) noexcept
        {
            return this->data[idx]; // NOLINT
        }

        ///
        /// Arithmetic operators
        ///

        template<std::size_t OtherR, std::size_t OtherC>
        [[nodiscard]] constexpr Matrix<T, R, OtherC>
        operator* (const Matrix<T, OtherR, OtherC>& other) const noexcept
            requires (C == OtherR)
        {
            Matrix<T, R, OtherC> output {};

            for (std::size_t outputColumn = 0; outputColumn < OtherC; ++outputColumn)
            {
                for (std::size_t outputRow = 0; outputRow < R; ++outputRow)
                {
                    output[outputColumn][outputRow] =
                        this->getRow(outputRow).dot(other[outputColumn]);
                }
            }

            return output;
        }

        template<std::size_t OtherR>
        [[nodiscard]] constexpr Vector<T, R>
        operator* (const Vector<T, OtherR>& other) const noexcept
            requires (C == OtherR)
        {
            const Matrix<T, R, 1> MatrixOther {other};
            const Matrix<T, R, 1> MultipliedMatrix {(*this) * MatrixOther};

            return MultipliedMatrix[0];
        }

        [[nodiscard]] constexpr Matrix<T, C, R> transpose() const noexcept
        {
            Matrix<T, C, R> output {};

            for (std::size_t i = 0; i < R; ++i)
            {
                output[i] = this->getRow(i);
            }

            return output;
        }

        [[nodiscard]] constexpr T determinant() const noexcept
            requires (R == C)
        {
            if constexpr (R == 1)
            {
                return data[0][0];
            }
            else
            {
                T output {static_cast<T>(0.0)};

                constexpr std::size_t SubMatrixSize = R - 1;

                using SubMatrix = Matrix<T, SubMatrixSize, SubMatrixSize>;

                for (std::size_t currentColumn = 0; currentColumn < R; ++currentColumn)
                {
                    // Create a subMatrix of size (R-1)x(R-1) to calculate its
                    // cofactor.
                    SubMatrix subMatrix;

                    for (std::size_t currentRow = 1; currentRow < R; ++currentRow)
                    {
                        for (std::size_t originalColumn = 0, subMatrixColumn = 0;
                             originalColumn < C;
                             ++originalColumn)
                        {
                            // Exclusion column + row
                            if (originalColumn != currentColumn)
                            {
                                // Fill the subMatrix by skipping the current
                                // column and copying the remaining elements.
                                subMatrix[subMatrixColumn][currentRow - 1] =
                                    data[originalColumn][currentRow];
                                ++subMatrixColumn;
                            }
                        }
                    }
                    T subDeterminant = subMatrix.determinant();

                    // This creates the checkerboard pattern for all recursive
                    // instances
                    if (currentColumn % 2 == 1)
                    {
                        subDeterminant = -subDeterminant;
                    }

                    // Multiply the element of the first row with its cofactor
                    // and accumulate the results.
                    output += data[currentColumn][0] * subDeterminant;
                }

                return output;
            }
        }

        [[nodiscard]] constexpr Matrix<T, R, C> inverse() const
        {
            static_assert(R == C, "Inverse can only be calculated for square matrices.");

            constexpr std::size_t matrixSize    = R;
            constexpr std::size_t subMatrixSize = matrixSize - 1;

            Matrix<T, R, C> result;
            T               det = determinant();

            if (det == T {})
            {
                // Matrix is not invertible
                return result;
            }

            for (std::size_t col = 0; col < matrixSize; ++col)
            {
                for (std::size_t row = 0; row < matrixSize; ++row)
                {
                    // Create a subMatrix of size (matrixSize-1)x(matrixSize-1)
                    // for each element.
                    Matrix<T, subMatrixSize, subMatrixSize> subMatrix;

                    for (std::size_t subCol = 0, origCol = 0; subCol < subMatrixSize;
                         ++subCol, ++origCol)
                    {
                        if (origCol == col)
                        {
                            ++origCol;
                        }

                        for (std::size_t subRow = 0, origRow = 0; subRow < subMatrixSize;
                             ++subRow, ++origRow)
                        {
                            if (origRow == row)
                            {
                                ++origRow;
                            }

                            subMatrix[subCol][subRow] = data[origCol][origRow];
                        }
                    }

                    // Calculate the cofactor of the current element by
                    // recursively calling the determinant() function on the
                    // subMatrix.
                    T cofactor = subMatrix.determinant();

                    // Apply sign change based on the position of the element.
                    T sign = ((row + col) % 2 == 0) ? 1 : -1;

                    // Calculate the adjugate and divide by the determinant to
                    // get the inverse.
                    result[col][row] = (cofactor * sign) / det;
                }
            }

            return result;
        }

        /// Iterator interfacing

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::iterator begin() noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::const_iterator begin() const noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::const_iterator cbegin() const noexcept
        {
            return this->data.begin();
        }

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::iterator end() noexcept
        {
            return this->data.end();
        }

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::const_iterator end() const noexcept
        {
            return this->data.cend();
        }

        [[nodiscard]] constexpr std::array<Vector<T, R>, C>::const_iterator cend() const noexcept
        {
            return this->data.cend();
        }


    private:
        std::array<Vector<T, R>, C> data;
    };

    consteval bool test()
    {
        {
            constexpr Matrix<float, 4, 4> mat0 {};

            for (Vector<float, 4> vec : mat0)
            {
                for (float f : vec)
                {
                    if (f != INFINITY)
                    {
                        return false;
                    }
                }
            }
        }

        // multiplication
        {
            constexpr Matrix<float, 4, 4> mat1 {
                1.0f,
                4.0f,
                8.0f,
                12.0f,
                2.0f,
                5.0f,
                9.0f,
                13.0f,
                3.0f,
                6.0f,
                10.0f,
                14.0f,
                4.0f,
                7.0f,
                11.0f,
                15.0f};
            constexpr Matrix<float, 4, 4> mat2 {mat1 * mat1};
            constexpr Matrix<float, 4, 4> mat3 {
                81.0f,
                156.0f,
                256.0f,
                356.0f,
                91.0f,
                178.0f,
                294.0f,
                410.0f,
                101.0f,
                200.0f,
                332.0f,
                464.0f,
                111.0f,
                222.0f,
                370.0f,
                518.0f};

            static_assert(mat2 == mat3);

            constexpr Matrix<float, 4, 4> mat4 {mat1};
            constexpr Vector<float, 4>    vec0 {1.0f, 2.0f, 3.0f, 4.0f};
            constexpr Vector<float, 4>    vec1 {mat4 * vec0};

            static_assert(vec1 == Vector<float, 4> {30.0f, 60.0f, 100.0f, 140.0f});
        }

        // transpose
        {
            constexpr Matrix<float, 4, 3> mat1 {
                1.0f,
                2.0f,
                3.0f,
                4.0f,

                5.0f,
                6.0f,
                7.0f,
                8.0f,

                10.0f,
                11.0f,
                12.0f,
                13.0f};
            constexpr Matrix<float, 3, 4> mat1trp {mat1.transpose()};
            constexpr Matrix<float, 3, 4> mat2 {
                1.0f,
                5.0f,
                10.0f,

                2.0f,
                6.0f,
                11.0f,

                3.0f,
                7.0f,
                12.0f,

                4.0f,
                8.0f,
                13.0f};

            static_assert(mat2 == mat1trp);
        }

        // determinant testing
        {
            constexpr Matrix<float, 4, 4> mat1 {
                1.0f,
                5.0f,
                9.0f,
                13.0f,

                2.0f,
                6.0f,
                10.0f,
                14.0f,

                3.0f,
                7.0f,
                11.0f,
                15.0f,

                4.0f,
                8.0f,
                12.0f,
                16.0f};

            constexpr float det1 {mat1.determinant()};
            static_assert(det1 == 0.0f);

            constexpr Matrix<float, 3, 3> mat2 {
                1.0f, 3.0f, -5.0f, 2.0f, 0.0f, 4.0f, -1.0f, 1.0f, 2.0f};

            static_assert(mat2.determinant() == -38.0f);

            constexpr Matrix<float, 2, 2> mat3 {1.0f, 3.0f, 2.0f, 4.0f};
            static_assert(mat3.determinant() == -2.0f);
        }

        // Inverse
        {
            constexpr Matrix<float, 2, 2> mat0 {1.0f, 3.0f, 2.0f, 4.0f};
            constexpr Matrix<float, 2, 2> mat0Inverse {-2.0f, 1.0f, 1.5f, -0.5f};

            static_assert(mat0.inverse() == mat0Inverse);

            // non invertible
            constexpr Matrix<float, 4, 4> mat1 {
                1.0f,
                2.0f,
                3.0f,
                4.0f,

                5.0f,
                6.0f,
                7.0f,
                8.0f,

                9.0f,
                10.0f,
                11.0f,
                12.0f,

                13.0f,
                14.0f,
                15.0f,
                16.0f};

            constexpr Matrix<float, 4, 4> mat1Inverse {};

            static_assert(mat1.inverse() == mat1Inverse);
        }

        return true;
    }

    static_assert(test());

} // namespace util

#endif // SRC_UTIL_MATRIX_HPP