#include "Include/Process.hpp"

namespace process
{

namespace details
{

void
apply_orientation(types::Rectangle& rect, types::Orientation orientation, double width, double height)
{
  for(size_t i = 0; i < rect.size(); i += 2)
    {
      double x = rect[i];
      double y = rect[i + 1];

      switch(orientation)
        {
        case types::Orientation::S:
          {
            rect[i]     = width - x;
            rect[i + 1] = height - y;
            break;
          }
        case types::Orientation::E:
          {
            rect[i]     = height - y;
            rect[i + 1] = x;
            break;
          }
        case types::Orientation::W:
          {
            rect[i]     = y;
            rect[i + 1] = width - x;
            break;
          }
        case types::Orientation::FN:
          {
            rect[i] = width - x;
            break;
          }
        case types::Orientation::FS:
          {
            rect[i + 1] = height - y;
            break;
          }
        case types::Orientation::FE:
          {
            rect[i]     = y;
            rect[i + 1] = x;
            break;
          }
        case types::Orientation::FW:
          {
            rect[i]     = height - y;
            rect[i + 1] = width - x;
            break;
          }
        default:
          break;
        }
    }

  if(rect[3] > rect[5])
    {
      std::swap(rect[3], rect[5]);
      std::swap(rect[1], rect[7]);
    }

  if(rect[0] > rect[2])
    {
      std::swap(rect[0], rect[2]);
      std::swap(rect[4], rect[6]);
    }
}

void
scale_by(types::Rectangle& rect, double scale)
{
  for(auto& point : rect)
    {
      point *= scale;
    }
}

void
place_at(types::Rectangle& rect, double x, double y, double height)
{
  for(std::size_t i = 0, end = rect.size(); i < end; i += 2)
    {
      rect[i] += x;
      rect[i + 1] += y;
    }
}

} // namespace details

void
fill_gcells(def::Data& def_data, const lef::Data& lef_data)
{
  /** Add global pins to gcells */
  for(auto& [name, pin] : def_data.m_pins)
    {
      for(auto& port : pin->m_ports)
        {
          auto gcells_with_overlap = def::GCell::find_overlaps(port.m_rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

          for(auto& [gcell, _] : gcells_with_overlap)
            {
              if(!gcell->m_pins.count(name))
                {
                  gcell->m_pins.emplace(name, pin);
                }
            }
        }
    }

  /** Add global obstacles to gcells */
  for(auto& [rect, metal] : def_data.m_obstacles)
    {
      auto gcells_with_overlap = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gcells_with_overlap)
        {
          gcell->m_obstacles.emplace_back(overlap, metal);
        }
    }

  /** Add component's pins and obstacles to gcells */
  for(auto& component : def_data.m_components)
    {
      if(!lef_data.m_macros.count(component.m_name))
        {
          continue;
        }

      lef::Macro   macro  = lef_data.m_macros.at(component.m_name);
      const double width  = macro.m_width * lef_data.m_database_number;
      const double height = macro.m_height * lef_data.m_database_number;

      for(auto& obs : macro.m_obs)
        {
          for(auto& rect : obs.m_rect)
            {
              details::scale_by(rect, lef_data.m_database_number);
              details::apply_orientation(rect, component.m_orientation, width, height);
              details::place_at(rect, component.m_x, component.m_y, height);

              auto gcells_with_overlap = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gcells_with_overlap)
                {
                  gcell->m_obstacles.emplace_back(rect, obs.m_metal);
                }
            }
        }

      for(auto& pin : macro.m_pins)
        {
          const std::string pin_name = component.m_id + ":" + pin.m_name;
          def_data.m_pins[pin_name]  = new pin::Pin(pin);

          for(auto& port : def_data.m_pins.at(pin_name)->m_ports)
            {
              details::scale_by(port.m_rect, lef_data.m_database_number);
              details::apply_orientation(port.m_rect, component.m_orientation, width, height);
              details::place_at(port.m_rect, component.m_x, component.m_y, height);

              auto gcells_with_overlap = def::GCell::find_overlaps(port.m_rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, _] : gcells_with_overlap)
                {
                  if(!gcell->m_pins.count(pin_name))
                    {
                      gcell->m_pins.emplace(pin_name, def_data.m_pins.at(pin_name));
                    }
                }
            }
        }
    }
}

} // namespace process
