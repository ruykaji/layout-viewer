#include "Include/Pin.hpp"

namespace pin
{

void
Pin::set_direction(const std::string_view direction)
{
  if(direction == "FEEDTHRU")
    {
      m_direction = Direction::FEEDTHRU;
    }
  else if(direction == "INPUT")
    {
      m_direction = Direction::INPUT;
    }
  else if(direction == "OUTPUT")
    {
      m_direction = Direction::OUTPUT;
    }
  else if(direction == "INOUT")
    {
      m_direction = Direction::INOUT;
    }
}

void
Pin::set_use(const std::string_view use)
{
  if(use == "ANALOG")
    {
      m_use = Use::ANALOG;
    }
  else if(use == "SCAN")
    {
      m_use = Use::SCAN;
    }
  else if(use == "TIEOFF")
    {
      m_use = Use::TIEOFF;
    }
  else if(use == "RESET")
    {
      m_use = Use::RESET;
    }
  else if(use == "SIGNAL")
    {
      m_use = Use::SIGNAL;
    }
  else if(use == "CLOCK")
    {
      m_use = Use::CLOCK;
    }
  else if(use == "POWER")
    {
      m_use = Use::POWER;
    }
  else if(use == "GROUND")
    {
      m_use = Use::GROUND;
    }
}

} // namespace pin