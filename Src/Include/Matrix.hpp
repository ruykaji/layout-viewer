#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

namespace matrix {

struct Shape {
    uint8_t m_x = 0;
    uint8_t m_y = 0;
    uint8_t m_z = 0;
};

class Matrix {
public:
    /** =============================== CONSTRUCTORS ================================= */

    /**
     * @brief Default constructor
     *
     * @param shape Shape of the matrix
     */
    Matrix(const Shape& shape = {});

    ~Matrix();

    /**
     * @brief Copy constructor
     *
     * @param matrix Matrix that about to be copied
     */
    Matrix(const Matrix& matrix);

    /**
     * @brief Move constructor
     *
     * @param matrix Matrix that about to be moved
     */
    Matrix(Matrix&& matrix);

public:
    /** =============================== OPERATORS ==================================== */

    /**
     * @brief Copy assignment
     *
     * @param matrix Matrix that about to be copied
     * @return Matrix&
     */
    Matrix& operator=(const Matrix& matrix);

    /**
     * @brief Move operator
     *
     * @param matrix Matrix that about to be moved
     * @return Matrix&
     */
    Matrix& operator=(Matrix&& matrix);

public:
    /** =============================== PUBLIC METHODS =============================== */

    /**
     * @brief Get matrix shape
     *
     * @return const Shape&
     */
    const Shape& shape()
    {
        return m_shape;
    }

    /**
     * @brief Get the value at the (Z,Y,X) position
     *
     * @param z
     * @param y
     * @param x
     * @return uint8_t
     */
    uint8_t get_at(const uint8_t x, const uint8_t y, const uint8_t z) const;

    /**
     * @brief Set a value at the (Z,Y,X) position
     *
     * @param value
     * @param z
     * @param y
     * @param x
     * @return uint8_t
     */
    void set_at(const uint8_t value, const uint8_t x, const uint8_t y, const uint8_t z);

    /**
     * @brief Clears matrix
     *
     */
    void clear() noexcept(true);

private:
    /** =============================== PRIVATE METHODS ============================== */

    /**
     * @brief Allocates memory and returns number of allocated elements
     *
     * @return std::size_t
     */
    std::size_t allocate();

private:
    Shape m_shape;
    uint8_t* m_data;
};

} // namespace matrix

#endif