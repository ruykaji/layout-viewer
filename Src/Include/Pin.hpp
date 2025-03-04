#ifndef __PIN_HPP__
#define __PIN_HPP__

#include <forward_list>
#include <string>
#include <vector>

#include "Include/Geometry.hpp"
#include "Include/Types.hpp"

namespace pin
{

enum class Use
{
  SIGNAL = 0,
  ANALOG,
  CLOCK,
  GROUND,
  POWER,
  RESET,
  SCAN,
  TIEOFF
};

enum class Direction
{
  FEEDTHRU = 0,
  INPUT,
  OUTPUT,
  INOUT,
};

class Pin
{
public:
  bool                       m_is_placed = false;
  std::string                m_name;
  Use                        m_use;
  Direction                  m_direction;
  geom::Point                m_center;
  std::vector<geom::Polygon> m_ports;
  std::vector<geom::Polygon> m_obs;

public:
  /**
   * @brief Sets a direction to the pin.
   *
   * @param pin The Pin.
   * @param direction The direction as a string.
   */
  void
  set_direction(const std::string_view direction);

  /**
   * @brief Sets a use to the pin.
   *
   * @param pin The pin.
   * @param use The use as a string.
   */
  void
  set_use(const std::string_view use);
};

} // namespace pin

#endif