#ifndef __DEF_PIN_HPP__
#define __DEF_PIN_HPP__

#include "Include/DEF/Net.hpp"
#include "Include/DEF/Utils.hpp"
#include "Include/Pin.hpp"

namespace def
{

class Pin
{
public:
  Pin(pin::Pin* ptr, Net* net)
      : m_ptr(ptr), m_net(net), m_type(Type::INNER)
  {
    m_access_points.m_metal = ptr->m_ports.at(0).m_metal == types::Metal::L1 ? types::Metal::M1 : ptr->m_ports.at(0).m_metal;
  };

public:
  /**
   * @brief Get the real bounding of a pin.
   *
   * @return std::pair<geom::Point, geom::Point>
   */
  std::pair<geom::Point, geom::Point>
  get_bounding_box() const
  {
    if(m_ptr == nullptr)
      {
        throw std::runtime_error("DEF Pin Error: Nullptr parent pin");
      }

    if(m_ptr->m_ports.empty())
      {
        throw std::runtime_error("DEF Pin Error: Expected to have port geometry");
      }

    const geom::Polygon& port           = m_ptr->m_ports[0];
    const auto [left_top, right_bottom] = port.get_extrem_points();

    return port.get_extrem_points();
  };

  /**
   * @brief Checks if a given target intersect with a pin.
   *
   * @param target A target to check intersection with.
   * @return std::pair<std::vector<geom::Point>, bool>
   */
  std::pair<std::vector<geom::Point>, bool>
  check_intersection(const geom::Polygon& target) const
  {
    if(m_ptr == nullptr)
      {
        throw std::runtime_error("DEF Pin Error: Nullptr parent pin");
      }

    if(target.m_points.empty())
      {
        throw std::runtime_error("DEF Pin Error: Expected target to have geometry");
      }

    const auto [left_top, right_bottom] = target.get_extrem_points();

    if(right_bottom.x - left_top.x == 0 && right_bottom.y - left_top.y == 0)
      {
        throw std::runtime_error("DEF Pin Error: Expected target to have non-zero geometry");
      }

    m_ptr->m_ports[0] / target;
  }

public:
  enum class Type
  {
    INNER = 0,
    CROSS,
    BETWEEN_STACKS
  } m_type;

  pin::Pin*           m_ptr;           ///> Pointer to the top level pin.
  Net*                m_net;           ///> Pointer to a parent net.
  geom::PointS        m_center;        ///> Selected access point projection on a base grid.
  geom::PointS        m_matrix_pos;    ///> Position on a matrix.
  utils::AccessPoints m_access_points; ///> All point that can be used as access to a pin.
};

} // namespace def

#endif