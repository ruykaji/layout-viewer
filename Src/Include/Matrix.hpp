#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace matrix
{

struct Node
{
  uint32_t m_x;
  uint32_t m_y;
  uint32_t m_z;

  double   m_cost;     // accumulated cost
  uint32_t m_source_x; // original terminal x
  uint32_t m_source_y; // original terminal y

  struct Hash
  {
    std::size_t
    operator()(const Node& node) const
    {
      auto hash = std::hash<uint32_t>()(node.m_x);
      hash ^= std::hash<uint32_t>()(node.m_y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      hash ^= std::hash<uint32_t>()(node.m_z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      return hash;
    }
  };

  struct Equal
  {
    bool
    operator()(const Node& lhs, const Node& rhs) const
    {
      return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y && lhs.m_z == rhs.m_z;
    }
  };

  friend bool
  operator==(const Node& lhs, const Node& rhs)
  {
    return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y && lhs.m_z == rhs.m_z;
  }
};

struct CompareNode
{
  bool
  operator()(const matrix::Node& a, const matrix::Node& b) const
  {
    return a.m_cost > b.m_cost;
  }
};

using MapOfNodes = std::unordered_map<Node, uint32_t, Node::Hash, Node::Equal>;
using SetOfNodes = std::unordered_set<Node, Node::Hash, Node::Equal>;

struct Shape
{
  std::size_t m_x = 0;
  std::size_t m_y = 0;
  std::size_t m_z = 0;

  friend bool
  operator==(const Shape& lhs, const Shape& rhs)
  {
    return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y && lhs.m_z == rhs.m_z;
  }

  friend bool
  operator!=(const Shape& lhs, const Shape& rhs)
  {
    return !(lhs == rhs);
  }
};

class Matrix
{
public:
  /** =============================== CONSTRUCTORS ================================= */

  /**
   * @brief Constructs a Matrix with the specified shape
   *
   * @param shape Shape structure containing the dimensions of the matrix
   */
  Matrix(const Shape& shape = {});

  /**
   * @brief  Destroys the Matrix object.
   *
   */
  ~Matrix();

  /**
   * @brief Copy constructor.
   *
   * @param matrix The matrix to be copied.
   */
  Matrix(const Matrix& matrix);

  /**
   * @brief Move constructor.
   *
   * @param matrix The matrix to be moved.
   */
  Matrix(Matrix&& matrix);

public:
  /** =============================== OPERATORS ==================================== */

  /**
   * @brief Copy assignment operator.
   *
   * @param matrix The matrix to copy from.
   * @return Matrix&
   */
  Matrix&
  operator=(const Matrix& matrix);

  /**
   * @brief Move assignment operator.
   *
   * @param matrix The matrix to be moved.
   * @return Matrix&
   */
  Matrix&
  operator=(Matrix&& matrix);

  /**
   * @brief Sum-assignment of two matrices.
   *
   * @param matrix Matrix to sum with.
   * @return Matrix&
   */
  Matrix&
  operator+=(const Matrix& matrix);

public:
  /** =============================== PUBLIC STATIC METHODS =============================== */

  /**
   * @brief Masks matrix using another matrix.
   *
   * @param matrix Matrix to mask.
   * @param mask Mask to use.
   */
  static void
  mask(Matrix& matrix, const Matrix& mask);

public:
  /** =============================== PUBLIC METHODS =============================== */

  /**
   * @brief Returns pointer to the underlying data.
   *
   * @return double*
   */
  double*
  data();

  /**
   * @brief Returns pointer to the underlying data.
   *
   * @return const double*
   */
  const double*
  data() const;

  /**
   * @brief Returns the shape of the matrix.
   *
   * @return const Shape&
   */
  const Shape&
  shape() const;

  /**
   * @brief Retrieves the value stored at the given (x, y, z) coordinates.
   *
   * @param x X-coordinate (width).
   * @param y Y-coordinate (height).
   * @param z Z-coordinate (depth).
   * @return const double&
   */
  const double&
  get_at(const uint32_t x, const uint32_t y, const uint32_t z) const;

  /**
   * @brief Sets the value at the given (x, y, z) coordinates.
   *
   * @param value The value to set at the given coordinates.
   * @param x X-coordinate (width).
   * @param y Y-coordinate (height).
   * @param z Z-coordinate (depth).
   */
  void
  set_at(const double value, const uint32_t x, const uint32_t y, const uint32_t z);

  /**
   * @brief Clears the matrix data.
   *
   */
  void
  clear() noexcept(true);

private:
  /** =============================== PRIVATE METHODS ============================== */

  /**
   * @brief Allocates memory for matrix elements.
   *
   * @return std::size_t
   */
  std::size_t
  allocate();

public:
  Shape m_shape; ///< Holds the dimensions of the matrix.

private:
  double* m_data; ///< Pointer to the dynamically allocated matrix data.
};

} // namespace matrix

#endif