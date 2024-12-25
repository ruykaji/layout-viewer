#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <unordered_set>

#include <clipper2/clipper.h>

#include "Include/Process.hpp"
#include "Include/Utils.hpp"

namespace process::details::apply_global_routing
{

/** Support function for placement macros in gcells */
bool
is_ignore_metal(const types::Metal metal)
{
  switch(metal)
    {
    case types::Metal::L1M1_V:
    case types::Metal::M1M2_V:
    case types::Metal::M2M3_V:
    case types::Metal::M3M4_V:
    case types::Metal::M4M5_V:
    case types::Metal::M6M7_V:
    case types::Metal::M7M8_V:
    case types::Metal::M8M9_V:
    case types::Metal::NONE:
      return true;
    default:
      return false;
    }
}

void
apply_orientation(geom::Polygon& poly, types::Orientation orientation, double width, double height)
{
  if(poly.m_points.size() < 4)
    {
      throw std::runtime_error("Process Error: Wrong rectangle format.");
    }

  for(size_t i = 0; i < poly.m_points.size(); ++i)
    {
      double x = poly.m_points[i].x;
      double y = poly.m_points[i].y;

      switch(orientation)
        {
        case types::Orientation::S:
          {
            poly.m_points[i].x = width - x;
            poly.m_points[i].y = height - y;
            break;
          }
        case types::Orientation::E:
          {
            poly.m_points[i].x = height - y;
            poly.m_points[i].y = x;
            break;
          }
        case types::Orientation::W:
          {
            poly.m_points[i].x = y;
            poly.m_points[i].y = width - x;
            break;
          }
        case types::Orientation::FN:
          {
            poly.m_points[i].x = width - x;
            break;
          }
        case types::Orientation::FS:
          {
            poly.m_points[i].y = height - y;
            break;
          }
        case types::Orientation::FE:
          {
            poly.m_points[i].x = y;
            poly.m_points[i].y = x;
            break;
          }
        case types::Orientation::FW:
          {
            poly.m_points[i].x = height - y;
            poly.m_points[i].y = width - x;
            break;
          }
        default:
          break;
        }
    }
}

} // namespace process::details::global_routing

namespace process::details::encode
{

void
prepare_root_matrix(matrix::Matrix& root_matrix, const def::GCell* gcell, const def::Data& def_data)
{
  const std::size_t number_of_metals = def_data.m_tracks_x.size();

  const double      m1_offset        = def_data.m_tracks_x[0].m_start;
  const double      m1_step          = def_data.m_tracks_x[0].m_spacing;

  const double      m1_left_x        = gcell->m_tracks_x.front().m_points[0].x;
  const double      m1_right_x       = gcell->m_tracks_x.back().m_points[1].x;

  const double      m1_left_y        = gcell->m_tracks_y.front().m_points[1].y;
  const double      m1_right_y       = gcell->m_tracks_y.back().m_points[2].y;

  const std::size_t m1_max_tracks_x  = (m1_right_x - m1_left_x) / m1_step;
  const std::size_t m1_max_tracks_y  = (m1_right_y - m1_left_y) / m1_step;

  for(std::size_t m = 0; m < number_of_metals; ++m)
    {
      if(m == 0)
        {
          /** First layer always fill whole matrix as it suppose to be */
          for(std::size_t y = 0, end_y = m1_max_tracks_y * 2, end_x = m1_max_tracks_x * 2; y < end_y; y += 2)
            {
              for(std::size_t x = 0; x < end_x; ++x)
                {
                  root_matrix.set_at(uint8_t(types::Cell::TRACE), x, y, m);
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
                  int64_t n = std::floor((m1_right_x - (left - m1_offset)) / m1_step);

                  /** Add track by x */
                  for(std::size_t x = 0, end_x = end_x = m1_max_tracks_x * 2; x < end_x; ++x)
                    {
                      root_matrix.set_at(uint8_t(types::Cell::TRACE), x, n * 2, m);

                      if(root_matrix.get_at(x, n * 2, m - 1) == uint8_t(types::Cell::TRACE))
                        {
                          root_matrix.set_at(uint8_t(types::Cell::INTERSECTION_VIA), x, n * 2, m - 1);
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
                  int64_t n = std::floor((m1_right_y - (left - m1_offset)) / m1_step);

                  /** Add track by y*/
                  for(std::size_t y = 0, end_y = m1_max_tracks_y * 2; y < end_y; ++y)
                    {
                      root_matrix.set_at(uint8_t(types::Cell::TRACE), n * 2, y, m);

                      if(root_matrix.get_at(n * 2, y, m - 1) == uint8_t(types::Cell::TRACE))
                        {
                          root_matrix.set_at(uint8_t(types::Cell::INTERSECTION_VIA), n * 2, y, m - 1);
                        }
                    }

                  left += step;
                }
            }
        }
    }
}

std::tuple<uint8_t, uint8_t, uint8_t>
map_to_rgb(const std::vector<uint32_t>& values, uint32_t N)
{
  for(uint32_t value : values)
    {
      if(value < 0 || value >= N)
        {
          throw std::invalid_argument("All values must be in the range [0, N-1]");
        }
    }

  std::size_t cmb_value = 0;
  std::size_t k         = values.size();

  for(std::size_t i = 0; i < k; ++i)
    {
      cmb_value += values[i] * std::pow(N, k - i - 1);
    }

  return { ((cmb_value / (256 * 256)) % 256), ((cmb_value / 256) % 256), (cmb_value % 256) };
}

/** Support functions for track assignment */
std::pair<double, double>
calculate_centroid(const std::vector<double>& polygon)
{
  const std::size_t n    = polygon.size() / 2;

  double            area = 0.0;
  double            cx   = 0.0;
  double            cy   = 0.0;

  for(std::size_t i = 0; i < n; ++i)
    {
      const double x1     = polygon[2 * i];
      const double y1     = polygon[2 * i + 1];
      const double x2     = polygon[2 * ((i + 1) % n)];
      const double y2     = polygon[2 * ((i + 1) % n) + 1];

      const double factor = -(x1 * y2 - x2 * y1);

      area += factor;
      cx += (x1 + x2) * factor;
      cy += (y1 + y2) * factor;
    }

  area = std::abs(area) / 2.0;

  if(area == 0.0)
    {
      throw std::runtime_error("Degenerate polygon: area is zero.");
    }

  cx /= (6.0 * area);
  cy /= (6.0 * area);

  return { std::abs(cx), std::abs(cy) };
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

} // namespace process::details::encode

namespace process
{

std::pair<matrix::Matrix, std::vector<std::vector<uint8_t>>>
Task::encode_wip(const uint8_t max_layers)
{
  /** Pack all nets */
  const matrix::Matrix&             wip_matrix = m_matrices[m_wip_idx];
  const matrix::Shape               wip_shape  = m_matrices[m_wip_idx].shape();
  const uint8_t                     min_z      = m_wip_idx * max_layers;
  const uint8_t                     max_z      = min_z + wip_shape.m_z;

  std::vector<std::vector<uint8_t>> packed_terminals;

  for(std::size_t i = 0, end = m_nets.size(); i < end; ++i)
    {
      std::vector<uint8_t> terminals{ uint8_t(i), 0 };

      for(std::size_t j = 0, end = m_nets[i].size() / 3; j < end; j += 3)
        {
          if(m_nets[i][j + 2] >= min_z && m_nets[i][j + 2] < max_z)
            {
              terminals.emplace_back(m_nets[i][j]);
              terminals.emplace_back(m_nets[i][j + 1]);
              terminals.emplace_back(m_nets[i][j + 2]);
            }
        }

      if(!terminals.empty())
        {
          terminals[1] = uint8_t(terminals.size());
          packed_terminals.emplace_back(std::move(terminals));
        }
    }

  /** Encode the wip matrix */
  matrix::Matrix rgb{ { wip_shape.m_x, wip_shape.m_y, 3 } };

  for(uint8_t x = 0; x < wip_shape.m_x; ++x)
    {
      for(uint8_t y = 0; y < wip_shape.m_x; ++y)
        {
          std::vector<uint32_t> channels;

          for(uint8_t z = 0; z < wip_shape.m_z; ++z)
            {
              channels.emplace_back(wip_matrix.get_at(x, y, z));
            }

          const auto rgb_value = details::encode::map_to_rgb(channels, 100); // TODO Remove this hardcode

          rgb.set_at(std::get<0>(rgb_value), x, y, 0);
          rgb.set_at(std::get<1>(rgb_value), x, y, 1);
          rgb.set_at(std::get<2>(rgb_value), x, y, 2);
        }
    }

  return std::make_pair(std::move(rgb), std::move(packed_terminals));
}

void
Task::set_value(const std::string& name, const uint8_t value, const uint8_t x, const uint8_t y, const uint8_t z)
{
}

void
apply_global_routing(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets)
{
  using GCellsWithOverlaps = std::vector<std::pair<def::GCell*, geom::Polygon>>;

  /** Add global obstacles to gcells */
  for(auto& poly : def_data.m_obstacles)
    {
      if(details::apply_global_routing::is_ignore_metal(poly.m_metal) || poly.m_metal == types::Metal::L1)
        {
          continue;
        }

      GCellsWithOverlaps gwo = def::GCell::find_overlaps(poly, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gwo)
        {
          gcell->m_obstacles.emplace_back(overlap);
        }
    }

  // TODO: Try to replace with something less memory required data structures, or it will go away itself as i replace guide file with my own global routing
  std::unordered_map<def::GCell*, std::unordered_map<pin::Pin*, std::vector<geom::Polygon>>> gcell_to_pins;
  std::unordered_map<pin::Pin*, std::unordered_set<def::GCell*>>                             pin_to_gcells;

  for(auto& [_, pin] : def_data.m_pins)
    {
      for(const auto& port : pin->m_ports)
        {
          if(details::apply_global_routing::is_ignore_metal(port.m_metal))
            {
              continue;
            }

          GCellsWithOverlaps gwo = def::GCell::find_overlaps(port, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

          for(auto& [gcell, overlap] : gwo)
            {
              gcell_to_pins[gcell][pin].emplace_back(std::move(overlap));
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
          if(details::apply_global_routing::is_ignore_metal(obs.m_metal) || obs.m_metal == types::Metal::L1)
            {
              continue;
            }

          obs.scale_by(lef_data.m_database_number);
          details::apply_global_routing::apply_orientation(obs, component.m_orientation, width, height);
          obs.move_by({ component.m_x, component.m_y });

          GCellsWithOverlaps gwo = def::GCell::find_overlaps(obs, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

          for(auto& [gcell, overlap] : gwo)
            {
              gcell->m_obstacles.emplace_back(overlap);
            }
        }

      for(auto& [name, pin] : macro.m_pins)
        {
          pin::Pin* new_pin = new pin::Pin(pin);

          for(auto& port : new_pin->m_ports)
            {
              if(details::apply_global_routing::is_ignore_metal(port.m_metal))
                {
                  continue;
                }

              port.scale_by(lef_data.m_database_number);
              details::apply_global_routing::apply_orientation(port, component.m_orientation, width, height);
              port.move_by({ component.m_x, component.m_y });

              GCellsWithOverlaps gwo = def::GCell::find_overlaps(port, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gwo)
                {
                  gcell_to_pins[gcell][new_pin].emplace_back(std::move(overlap));
                  pin_to_gcells[new_pin].emplace(gcell);
                }
            }

          for(auto& poly : new_pin->m_obs)
            {
              if(details::apply_global_routing::is_ignore_metal(poly.m_metal))
                {
                  continue;
                }

              poly.scale_by(lef_data.m_database_number);
              details::apply_global_routing::apply_orientation(poly, component.m_orientation, width, height);
              poly.move_by({ component.m_x, component.m_y });

              GCellsWithOverlaps gwo = def::GCell::find_overlaps(poly, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gwo)
                {
                  gcell->m_obstacles.emplace_back(overlap);
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

      for(const auto& poly : guide_net.m_path)
        {
          GCellsWithOverlaps    gwo       = def::GCell::find_overlaps(poly, def_data.m_gcells, def_data.m_max_gcell_x, def_data.m_max_gcell_y);
          lef::Layer::Direction direction = lef_data.m_layers.at(poly.m_metal).m_direction;

          for(std::size_t i = 0, end = gwo.size(); i < end; ++i)
            {
              def::GCell* gcell = gwo[i].first;

              /** Add edge pins and assign them to tracks */
              if(i > 0)
                {
                  if(!gcell->m_edge_pins.count(guide_net.m_name))
                    {
                      gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(poly.m_metal, 1));
                    }
                  else
                    {
                      gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(poly.m_metal, 0));
                    }

                  def::GCell* prev_gcell = gwo[i - 1].first;

                  if(!prev_gcell->m_edge_pins.count(guide_net.m_name))
                    {
                      prev_gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(poly.m_metal, -1));
                    }
                  else
                    {
                      prev_gcell->m_edge_pins.emplace(guide_net.m_name, std::make_pair(poly.m_metal, 0));
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
                              const std::vector<geom::Polygon>& obstacles = gcell_to_pins.at(another_gcell).at(pin);

                              for(const auto& obs : obstacles)
                                {
                                  if(details::apply_global_routing::is_ignore_metal(obs.m_metal) || obs.m_metal == types::Metal::L1)
                                    {
                                      continue;
                                    }

                                  another_gcell->m_obstacles.emplace_back(obs);
                                }
                            }
                        }

                      /** Place current pin in a gcell */
                      pin->m_is_placed = true;
                      pin->m_ports     = std::move(gcell_pins.at(pin));

                      gcell->m_nets[guide_net.m_name].emplace_back(pin);
                      def_data.m_nets_gcells[guide_net.m_name].emplace_back(gcell);
                    }
                }
            }
        }
    }

  /** Remove all not used gcells and collect statistics */
  for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
    {
      auto itr = std::remove_if(def_data.m_gcells[y].begin(), def_data.m_gcells[y].end(), [](const def::GCell* ptr) {
        if(ptr->m_nets.size() == 0 && ptr->m_edge_pins.empty())
          {
            delete ptr;
            return true;
          }

        return false;
      });

      def_data.m_gcells[y].erase(itr, def_data.m_gcells[y].end());
    }

  auto itr = std::remove_if(def_data.m_gcells.begin(), def_data.m_gcells.end(), [](const auto vec) { return vec.size() == 0; });
  def_data.m_gcells.erase(itr, def_data.m_gcells.end());

  int a = 0;
}

std::vector<Task>
encode(def::Data& def_data)
{
  const uint8_t     max_nets_in_task     = 10; // TODO: Remove this hardcode
  const uint8_t     max_layers_in_matrix = 2;  // TODO: Remove this hardcode
  const uint8_t     size                 = 32; // TODO: Remove this hardcode
  const uint8_t     number_of_layers     = uint8_t(def_data.m_tracks_x.size());

  std::vector<Task> tasks;

  for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(std::size_t x = 0, end_x = def_data.m_gcells[y].size(); x < end_x; ++x)
        {
          def::GCell*    gcell = def_data.m_gcells[y][x];

          /** Preapare the root matrix */
          matrix::Matrix root_matrix{ { size * 2, size * 2, uint8_t(def_data.m_tracks_x.size()) } };
          details::encode::prepare_root_matrix(root_matrix, gcell, def_data);

          TaskBatch task_batch;
          task_batch.m_gcell = gcell;

          /** Make tasks for each net group */
          for(auto net_itr = gcell->m_nets.begin(), net_itr_end = gcell->m_nets.end(); net_itr != net_itr_end;)
            {
              Task task;

              for(uint8_t i = 0; i <= number_of_layers; i += max_layers_in_matrix)
                {
                  task.m_matrices.emplace_back(matrix::Shape{ size * 2, size * 2, std::min(max_layers_in_matrix, uint8_t(number_of_layers - i)) });
                }

              for(std::size_t i = 0; i < max_nets_in_task && net_itr != net_itr_end; ++i, ++net_itr)
                {
                  /** Track assignment */
                  const auto& [name, net] = *net_itr;

                  for(const auto& pin : net)
                    {
                      geom::Polygon pin_polygon;

                      /**
                       * 1. Find polygon centroid.
                       * 2. Project it on polygon(non-convex case).
                       * 3. Project new(old in convex case) centroid on closest track.
                       * 4. If point is not iside of the polygon that add projection line to resulting path for this net.
                       */
                      // for(const auto& [rect, metal] : pin->m_portss)
                      //   {
                      //     pin_polygon = details::encode::unionPolygons(pin_polygon, rect);
                      //   }

                      // std::pair<double, double> centroid = details::encode::calculate_centroid(pin_polygon);

                      // if(!details::encode::is_point_inside_polygon(centroid.first, centroid.second, pin_polygon))
                      //   {
                      //     centroid = details::encode::find_closest_point_in_polygon(centroid, pin_polygon);
                      //   }
                    }
                }

              task_batch.m_task.emplace_back(std::move(task));
            }
        }
    }

  return tasks;
}

} // namespace process
