#include <iostream>
#include <unordered_set>

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
fill_gcells(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets)
{
  using GCellsWithOverlaps = std::vector<std::pair<def::GCell*, types::Rectangle>>;

  /** Add global obstacles to gcells */
  for(auto& [rect, metal] : def_data.m_obstacles)
    {
      GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gwo)
        {
          gcell->m_obstacles.emplace_back(overlap, metal);
        }
    }

  /** TODO: Try to replace with something less memory required data structures, or it will go away itself as i replace guide file with my own global routing */
  std::unordered_map<def::GCell*, std::unordered_map<pin::Pin*, std::vector<std::pair<types::Rectangle, types::Metal>>>> gcell_to_pins;
  std::unordered_map<pin::Pin*, std::unordered_set<def::GCell*>>                                                         pin_to_gcells;

  for(auto& [_, pin] : def_data.m_pins)
    {
      for(auto& [rect, metal] : pin->m_ports)
        {
          GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

          for(auto& [gcell, overlap] : gwo)
            {
              gcell_to_pins[gcell][pin].emplace_back(std::move(overlap), metal);
              pin_to_gcells[pin].emplace(gcell);
            }
        }
    }

  /** Add component's pins and obstacles to gcells */
  for(auto& component : def_data.m_components)
    {
      if(lef_data.m_macros.count(component.m_name) == 0)
        {
          throw std::runtime_error("Process Error: Couldn't find a macro with the name - \"" + component.m_name + "\".");
        }

      lef::Macro   macro  = lef_data.m_macros.at(component.m_name);
      const double width  = macro.m_width * lef_data.m_database_number;
      const double height = macro.m_height * lef_data.m_database_number;

      // for(auto& obs : macro.m_obs)
      //   {
      //     for(auto& rect : obs.m_rect)
      //       {
      //         details::scale_by(rect, lef_data.m_database_number);
      //         details::apply_orientation(rect, component.m_orientation, width, height);
      //         details::place_at(rect, component.m_x, component.m_y, height);

      //         GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

      //         for(auto& [gcell, overlap] : gwo)
      //           {
      //             gcell->m_obstacles.emplace_back(rect, obs.m_metal);
      //           }
      //       }
      //   }

      for(auto& [name, pin] : macro.m_pins)
        {
          pin::Pin* new_pin = new pin::Pin(pin);

          for(auto& [rect, metal] : new_pin->m_ports)
            {
              details::scale_by(rect, lef_data.m_database_number);
              details::apply_orientation(rect, component.m_orientation, width, height);
              details::place_at(rect, component.m_x, component.m_y, height);

              GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gwo)
                {
                  gcell_to_pins[gcell][new_pin].emplace_back(std::move(overlap), metal);
                  pin_to_gcells[new_pin].emplace(gcell);
                }
            }

          def_data.m_pins[component.m_id + ":" + name] = new_pin;
        }
    }

  /** Apply global routing to gcells and pins */
  for(const auto& guide_net : nets)
    {
      if(def_data.m_nets.count(guide_net.m_name) == 0)
        {
          std::cout << "Process Warning: Couldn't find a net with the name - \"" << guide_net.m_name << "\"." << std::endl;
          continue;
        }

      const def::Net& current_net = def_data.m_nets.at(guide_net.m_name);

      for(const auto& [rect, metal] : guide_net.m_path)
        {
          GCellsWithOverlaps    gwo       = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);
          lef::Layer::Direction direction = lef_data.m_layers.at(metal).m_direction;

          for(std::size_t i = 0, end = gwo.size(); i < end; ++i)
            {
              def::GCell* gcell = gwo[i].first;

              /** Add edge pins and assign them to tracks */
              if(i > 0)
                {
                  def::GCell*                     prev_gcell        = gwo[i - 1].first;

                  std::vector<def::GCell::Track>& prev_gcell_tracks = direction == lef::Layer::Direction::HORIZONTAL ? prev_gcell->m_tracks_x : prev_gcell->m_tracks_y;
                  std::vector<def::GCell::Track>& gcell_tracks      = direction == lef::Layer::Direction::HORIZONTAL ? gcell->m_tracks_x : gcell->m_tracks_y;

                  for(std::size_t j = 0, end = prev_gcell_tracks.size(); j < end; ++j)
                    {
                      if(prev_gcell_tracks[j].m_metal == metal && prev_gcell_tracks[j].m_rn == 0)
                        {
                          prev_gcell_tracks[j].m_rn = current_net.m_idx;
                          break;
                        }
                    }

                  for(std::size_t j = 0, end = gcell_tracks.size(); j < end; ++j)
                    {
                      if(gcell_tracks[j].m_metal == metal && gcell_tracks[j].m_ln == 0)
                        {
                          gcell_tracks[j].m_ln = current_net.m_idx;
                          break;
                        }
                    }
                }

              if(gcell_to_pins.count(gcell) == 0)
                {
                  continue;
                }

              const auto& gcell_pins = gcell_to_pins.at(gcell);

              for(const auto& name : current_net.m_pins)
                {
                  pin::Pin* pin = def_data.m_pins.at(name);

                  if(pin->m_is_placed)
                    {
                      continue;
                    }

                  if(gcell_pins.count(pin))
                    {
                      const std::unordered_set<def::GCell*>& pins_gcells = pin_to_gcells.at(pin);

                      for(auto& another_gcell : pins_gcells)
                        {
                          if(another_gcell != gcell)
                            {
                              /** For this cell current pin's geometry is an obstacle */
                              // const std::vector<std::pair<types::Rectangle, types::Metal>>& obstacles = gcell_to_pins.at(another_gcell).at(pin);
                              // another_gcell->m_obstacles.insert(another_gcell->m_obstacles.end(), std::make_move_iterator(obstacles.begin()), std::make_move_iterator(obstacles.end()));
                            }
                        }

                      /** Place current pin in a gcell */
                      pin->m_is_placed                      = true;
                      pin->m_ports                          = std::move(gcell_pins.at(pin));

                      gcell->m_pins[name]                   = pin;
                      gcell->m_nets[guide_net.m_name].m_idx = current_net.m_idx;
                      gcell->m_nets[guide_net.m_name].m_pins.emplace_back(name);
                    }
                }
            }
        }
    }
}

} // namespace process
