#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <array>

#include "Include/DEF/DEF.hpp"
#include "Include/Graph.hpp"
#include "Include/Guide.hpp"
#include "Include/LEF.hpp"
#include "Include/Matrix.hpp"

namespace process
{

class Process
{

public:
  ~Process();

  /** Setters */
public:
  /**
   * @brief Set the path to a pdk.
   *
   * @param path A path to a pdk.
   */
  void
  set_path_pdk(const std::filesystem::path& path) noexcept(true)
  {
    m_path_pdk = path;
  }

  /**
   * @brief Set the path to a design.
   *
   * @param path A path to a design.
   */
  void
  set_path_design(const std::filesystem::path& path) noexcept(true)
  {
    m_path_design = path;
  }

  /**
   * @brief Set the path to a guide.
   *
   * @param path A path to a guide.
   */
  void
  set_path_guide(const std::filesystem::path& path) noexcept(true)
  {
    m_path_guide = path;
  }

  /**
   * @brief Set the size of a matrix.
   *
   * @param size A size of a matrix.
   */
  void
  set_matrix_size(const std::size_t size) noexcept(true)
  {
    m_matrix_size = size;
  }

  /**
   * @brief Set the step size of a matrix.
   *
   * @param size A step size of a matrix.
   */
  void
  set_matrix_step_size(const std::size_t size) noexcept(true)
  {
    m_matrix_step_size = size;
  }

  /** Getters */
public:
  /**
   * @brief Returns LEF data.
   *
   * @return def::Data&
   */
  def::Data&
  get_def_data() noexcept(true)
  {
    return m_def_data;
  }

  /**
   * @brief Returns LEF data.
   *
   * @return lef::Data&
   */
  lef::Data&
  get_lef_data() noexcept(true)
  {
    return m_lef_data;
  }

  /** Stages */
public:
  /**
   * @brief Read and parse LEF, DEf and Guide files.
   *
   */
  void
  prepare_data();

  /**
   * @brief Collect overlaps between gcells and design.
   *
   */
  void
  collect_overlaps();

  /**
   * @brief Apply the guide data.
   *
   */
  void
  apply_guide();

  /**
   * @brief Remove gcells that doesn't contains nets.
   *
   */
  void
  remove_empty_gcells();

  /**
   * @brief Trys to solve next stack for each gcell.
   *
   * @return true - END(No more stacks left).
   * @return false
   */
  bool
  solve_next_stack();

  /**
   * @brief Make training dataset.
   *
   */
  void
  make_dataset();

private:
  std::tuple<std::vector<def::Response>, bool, std::vector<std::string>, std::size_t>
  solve_nets(def::Stack& stack) const;

  // --- Edge cost functions ---
  // For horizontal moves (layer 0), the ideal is to remain in the same row as the originating terminal.
  double
  compute_edge_cost_horizontal(const matrix::Node& current,
                               const matrix::Node& neighbor,
                               const matrix::Node& source,
                               double              lambda_param,
                               double              mu_param)
  {
    // Compute “distance” from the original terminal (source) to current and neighbor.
    double d_source_current  = (current.m_y == source.m_y)
                                   ? std::abs(static_cast<int>(current.m_x) - static_cast<int>(source.m_x))
                                   : std::abs(static_cast<int>(current.m_x) - static_cast<int>(source.m_x)) + std::abs(static_cast<int>(current.m_y) - static_cast<int>(source.m_y)) + 1;

    double d_source_neighbor = (neighbor.m_y == source.m_y)
                                   ? std::abs(static_cast<int>(neighbor.m_x) - static_cast<int>(source.m_x))
                                   : std::abs(static_cast<int>(neighbor.m_x) - static_cast<int>(source.m_x)) + std::abs(static_cast<int>(neighbor.m_y) - static_cast<int>(source.m_y)) + 1;

    double detour            = d_source_neighbor - d_source_current; // ideally 1 if move is along x
    // Define cost: ideal (detour=0) yields cost 1; deviations add cost.
    double cost_distance     = 1.0 + lambda_param * detour;
    // Ideal direction: staying in the same row.
    double alignment         = (neighbor.m_y == source.m_y) ? 1.0 : 0.0;
    double cost_direction    = 1.0 + mu_param * (1.0 - alignment);
    return cost_distance * cost_direction;
  }

  // For vertical moves (layer 1), the ideal is to remain in the same column as the originating terminal.
  double
  compute_edge_cost_vertical(const matrix::Node& current,
                             const matrix::Node& neighbor,
                             const matrix::Node& source,
                             double              lambda_param,
                             double              mu_param)
  {
    double d_source_current  = (current.m_x == source.m_x)
                                   ? std::abs(static_cast<int>(current.m_y) - static_cast<int>(source.m_y))
                                   : std::abs(static_cast<int>(current.m_y) - static_cast<int>(source.m_y)) + std::abs(static_cast<int>(current.m_x) - static_cast<int>(source.m_x)) + 1;

    double d_source_neighbor = (neighbor.m_x == source.m_x)
                                   ? std::abs(static_cast<int>(neighbor.m_y) - static_cast<int>(source.m_y))
                                   : std::abs(static_cast<int>(neighbor.m_y) - static_cast<int>(source.m_y)) + std::abs(static_cast<int>(neighbor.m_x) - static_cast<int>(source.m_x)) + 1;

    double detour            = d_source_neighbor - d_source_current;
    double cost_distance     = 1.0 + lambda_param * detour;
    double alignment         = (neighbor.m_x == source.m_x) ? 1.0 : 0.0;
    double cost_direction    = 1.0 + mu_param * (1.0 - alignment);
    return cost_distance * cost_direction;
  }

  std::pair<matrix::Matrix, matrix::Matrix>
  distance_cost_map(const matrix::Matrix&     source,
                    const matrix::SetOfNodes& terminals,
                    const matrix::SetOfNodes& obs)
  {
    using namespace matrix;

    double lambda_param = 0.125;
    double mu_param     = 1.0;
    double switch_cost  = 2.0; // fixed cost for switching layers.

    // Create cost maps for horizontal (layer 0) and vertical (layer 1).
    Matrix horizontal(Shape{ source.m_shape.m_x, source.m_shape.m_y, 1 });
    Matrix vertical(Shape{ source.m_shape.m_x, source.m_shape.m_y, 1 });

    // Initialize cost maps to INF.
    double INF = std::numeric_limits<double>::max();
    
    for(uint32_t x = 0; x < source.m_shape.m_x; ++x)
      {
        for(uint32_t y = 0; y < source.m_shape.m_y; ++y)
          {
            horizontal.set_at(INF, x, y, 0);
            vertical.set_at(INF, x, y, 0);
          }
      }

    // Priority queue for Dijkstra.
    std::priority_queue<Node, std::vector<Node>, CompareNode> pq;

    // Insert all terminal nodes as sources in both layers with cost 0.
    for(const auto& T : terminals)
      {
        Node n;
        n.m_x        = T.m_x;
        n.m_y        = T.m_y;
        n.m_source_x = T.m_x;
        n.m_source_y = T.m_y;
        n.m_cost     = 0.0;

        if(T.m_z == 0)
          {
            n.m_z = 0;
            horizontal.set_at(0.0, T.m_x, T.m_y, 0);
          }
        else
          {
            n.m_z = 1;
            vertical.set_at(0.0, T.m_x, T.m_y, 0);
          }

        pq.push(n);
      }

    // Multi-source Dijkstra.
    while(!pq.empty())
      {
        Node current = pq.top();
        pq.pop();

        // Check if the popped cost is outdated.
        if(current.m_z == 0)
          {
            if(current.m_cost > horizontal.get_at(current.m_x, current.m_y, 0))
              continue;
          }
        else
          {
            if(current.m_cost > vertical.get_at(current.m_x, current.m_y, 0))
              continue;
          }

        // Expand neighbors based on current layer.
        if(current.m_z == 0)
          { // horizontal layer: allowed moves left/right.
            // Move left.
            if(current.m_x > 0)
              {
                Node neighbor;
                neighbor.m_x        = current.m_x - 1;
                neighbor.m_y        = current.m_y;
                neighbor.m_z        = 0;
                neighbor.m_source_x = current.m_source_x;
                neighbor.m_source_y = current.m_source_y;

                if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
                  {
                    double edge_cost = compute_edge_cost_horizontal(
                        current, neighbor,
                        Node{ current.m_source_x, current.m_source_y, 0, 0.0, 0, 0 },
                        lambda_param, mu_param);
                    double new_cost  = current.m_cost + edge_cost;
                    double prev_cost = horizontal.get_at(neighbor.m_x, neighbor.m_y, 0);
                    if(new_cost < prev_cost)
                      {
                        horizontal.set_at(new_cost, neighbor.m_x, neighbor.m_y, 0);
                        neighbor.m_cost = new_cost;
                        pq.push(neighbor);
                      }
                  }
              }
            // Move right.
            if(current.m_x < source.m_shape.m_x - 1)
              {
                Node neighbor;
                neighbor.m_x        = current.m_x + 1;
                neighbor.m_y        = current.m_y;
                neighbor.m_z        = 0;
                neighbor.m_source_x = current.m_source_x;
                neighbor.m_source_y = current.m_source_y;

                if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
                  {
                    double edge_cost = compute_edge_cost_horizontal(
                        current, neighbor,
                        Node{ current.m_source_x, current.m_source_y, 0, 0.0, 0, 0 },
                        lambda_param, mu_param);
                    double new_cost  = current.m_cost + edge_cost;
                    double prev_cost = horizontal.get_at(neighbor.m_x, neighbor.m_y, 0);
                    if(new_cost < prev_cost)
                      {
                        horizontal.set_at(new_cost, neighbor.m_x, neighbor.m_y, 0);
                        neighbor.m_cost = new_cost;
                        pq.push(neighbor);
                      }
                  }
              }
          }
        else
          { // vertical layer (m_z == 1): allowed moves up/down.
            // Move up.
            if(current.m_y > 0)
              {
                Node neighbor;
                neighbor.m_x        = current.m_x;
                neighbor.m_y        = current.m_y - 1;
                neighbor.m_z        = 1;
                neighbor.m_source_x = current.m_source_x;
                neighbor.m_source_y = current.m_source_y;

                if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
                  {
                    double edge_cost = compute_edge_cost_vertical(
                        current, neighbor,
                        Node{ current.m_source_x, current.m_source_y, 0, 0.0, 0, 0 },
                        lambda_param, mu_param);
                    double new_cost  = current.m_cost + edge_cost;
                    double prev_cost = vertical.get_at(neighbor.m_x, neighbor.m_y, 0);
                    if(new_cost < prev_cost)
                      {
                        vertical.set_at(new_cost, neighbor.m_x, neighbor.m_y, 0);
                        neighbor.m_cost = new_cost;
                        pq.push(neighbor);
                      }
                  }
              }
            // Move down.
            if(current.m_y < source.m_shape.m_y - 1)
              {
                Node neighbor;
                neighbor.m_x        = current.m_x;
                neighbor.m_y        = current.m_y + 1;
                neighbor.m_z        = 1;
                neighbor.m_source_x = current.m_source_x;
                neighbor.m_source_y = current.m_source_y;

                if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
                  {
                    double edge_cost = compute_edge_cost_vertical(
                        current, neighbor,
                        Node{ current.m_source_x, current.m_source_y, 0, 0.0, 0, 0 },
                        lambda_param, mu_param);

                    double new_cost  = current.m_cost + edge_cost;
                    double prev_cost = vertical.get_at(neighbor.m_x, neighbor.m_y, 0);

                    if(new_cost < prev_cost)
                      {
                        vertical.set_at(new_cost, neighbor.m_x, neighbor.m_y, 0);
                        neighbor.m_cost = new_cost;
                        pq.push(neighbor);
                      }
                  }
              }
          }

        // Allow layer switching at intersections (where both x and y are even).

        Node neighbor;
        neighbor.m_x        = current.m_x;
        neighbor.m_y        = current.m_y;
        neighbor.m_source_x = current.m_source_x;
        neighbor.m_source_y = current.m_source_y;
        neighbor.m_cost     = current.m_cost + switch_cost;

        if(current.m_z == 0)
          {
            neighbor.m_z     = 1;
            double prev_cost = vertical.get_at(neighbor.m_x, neighbor.m_y, 0);

            if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
              {
                if(neighbor.m_cost < prev_cost)
                  {
                    vertical.set_at(neighbor.m_cost, neighbor.m_x, neighbor.m_y, 0);
                    pq.push(neighbor);
                  }
              }
          }
        else
          {
            neighbor.m_z     = 0;
            double prev_cost = horizontal.get_at(neighbor.m_x, neighbor.m_y, 0);

            if(source.get_at(neighbor.m_x, neighbor.m_y, neighbor.m_z) != 0 && obs.count(neighbor) == 0)
              {
                if(neighbor.m_cost < prev_cost)
                  {
                    horizontal.set_at(neighbor.m_cost, neighbor.m_x, neighbor.m_y, 0);
                    pq.push(neighbor);
                  }
              }
          }

      } // end while

    // --- Invert and scale the cost maps ---
    // Here lower cost (closer to 0) means a higher probability.
    // We scale each map so that: normalized = 1.0 - (cost - min_cost)/(max_cost - min_cost).
    double max = -std::numeric_limits<double>::max();
    double min = INF;

    for(uint32_t x = 0; x < source.m_shape.m_x; ++x)
      {
        for(uint32_t y = 0; y < source.m_shape.m_y; ++y)
          {
            double cost_h = horizontal.get_at(x, y, 0);

            if(cost_h != INF)
              {
                if(cost_h < min)
                  min = cost_h;

                if(cost_h > max)
                  max = cost_h;
              }
            else
              {
                horizontal.set_at(-1.0, x, y, 0);
              }

            double cost_v = vertical.get_at(x, y, 0);

            if(cost_v != INF)
              {
                if(cost_v < min)
                  min = cost_v;

                if(cost_v > max)
                  max = cost_v;
              }
            else
              {
                vertical.set_at(-1.0, x, y, 0);
              }
          }
      }

    // Normalize (if max == min, set to 1.0 everywhere).
    for(uint32_t x = 0; x < source.m_shape.m_x; ++x)
      {
        for(uint32_t y = 0; y < source.m_shape.m_y; ++y)
          {
            double cost_h = horizontal.get_at(x, y, 0);

            if(cost_h != -1.0)
              {
                double norm_h = (max > min) ? 0.9 * (1.0 - ((cost_h - min) / (max - min))) : 0.9;
                horizontal.set_at(norm_h, x, y, 0);
              }
            else
              {
                horizontal.set_at(0.0, x, y, 0);
              }

            double cost_v = vertical.get_at(x, y, 0);

            if(cost_v != -1.0)
              {
                double norm_v = (max > min) ? 0.9 * (1.0 - ((cost_v - min) / (max - min))) : 0.9;
                vertical.set_at(norm_v, x, y, 0);
              }
            else
              {
                vertical.set_at(0.0, x, y, 0);
              }
          }
      }

    for(const auto& T : terminals)
      {
        if(T.m_z == 0)
          {
            horizontal.set_at(1.0, T.m_x, T.m_y, 0);
          }
        else
          {
            vertical.set_at(1.0, T.m_x, T.m_y, 0);
          }
      }

    return std::make_pair(std::move(horizontal), std::move(vertical));
  }

private:
  /** Project settings */
  std::filesystem::path                                                         m_path_pdk;         ///> A Path to a pdk.
  std::filesystem::path                                                         m_path_design;      ///> A path to a design.
  std::filesystem::path                                                         m_path_guide;       ///> A path to a guide file.
  std::size_t                                                                   m_matrix_size;      ///> The size of a matrix.
  std::size_t                                                                   m_matrix_step_size; ///> The step size of a matrix.

  /** Work data */
  lef::Data                                                                     m_lef_data;        ///> Lef data.
  def::Data                                                                     m_def_data;        ///> Def data.
  std::vector<guide::Tree>                                                      m_guide;           ///> Guide data.
  std::unordered_map<def::GCell*, std::unordered_map<pin::Pin*, geom::Polygon>> m_gcell_to_pins;   ///> Maps gcell to collided pins.
  std::unordered_map<pin::Pin*, std::unordered_set<def::GCell*>>                m_pin_to_gcells;   ///> Maps pins to collided gcells.
  std::unordered_map<std::string, def::GCell*>                                  m_gcells_by_names; ///> All gcells that left after guide was applied.
};

void
work_loop(def::Data& data);

} // namespace process

#endif