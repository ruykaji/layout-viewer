#ifndef __PIN_HPP__
#define __PIN_HPP__

#include <string>
#include <vector>

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

struct Port
{
  types::Rectangle m_rect;
  types::Metal     m_metal;
};

struct Pin
{
  Use               m_use;
  Direction         m_direction;
  std::string       m_name;
  std::vector<Port> m_ports;
};

/**
 * @brief Sets a direction to the pin.
 *
 * @param pin The Pin.
 * @param direction The direction.
 */
void
set_direction(Pin& pin, const std::string_view direction);

/**
 * @brief Sets a use to the pin.
 *
 * @param pin The pin.
 * @param use The use.
 */
void
set_use(Pin& pin, const std::string_view use);

} // namespace pin

#endif