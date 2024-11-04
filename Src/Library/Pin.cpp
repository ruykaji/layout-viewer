#include "Include/Pin.hpp"

namespace pin
{

/** =============================== PUBLIC METHODS =============================== */

void
set_direction(Pin& pin, const std::string_view direction)
{
  if(direction == "FEEDTHRU")
    {
      pin.m_direction = Direction::FEEDTHRU;
    }
  else if(direction == "INPUT")
    {
      pin.m_direction = Direction::INPUT;
    }
  else if(direction == "OUTPUT")
    {
      pin.m_direction = Direction::OUTPUT;
    }
  else if(direction == "INOUT")
    {
      pin.m_direction = Direction::INOUT;
    }
}

void
set_use(Pin& pin, const std::string_view use)
{
  if(use == "ANALOG")
    {
      pin.m_use = Use::ANALOG;
    }
  else if(use == "SCAN")
    {
      pin.m_use = Use::SCAN;
    }
  else if(use == "TIEOFF")
    {
      pin.m_use = Use::TIEOFF;
    }
  else if(use == "RESET")
    {
      pin.m_use = Use::RESET;
    }
  else if(use == "SIGNAL")
    {
      pin.m_use = Use::SIGNAL;
    }
  else if(use == "CLOCK")
    {
      pin.m_use = Use::CLOCK;
    }
  else if(use == "POWER")
    {
      pin.m_use = Use::POWER;
    }
  else if(use == "GROUND")
    {
      pin.m_use = Use::GROUND;
    }
}

} // namespace pin