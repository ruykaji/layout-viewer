#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <unordered_set>

#include "Include/Process.hpp"
#include "Include/Utils.hpp"

namespace process
{

namespace details
{

/** Support function for placement in gcell */
void
apply_orientation(types::Polygon& rect, types::Orientation orientation, double width, double height)
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
scale_by(types::Polygon& rect, double scale)
{
  for(auto& point : rect)
    {
      point *= scale;
    }
}

void
place_at(types::Polygon& rect, double x, double y, double height)
{
  for(std::size_t i = 0, end = rect.size(); i < end; i += 2)
    {
      rect[i] += x;
      rect[i + 1] += y;
    }
}

/** Support functions for track assignment */
std::pair<double, double>
calculate_centroid(const std::vector<double>& polygon)
{
  const std::size_t n    = polygon.size() / 2;

  double            area = 0.0;
  double            cx   = 0.0;
  double            cy   = 0.0;

  for(std::size_t i = 0; i < n; i += 2)
    {
      const double x1 = polygon[2 * i];
      const double y1 = polygon[2 * i + 1];
      const double x2 = polygon[2 * ((i + 1) % n)];
      const double y2 = polygon[2 * ((i + 1) % n) + 1];

      area += (x1 * y2 - x2 * y1);

      const double factor = (x1 * y2 - x2 * y1);

      cx += (x1 + x2) * factor;
      cy += (y1 + y2) * factor;
    }

  area *= 3.0;

  return { cx / area, cy / area };
}

bool
is_point_inside_polygon(double px, double py, const std::vector<double>& polygon)
{
  std::size_t n      = polygon.size() / 2;
  bool        inside = false;

  for(int i = 0, j = n - 1; i < n; j = i++)
    {
      double xi = polygon[2 * i], yi = polygon[2 * i + 1];
      double xj = polygon[2 * j], yj = polygon[2 * j + 1];

      bool   intersect = ((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi);

      if(intersect)
        {
          inside = !inside;
        }
    }

  return inside;
}

std::pair<double, double>
find_closest_point_in_polygon(const std::pair<double, double>& centroid, const std::vector<double>& polygon)
{
  const std::size_t         n = polygon.size() / 2; // Number of vertices

  std::pair<double, double> closest_point;
  double                    min_dist_sq = std::numeric_limits<double>::max();

  for(std::size_t i = 0; i < n; ++i)
    {
      const std::pair<double, double> a  = { polygon[2 * i], polygon[2 * i + 1] };
      const std::pair<double, double> b  = { polygon[2 * ((i + 1) % n)], polygon[2 * ((i + 1) % n) + 1] };

      const double                    ax = b.first - a.first;
      const double                    ay = b.second - a.second;
      double                          t  = ((centroid.first - a.first) * ax + (centroid.second - a.second) * ay) / (ax * ax + ay * ay);

      if(t < 0.0)
        {
          t = 0.0;
        }

      if(t > 1.0)
        {
          t = 1.0;
        }

      const std::pair<double, double> projection = { a.first + t * ax, a.second + t * ay };
      const double                    dist_sq    = (centroid.first - projection.first) * (centroid.first - projection.first) + (centroid.second - projection.second) * (centroid.second - projection.second);

      if(dist_sq < min_dist_sq)
        {
          min_dist_sq   = dist_sq;
          closest_point = projection;
        }
    }

  return closest_point;
}

} // namespace details

void
apply_global_routing(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets)
{
  using GCellsWithOverlaps = std::vector<std::pair<def::GCell*, types::Polygon>>;

  /** Add global obstacles to gcells */
  for(auto& [rect, metal] : def_data.m_obstacles)
    {
      if(metal == types::Metal::L1 || metal == types::Metal::NONE)
        {
          continue;
        }

      GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gwo)
        {
          gcell->m_obstacles.emplace_back(overlap, metal);
        }
    }

  /** TODO: Try to replace with something less memory required data structures, or it will go away itself as i replace guide file with my own global routing */
  std::unordered_map<def::GCell*, std::unordered_map<pin::Pin*, std::vector<std::pair<types::Polygon, types::Metal>>>> gcell_to_pins;
  std::unordered_map<pin::Pin*, std::unordered_set<def::GCell*>>                                                       pin_to_gcells;

  for(auto& [_, pin] : def_data.m_pins)
    {
      for(auto& [rect, metal] : pin->m_ports)
        {
          if(metal == types::Metal::NONE)
            {
              continue;
            }

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

      for(auto& obs : macro.m_obs)
        {
          if(obs.m_metal == types::Metal::L1 || obs.m_metal == types::Metal::NONE)
            {
              continue;
            }

          for(auto& rect : obs.m_rect)
            {
              details::scale_by(rect, lef_data.m_database_number);
              details::apply_orientation(rect, component.m_orientation, width, height);
              details::place_at(rect, component.m_x, component.m_y, height);

              GCellsWithOverlaps gwo = def::GCell::find_overlaps(rect, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gwo)
                {
                  gcell->m_obstacles.emplace_back(overlap, obs.m_metal);
                }
            }
        }

      for(auto& [name, pin] : macro.m_pins)
        {
          pin::Pin* new_pin = new pin::Pin(pin);

          for(auto& [rect, metal] : new_pin->m_ports)
            {
              if(metal == types::Metal::NONE)
                {
                  continue;
                }

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

  /** Apply global routing to gcells and pins from a guide file */
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
                  gcell->m_nets[guide_net.m_name].m_idx = current_net.m_idx;
                  gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(metal, 1));

                  def::GCell* prev_gcell                     = gwo[i - 1].first;

                  prev_gcell->m_nets[guide_net.m_name].m_idx = current_net.m_idx;
                  prev_gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(metal, -1));
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
                              const std::vector<std::pair<types::Polygon, types::Metal>>& obstacles = gcell_to_pins.at(another_gcell).at(pin);

                              for(const auto& [poly, metal] : obstacles)
                                {
                                  if(metal == types::Metal::L1 || metal == types::Metal::NONE)
                                    {
                                      continue;
                                    }

                                  another_gcell->m_obstacles.emplace_back(poly, metal);
                                }
                            }
                        }

                      /** Place current pin in a gcell */
                      pin->m_is_placed                      = true;
                      pin->m_ports                          = std::move(gcell_pins.at(pin));

                      gcell->m_pins[name]                   = pin;
                      gcell->m_nets[guide_net.m_name].m_idx = current_net.m_idx;
                      gcell->m_nets[guide_net.m_name].m_pins.emplace_back(name);

                      def_data.m_nets_gcells[guide_net.m_name].emplace_back(gcell);
                    }
                }
            }
        }
    }

  /** Remove all not used gcells and collect statistics */
  std::size_t max_nets     = 0;
  std::size_t max_pins     = 0;

  std::size_t min_nets     = UINT64_MAX;
  std::size_t min_pins     = UINT64_MAX;

  std::size_t total_gcells = 0;
  std::size_t total_nets   = 0;
  std::size_t total_pins   = 0;

  for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
    {
      auto itr = std::remove_if(def_data.m_gcells[y].begin(), def_data.m_gcells[y].end(), [&max_nets, &min_nets, &max_pins, &min_pins, &total_gcells, &total_nets, &total_pins](const def::GCell* ptr) {
        if(ptr->m_pins.size() == 0 && ptr->m_edge_pins.empty())
          {
            delete ptr;
            return true;
          }

        /** Collect statistics */
        max_nets = std::max(max_nets, ptr->m_nets.size());
        min_nets = std::min(min_nets, ptr->m_nets.size());
        max_pins = std::max(max_pins, ptr->m_pins.size() + ptr->m_edge_pins.size());
        min_pins = std::min(min_pins, ptr->m_pins.size() + ptr->m_edge_pins.size());

        total_nets += ptr->m_nets.size();
        total_pins += ptr->m_pins.size() + ptr->m_edge_pins.size();
        ++total_gcells;

        if(ptr->m_pins.size() + ptr->m_edge_pins.size() == 1)
          {
            std::cout << "Warning Routing: GCell contains only one pin!" << std::endl;
          }

        return false;
      });
      def_data.m_gcells[y].erase(itr, def_data.m_gcells[y].end());
    }

  auto itr = std::remove_if(def_data.m_gcells.begin(), def_data.m_gcells.end(), [](const auto vec) { return vec.size() == 0; });
  def_data.m_gcells.erase(itr, def_data.m_gcells.end());

  std::cout << "Gcell statistics:" << '\n';
  std::cout << "  - Nets [min, abg, max] : [" << min_nets << ',' << total_nets / total_gcells << ',' << max_nets << "]\n";
  std::cout << "  - Pins [min, avg, max]: [" << min_pins << ',' << total_pins / total_gcells << ',' << max_pins << "]\n";
  std::cout << std::flush;
}

std::vector<std::vector<Task>>
make_tasks(def::Data& def_data)
{
  const std::size_t              size             = 32; /** TODO: Remove this hardcode */
  const std::size_t              number_of_metals = def_data.m_tracks_x.size();
  const auto&                    pins             = def_data.m_pins;

  std::vector<std::vector<Task>> tasks;
  tasks.resize(def_data.m_gcells.size());

  for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(std::size_t x = 0, end_x = def_data.m_gcells[y].size(); x < end_x; ++x)
        {
          Task task;
          task.m_matrix           = matrix::Matrix({ size * 2, size * 2, uint8_t(def_data.m_tracks_x.size()) });

          def::GCell*  gcell      = def_data.m_gcells[y][x];

          /** Place tracks */
          /** TODO: Move to a separate function */
          const double m1_offset  = def_data.m_tracks_x[0].m_start;
          const double m1_step    = def_data.m_tracks_x[0].m_spacing;

          const double m1_left_x  = gcell->m_tracks_y.front().m_box[1];
          const double m1_right_x = gcell->m_tracks_y.back().m_box[1];

          const double m1_left_y  = gcell->m_tracks_x.front().m_box[0];
          const double m1_right_y = gcell->m_tracks_x.back().m_box[0];

          for(std::size_t m = 0; m < number_of_metals; ++m)
            {
              if(m == 0)
                {
                  /** First layer always fill whole matrix as it suppose to be */
                  for(std::size_t y = 0, end_y = size * 2; y < end_y; y += 2)
                    {
                      for(std::size_t x = 0, end_x = size * 2; x < end_x; ++x)
                        {
                          task.m_matrix.set_at(uint8_t(types::Cell::TRACE), x, y, m);
                        }
                    }
                }
              else
                {
                  const double offset = def_data.m_tracks_x[m].m_start;
                  const double step   = def_data.m_tracks_x[m].m_spacing;

                  if(m % 2 == 0)
                    {
                      double left = offset + std::ceil((m1_left_x - offset) / step) * step;

                      while(left < m1_right_x)
                        {
                          int64_t n = std::floor((left - m1_offset) / m1_step) - gcell->m_box[0];

                          /** Add track by x */
                          for(std::size_t x = 0, end_x = size * 2; x < end_x; ++x)
                            {
                              task.m_matrix.set_at(uint8_t(types::Cell::TRACE), x, n * 2, m);

                              if(task.m_matrix.get_at(x, n * 2, m - 1) == uint8_t(types::Cell::TRACE))
                                {
                                  task.m_matrix.set_at(uint8_t(types::Cell::INTERSECTION_VIA), x, n * 2, m - 1);
                                }
                            }

                          left += step;
                        }
                    }
                  else
                    {
                      double left = offset + std::ceil((m1_left_y - offset) / step) * step;

                      while(left < m1_right_y)
                        {
                          int64_t n = std::floor((left - m1_offset) / m1_step) - gcell->m_box[1];

                          /** Add track by y*/
                          for(std::size_t y = 0, end_y = size * 2; y < end_y; ++y)
                            {
                              task.m_matrix.set_at(uint8_t(types::Cell::TRACE), n * 2, y, m);

                              if(task.m_matrix.get_at(n * 2, y, m - 1) == uint8_t(types::Cell::TRACE))
                                {
                                  task.m_matrix.set_at(uint8_t(types::Cell::INTERSECTION_VIA), n * 2, y, m - 1);
                                }
                            }

                          left += step;
                        }
                    }
                }
            }

          // for(const auto& [name, net] : gcell->m_nets)
          //   {
          // double mean_pin_x;
          // double mean_pin_y;

          // for(const auto& pin_name : net.m_pins)
          //   {
          //     pin::Pin*                                        pin = pins.at(pin_name);
          //     std::unordered_map<types::Metal, types::Polygon> pin_polygons;

          //     /**
          //      * 1. Find polygon centroid.
          //      * 2. Project it on polygon(non-convex case).
          //      * 3. Project new(old in convex case) centroid on closest track.
          //      * 4. If point is not iside of the polygon that add projection line to resulting path for this net.
          //      */

          //     for(const auto& [rect, metal] : pin->m_ports)
          //       {
          //         pin_polygons[metal] = utils::merge_polygons(pin_polygons[metal], rect);
          //       }
          //   }
          // }

          tasks[y].emplace_back(std::move(task));
        }
    }

  return tasks;
}

} // namespace process
