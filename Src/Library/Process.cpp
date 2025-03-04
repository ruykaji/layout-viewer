#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <unordered_set>

#include <clipper2/clipper.h>

#include "Include/Algorithms.hpp"
#include "Include/GlobalUtils.hpp"
#include "Include/Numpy.hpp"
#include "Include/Process.hpp"

namespace process::details
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

namespace process
{

Process::~Process()
{
  guide::cleanup(m_guide);

  for(std::size_t y = 0, end_y = m_def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(std::size_t x = 0, end_x = m_def_data.m_gcells[y].size(); x < end_x; ++x)
        {
          delete m_def_data.m_gcells[y][x];
        }
    }

  for(auto [_, pin] : m_def_data.m_pins)
    {
      delete pin;
    }

  for(auto pin : m_def_data.m_cross_pins)
    {
      delete pin;
    }

  for(auto [_, net] : m_def_data.m_nets)
    {
      delete net;
    }
}

void
Process::prepare_data()
{
  const lef::LEF lef;
  const def::DEF def;

  m_lef_data = lef.parse(m_path_pdk);
  m_def_data = def.parse(m_path_design);
  m_guide    = guide::read(m_path_guide);
}

void
Process::collect_overlaps()
{
  /** GCells with overlaps */
  using GWO = std::vector<std::pair<def::GCell*, geom::Polygon>>;

  for(auto& poly : m_def_data.m_obstacles)
    {
      if(details::is_ignore_metal(poly.m_metal) || poly.m_metal == types::Metal::L1)
        {
          continue;
        }

      GWO gwo = def::GCell::find_overlaps(poly, m_def_data.m_gcells, m_def_data.m_max_gcell_x, m_def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gwo)
        {
          gcell->m_obstacles.emplace_back(overlap);
        }
    }

  for(auto& [_, pin] : m_def_data.m_pins)
    {
      geom::Polygon port = pin->m_ports.at(0);

      if(details::is_ignore_metal(port.m_metal))
        {
          continue;
        }

      if(port.m_points[1].x >= m_def_data.m_max_gcell_x)
        {
          port.m_points[1].x = m_def_data.m_gcells.back().back()->m_box.m_points[0].x * .999;
          port.m_points[2].x = m_def_data.m_gcells.back().back()->m_box.m_points[0].x * .999;
        }

      if(port.m_points[2].y >= m_def_data.m_max_gcell_y)
        {
          port.m_points[2].y = m_def_data.m_gcells.back().back()->m_box.m_points[0].y * .999;
          port.m_points[3].y = m_def_data.m_gcells.back().back()->m_box.m_points[0].y * .999;
        }

      GWO gwo = def::GCell::find_overlaps(port, m_def_data.m_gcells, m_def_data.m_max_gcell_x, m_def_data.m_max_gcell_y);

      for(auto& [gcell, overlap] : gwo)
        {
          m_gcell_to_pins[gcell][pin] = std::move(overlap);
          m_pin_to_gcells[pin].emplace(gcell);
        }
    }

  for(auto& component : m_def_data.m_components)
    {
      if(m_lef_data.m_macros.count(component.m_name) == 0)
        {
          throw std::runtime_error("Process Error: Couldn't find a macro with the name - \"" + component.m_name + "\".");
        }

      lef::Macro   macro  = m_lef_data.m_macros.at(component.m_name);
      const double width  = macro.m_width * m_lef_data.m_database_number;
      const double height = macro.m_height * m_lef_data.m_database_number;

      for(auto& obs : macro.m_obs)
        {
          if(details::is_ignore_metal(obs.m_metal) || obs.m_metal == types::Metal::L1)
            {
              continue;
            }

          obs.scale_by(m_lef_data.m_database_number);
          details::apply_orientation(obs, component.m_orientation, width, height);
          obs.move_by({ component.m_x, component.m_y });

          GWO gwo = def::GCell::find_overlaps(obs, m_def_data.m_gcells, m_def_data.m_max_gcell_x, m_def_data.m_max_gcell_y);

          for(auto& [gcell, overlap] : gwo)
            {
              gcell->m_obstacles.emplace_back(overlap);
            }
        }

      for(auto& [name, pin] : macro.m_pins)
        {
          pin::Pin*     new_pin = new pin::Pin(pin);
          geom::Polygon port    = new_pin->m_ports.at(0);

          if(details::is_ignore_metal(port.m_metal))
            {
              continue;
            }

          port.scale_by(m_lef_data.m_database_number);
          details::apply_orientation(port, component.m_orientation, width, height);
          port.move_by({ component.m_x, component.m_y });

          GWO gwo = def::GCell::find_overlaps(port, m_def_data.m_gcells, m_def_data.m_max_gcell_x, m_def_data.m_max_gcell_y);

          for(auto& [gcell, overlap] : gwo)
            {
              m_gcell_to_pins[gcell][new_pin] = std::move(overlap);
              m_pin_to_gcells[new_pin].emplace(gcell);
            }

          for(auto& poly : new_pin->m_obs)
            {
              if(details::is_ignore_metal(poly.m_metal))
                {
                  continue;
                }

              poly.scale_by(m_lef_data.m_database_number);
              details::apply_orientation(poly, component.m_orientation, width, height);
              poly.move_by({ component.m_x, component.m_y });

              GWO gwo = def::GCell::find_overlaps(poly, m_def_data.m_gcells, m_def_data.m_max_gcell_x, m_def_data.m_max_gcell_y);

              for(auto& [gcell, overlap] : gwo)
                {
                  gcell->m_obstacles.emplace_back(overlap);
                }
            }

          new_pin->m_name                    = component.m_id + ":" + name;
          m_def_data.m_pins[new_pin->m_name] = new_pin;
        }
    }
}

void
Process::apply_guide()
{
  /** Apply global routing to gcells and pins from a guide file */
  for(const auto& net : m_guide)
    {
      if(m_def_data.m_nets.count(net.m_name) == 0)
        {
          std::cout << "Process Warning: Couldn't find a net with the name - \"" << net.m_name << "\"." << std::endl;
          continue;
        }

      def::Net*                                               current_net = m_def_data.m_nets.at(net.m_name);

      std::unordered_set<def::GCell*>                         leaf_nodes;
      std::unordered_set<def::GCell*>                         inner_nodes;
      std::unordered_map<def::GCell*, std::vector<pin::Pin*>> all_pins;

      for(auto node : net.m_nodes)
        {
          def::GCell* gcell = m_def_data.m_gcells[node->m_y][node->m_x];

          if(node->m_connections == 1)
            {
              leaf_nodes.emplace(gcell);
            }
          else
            {
              inner_nodes.emplace(gcell);
            }

          all_pins[gcell] = {};
        }

      std::unordered_set<pin::Pin*> pins_set;

      for(const auto& name : current_net->m_pins)
        {
          pins_set.emplace(m_def_data.m_pins.at(name));
        }

      for(auto itr = leaf_nodes.cbegin(), end = leaf_nodes.cend(); itr != end;)
        {
          def::GCell* gcell = (*itr);

          if(m_gcell_to_pins.count(gcell) == 0)
            {
              itr = leaf_nodes.erase(itr);
              std::cout << "Apply guide Warning: Unable to find any pin for gcell - GCell_x_" << gcell->m_x << "_y_" << gcell->m_y << ". GCell will be removed. GCell will be removed from the NET: " << current_net->m_name << std::endl;
              continue;
            }

          const auto& gcell_pins        = m_gcell_to_pins.at(gcell);

          pin::Pin*   accessor          = nullptr;
          double      accessor_max_area = 0.0;

          for(auto& [pin, overlap] : gcell_pins)
            {
              if(pins_set.count(pin) != 0 && !pin->m_is_placed)
                {
                  double area = overlap.get_area();

                  if(area > accessor_max_area)
                    {
                      accessor          = pin;
                      accessor_max_area = area;
                    }
                }
            }

          if(accessor == nullptr)
            {
              itr = leaf_nodes.erase(itr);
              std::cout << "Apply guide Warning: Unable to find any pin for leaf node - GCell_x_" << gcell->m_x << "_y_" << gcell->m_y << " in NET: " << current_net->m_name << ". GCell will be removed from the NET." << std::endl;
              continue;
            }

          accessor->m_is_placed = true;
          accessor->m_ports.clear();
          accessor->m_ports.emplace_back(gcell_pins.at(accessor));

          all_pins[gcell].emplace_back(accessor);
          ++itr;
        }

      for(const auto& name : current_net->m_pins)
        {
          pin::Pin* pin = m_def_data.m_pins.at(name);

          if(pin->m_is_placed)
            {
              continue;
            }

          auto&       pin_gcells        = m_pin_to_gcells.at(pin);

          def::GCell* accessor          = nullptr;
          double      accessor_max_area = 0.0;

          for(auto& gcell : pin_gcells)
            {
              const geom::Polygon overlap = m_gcell_to_pins.at(gcell).at(pin);
              double              area    = overlap.get_area();

              if(area > accessor_max_area)
                {
                  accessor          = gcell;
                  accessor_max_area = area;
                }
            }

          if(accessor == nullptr)
            {
              throw std::runtime_error("Apply guide Error: Unable to find any gcell for a pin - " + pin->m_name);
            }

          pin->m_is_placed = true;
          pin->m_ports.clear();
          pin->m_ports.emplace_back(m_gcell_to_pins.at(accessor).at(pin));

          all_pins[accessor].emplace_back(pin);
        }

      for(const auto& edge : net.m_edges)
        {
          guide::Node* source     = edge.m_source;
          guide::Node* dest       = edge.m_destination;

          def::GCell*  gcell      = m_def_data.m_gcells[source->m_y][source->m_x];
          def::GCell*  next_gcell = m_def_data.m_gcells[dest->m_y][dest->m_x];

          if(!(leaf_nodes.count(gcell) != 0 || inner_nodes.count(gcell) != 0) || !(leaf_nodes.count(next_gcell) != 0 || inner_nodes.count(next_gcell) != 0))
            {
              continue;
            }

          if(!((gcell->m_x == next_gcell->m_x && gcell->m_y < next_gcell->m_y) || (gcell->m_x < next_gcell->m_x && gcell->m_y == next_gcell->m_y)))
            {
              std::swap(gcell, next_gcell);
            }

          pin::Pin*         cross_pin = new pin::Pin();

          const std::size_t metal_idx = (uint8_t(edge.m_metal_layer) - 1) / 2 - 1;

          geom::Point       start     = { 0.0, 0.0 };
          geom::Point       end       = { 0.0, 0.0 };

          if(metal_idx % 2 == 0)
            {
              start.x = gcell->m_box.m_points[0].x;
              end.x   = gcell->m_box.m_points[0].x;

              start.y = gcell->m_box.m_points[2].y;
              end.y   = gcell->m_box.m_points[0].y;
            }
          else
            {
              start.x = gcell->m_box.m_points[1].x;
              end.x   = gcell->m_box.m_points[0].x;

              start.y = gcell->m_box.m_points[0].y;
              end.y   = gcell->m_box.m_points[0].y;
            }

          cross_pin->m_ports.emplace_back(geom::Polygon{ { start.x, start.y, end.x, end.y }, edge.m_metal_layer });

          all_pins[gcell].emplace_back(cross_pin);
          all_pins[next_gcell].emplace_back(cross_pin);

          def::GCell::connect(gcell, next_gcell, edge.m_metal_layer);

          m_def_data.m_cross_pins.emplace_back(cross_pin);
        }

      for(auto node : net.m_nodes)
        {
          def::GCell* gcell = m_def_data.m_gcells[node->m_y][node->m_x];

          if(all_pins.count(gcell) != 0 && all_pins[gcell].size() > 1)
            {
              gcell->add_net(all_pins.at(gcell), current_net);
            }
        }
    }
}

void
Process::remove_empty_gcells()
{
  /** Remove unused gcells */
  for(std::size_t y = 0, end_y = m_def_data.m_gcells.size(); y < end_y; ++y)
    {
      auto itr = std::remove_if(m_def_data.m_gcells[y].begin(), m_def_data.m_gcells[y].end(), [this](def::GCell* gcell) {
        if((gcell->m_cross_pins.empty() && gcell->m_inner_pins.empty()))
          {
            delete gcell;
            return true;
          }

        const std::string name  = "GCell_x_" + std::to_string(gcell->m_x) + "_y_" + std::to_string(gcell->m_y);
        m_gcells_by_names[name] = gcell;

        return false;
      });

      m_def_data.m_gcells[y].erase(itr, m_def_data.m_gcells[y].end());
    }

  /** Setup obstacles and inner pins */
  for(std::size_t y = 0, end_y = m_def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(auto gcell : m_def_data.m_gcells[y])
        {
          gcell->setup_global_obstacles();
          gcell->setup_inner_pins();
        }
    }

  /** Setup cross, between stack pins and stacks itself  */
  for(std::size_t y = 0, end_y = m_def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(auto gcell : m_def_data.m_gcells[y])
        {
          gcell->setup_cross_pins();
          gcell->setup_between_stack_pins();
          gcell->setup_stacks();
        }
    }

  auto itr = std::remove_if(m_def_data.m_gcells.begin(), m_def_data.m_gcells.end(), [](const auto vec) { return vec.size() == 0; });
  m_def_data.m_gcells.erase(itr, m_def_data.m_gcells.end());
}

void
Process::make_dataset()
{
  const std::size_t           size              = 32;
  const std::size_t           step              = 2;
  const std::size_t           max_net_per_stack = 50;

  /** Preapare folders */
  const std::string           design_name       = m_path_design.filename().replace_extension("").string() + "_max";
  const std::filesystem::path root_folder       = std::filesystem::current_path() / design_name / "gcells";

  if(std::filesystem::exists(root_folder))
    {
      std::filesystem::remove_all(root_folder);
    }

  const std::filesystem::path csv_file_path = root_folder / "data.csv";
  const std::filesystem::path source_folder = root_folder / "source";
  const std::filesystem::path target_folder = root_folder / "target";

  std::filesystem::create_directories(source_folder);
  std::filesystem::create_directories(target_folder);

  std::ofstream csv_file(csv_file_path);
  csv_file << "source_h,source_v,net,target_path,pins_count,nets_count" << std::endl;

  for(auto [name, gcell] : m_gcells_by_names)
    {
      if(gcell->m_is_error)
        {
          continue;
        }

      std::size_t counter = 0;

      while(true)
        {
          auto [stack, is_last] = gcell->get_next_stack();

          if(!stack.is_empty())
            {
              const auto all_nets       = stack.m_nets;
              auto       left_nets_itr  = all_nets.begin();
              auto       right_nets_itr = all_nets.begin();

              /** Create task by steps */
              for(std::size_t i = 0, end = all_nets.size(); i < end; i += max_net_per_stack)
                {
                  if(i != 0)
                    {
                      std::advance(left_nets_itr, std::min(max_net_per_stack, end - i));
                    }

                  std::advance(right_nets_itr, std::min(max_net_per_stack, end - i));

                  const std::string save_name = name + "_stack_" + std::to_string(counter + 1) + "_" + std::to_string(i + 1);

                  stack.m_terminals.clear();
                  stack.m_nets.clear();

                  stack.m_nets.insert(left_nets_itr, right_nets_itr);

                  for(const auto& [_, local_net] : stack.m_nets)
                    {
                      for(const auto& terminal : local_net.m_terminals)
                        {
                          stack.m_terminals.insert(terminal);
                        }
                    }

                  stack.m_node_map.clear();
                  stack.m_nodes.clear();
                  stack.m_graph.get_adj().clear();

                  stack.create_matrix(size, step);
                  stack.create_graph();

                  if(stack.m_graph.get_adj().empty())
                    {
                      if(is_last)
                        {
                          break;
                        }

                      continue;
                    }

                  /** Setup dirs */
                  std::filesystem::create_directories(source_folder / save_name);
                  std::filesystem::create_directories(target_folder / save_name);

                  const auto [responses, is_any_solved, errors, iterations] = solve_nets(stack);

                  for(const auto& message : errors)
                    {
                      std::cout << "Error: " << save_name << " - " << message << std::endl;
                    }

                  if(!is_any_solved)
                    {
                      continue;
                    }

                  std::ofstream  nets_file(source_folder / save_name / (std::to_string(i + 1) + "_nets.txt"));

                  matrix::Matrix path{ { stack.m_matrix.m_shape.m_x, stack.m_matrix.m_shape.m_y, 1 } };
                  matrix::Matrix distance_matrix_h{ { stack.m_matrix.m_shape.m_x, stack.m_matrix.m_shape.m_y, responses.size() } };
                  matrix::Matrix distance_matrix_v{ { stack.m_matrix.m_shape.m_x, stack.m_matrix.m_shape.m_y, responses.size() } };

                  std::size_t    pins_counter = 0;
                  std::size_t    nets_counter = 0;

                  for(std::size_t j = 0, end_j = responses.size(); j < end_j; ++j)
                    {
                      const auto res              = responses.at(j);
                      const auto [net, local_net] = *std::find_if(stack.m_nets.begin(), stack.m_nets.end(), [res](const auto& pair) { return res.m_ptr->m_name == pair.first->m_name; });

                      pins_counter += local_net.m_terminals.size();
                      nets_counter += 1;

                      const auto [h_matrix, v_matrix] = distance_cost_map(stack.m_matrix, local_net.m_terminals, stack.m_terminals);

                      for(std::size_t y = 0; y < stack.m_matrix.m_shape.m_y; ++y)
                        {
                          for(std::size_t x = 0; x < stack.m_matrix.m_shape.m_x; ++x)
                            {
                              for(std::size_t z = 0; z < 2; ++z)
                                {
                                  distance_matrix_h.set_at(h_matrix.get_at(x, y, 0), x, y, j);
                                  distance_matrix_v.set_at(v_matrix.get_at(x, y, 0), x, y, j);
                                }
                            }
                        }

                      for(const auto& line : res.m_paths)
                        {
                          const std::size_t metal_idx = (uint8_t(line.m_metal) - 1) / 2 - 1;

                          if(line.m_start.x == line.m_end.x)
                            {
                              for(std::size_t y = line.m_start.y; y <= line.m_end.y; ++y)
                                {
                                  if(path.get_at(line.m_start.x, y, 0) == 0)
                                    {
                                      path.set_at(1, line.m_start.x, y, 0);
                                    }
                                  else
                                    {
                                      path.set_at(1, line.m_start.x, y, 0);
                                    }
                                }
                            }
                        }

                      for(const auto& line : res.m_paths)
                        {
                          const std::size_t metal_idx = (uint8_t(line.m_metal) - 1) / 2 - 1;

                          if(line.m_start.y == line.m_end.y)
                            {
                              for(std::size_t x = line.m_start.x; x <= line.m_end.x; ++x)
                                {
                                  if(path.get_at(x, line.m_start.y, 0) == 0)
                                    {
                                      path.set_at(1, x, line.m_start.y, 0);
                                    }
                                  else
                                    {
                                      path.set_at(1, x, line.m_start.y, 0);
                                    }
                                }
                            }
                        }

                      for(const auto& via : res.m_inner_via)
                        {
                          path.set_at(2, via.x, via.y, 0);
                          path.set_at(2, via.x, via.y, 0);
                        }

                      {
                        nets_file << "BEGIN" << std::endl;
                        nets_file << res.m_ptr->m_name << std::endl;

                        for(auto& pin : local_net.m_terminals)
                          {
                            nets_file << int32_t(pin.m_x) << ", " << int32_t(pin.m_y) << ", " << int32_t(pin.m_z) << std::endl;
                          }

                        nets_file << "END" << std::endl;
                      }
                    }

                  nets_file << "\n"
                            << pins_counter << "\n"
                            << nets_counter << std::endl;

                  nets_file.close();

                  numpy::save_as<double>(source_folder / save_name / (std::to_string(i + 1) + "_h.npy"), distance_matrix_h.data(), { stack.m_matrix.m_shape.m_y, stack.m_matrix.m_shape.m_x, responses.size() });
                  numpy::save_as<double>(source_folder / save_name / (std::to_string(i + 1) + "_v.npy"), distance_matrix_v.data(), { stack.m_matrix.m_shape.m_y, stack.m_matrix.m_shape.m_x, responses.size() });
                  numpy::save_as<double>(target_folder / save_name / (std::to_string(i + 1) + "_path.npy"), path.data(), { stack.m_matrix.m_shape.m_y, stack.m_matrix.m_shape.m_x, 1 });

                  csv_file << (source_folder / save_name / (std::to_string(i + 1) + "_h.npy")).string() << ","
                           << (source_folder / save_name / (std::to_string(i + 1) + "_v.npy")).string() << ","
                           << (source_folder / save_name / (std::to_string(i + 1) + "_nets.txt")).string() << ","
                           << (target_folder / save_name / (std::to_string(i + 1) + "_path.npy")).string() << ","
                           << pins_counter << ","
                           << nets_counter
                           << std::endl;
                }
            }

          if(is_last)
            {
              break;
            }

          ++counter;
        }
    }

  csv_file.close();
}

std::tuple<std::vector<def::Response>, bool, std::vector<std::string>, std::size_t>
Process::solve_nets(def::Stack& stack) const
{
  const std::size_t                                      max_iterations = 10;
  std::size_t                                            itr            = 0;

  std::vector<std::reference_wrapper<def::details::Net>> local_nets;
  std::vector<std::size_t>                               nets_idx;

  for(auto& [net, local_net] : stack.m_nets)
    {
      nets_idx.emplace_back(local_nets.size());
      local_nets.push_back(local_net);
    }

  do
    {
      algorithms::AStar          a_start(stack.m_graph, stack.m_terminals, stack.m_nodes);

      std::vector<def::Net*>     failed_nets;
      std::vector<std::string>   failed_messages;

      std::vector<def::Response> responses;

      for(const auto& idx : nets_idx)
        {
          const auto&                  local_net = local_nets.at(idx).get();

          std::unordered_set<uint32_t> local_terminals;
          bool                         is_blocked_terminal = false;

          for(const auto& terminal : local_net.m_terminals)
            {
              if(stack.m_node_map.count(terminal) == 0)
                {
                  is_blocked_terminal = true;
                  break;
                }

              local_terminals.emplace(stack.m_node_map[terminal]);
            }

          if(is_blocked_terminal)
            {
              failed_messages.emplace_back("Unable to find graph node associated with pin in net - " + local_net.m_ptr->m_name);
              failed_nets.emplace_back(local_net.m_ptr);
              continue;
            }

          const std::vector<graph::Edge> mst = a_start.multi_terminal_path(local_terminals);
          def::Response                  res = { local_net.m_ptr, mst.empty(), "" };

          if(res.m_is_failed)
            {
              failed_messages.emplace_back("Unable to solve net - " + local_net.m_ptr->m_name);
              failed_nets.emplace_back(local_net.m_ptr);
              continue;
            }
          else
            {
              for(const auto& edge : mst)
                {
                  const auto&  source      = stack.m_nodes[edge.m_source];
                  const auto&  destination = stack.m_nodes[edge.m_destination];

                  uint8_t      s_x         = std::min(source.m_x, destination.m_x);
                  uint8_t      s_y         = std::min(source.m_y, destination.m_y);
                  uint8_t      s_z         = std::min(source.m_z, destination.m_z);

                  uint8_t      d_x         = std::max(source.m_x, destination.m_x);
                  uint8_t      d_y         = std::max(source.m_y, destination.m_y);
                  uint8_t      d_z         = std::max(source.m_z, destination.m_z);

                  geom::PointS start_point = { s_x, s_y };
                  geom::PointS end_point   = { d_x, d_y };

                  if(d_z == stack.m_matrix.m_shape.m_z)
                    {
                      res.m_outer_via.emplace_back(end_point);
                    }
                  else if(s_z != d_z)
                    {
                      res.m_inner_via.emplace_back(start_point);
                    }

                  if(s_z == d_z)
                    {
                      const types::Metal metal_layer = types::Metal((s_z + 1) * 2 + 1);
                      res.m_paths.emplace_back(start_point, end_point, metal_layer);
                    }
                }
            }

          responses.emplace_back(std::move(res));
        }

      if(failed_nets.empty())
        {
          return { std::move(responses), true, {}, itr };
        }

      if(++itr == max_iterations || !std::next_permutation(nets_idx.begin(), nets_idx.end()))
        {
          bool is_all_failed = failed_nets.size() != stack.m_nets.size();

          for(const auto& net : failed_nets)
            {
              stack.m_nets.erase(net);
            }

          return { std::move(responses), is_all_failed, failed_messages, itr };
        }
    }
  while(true);

  return {};
}

} // namespace process