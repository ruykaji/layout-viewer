#ifndef __ACCESS_POINT_GRID_HPP__
#define __ACCESS_POINT_GRID_HPP__

#include <iostream>
#include <unordered_set>

#include "Include/DEF/Pin.hpp"

namespace def::details
{

struct AccessNode
{
  enum class Status
  {
    FREE = 0,    ///> No pin claimed a node.
    OCCUPIED,    ///> Some pin claimed a node.
    VIA_BLOCKAGE ///> Pins may be allowed in this node only if no other place is found.
  } m_status
      = Status::FREE;

  Pin*                             m_ptr = nullptr;  ///> Pointer to the def pin.
  std::unordered_set<types::Metal> m_blocked_metals; ///> Set of metal layers that occupied on this node.
};

struct AccessLine
{
  std::size_t             m_assigned = 0; ///> Number of pins assigned to a line.
  std::vector<AccessNode> m_pins;         ///> Array of pins.
  AccessNode              m_left_pin;     ///> Left side pin.
  AccessNode              m_right_pin;    ///> Right side pin.
};

} // namespace def::details

namespace def
{

class AccessPointGrid
{
public:
  /**
   * @brief Construct a new Access Point Grid.
   *
   * @param start The start of a grid.
   * @param end The end of a grid.
   * @param step The step of a grid
   */
  AccessPointGrid(const geom::Point& start, const geom::Point& end, const double step)
      : m_start(start), m_end(end), m_step(step), m_cost(0.0)
  {
    const std::size_t size_x = (end.x - start.x) / step + 1;
    const std::size_t size_y = (end.y - start.y) / step + 1;

    m_h_lines.resize(size_y);
    m_v_lines.resize(size_x);

    for(auto& line : m_h_lines)
      {
        line.m_pins.resize(size_x);
        line.m_pins.front().m_status = details::AccessNode::Status::VIA_BLOCKAGE;
        line.m_pins.back().m_status  = details::AccessNode::Status::VIA_BLOCKAGE;
      }

    for(auto& line : m_v_lines)
      {
        line.m_pins.resize(size_y);
        line.m_pins.front().m_status = details::AccessNode::Status::VIA_BLOCKAGE;
        line.m_pins.back().m_status  = details::AccessNode::Status::VIA_BLOCKAGE;
      }
  };

  ~AccessPointGrid()
  {
    if(m_left != nullptr)
      {
        m_left->m_right = nullptr;
      }

    if(m_right != nullptr)
      {
        m_right->m_left = nullptr;
      }

    if(m_top != nullptr)
      {
        m_top->m_bottom = nullptr;
      }

    if(m_bottom != nullptr)
      {
        m_bottom->m_top = nullptr;
      }
  }

public:
  /**
   * @brief Add an obstacle on a grid.
   *
   * @param poly An obstacle to place.
   */
  void
  add_obstacle(const geom::Polygon& poly, bool is_via_blockage = false)
  {
    const types::Metal metal            = poly.m_metal;
    const std::size_t  metal_idx        = (uint8_t(metal) - 1) / 2 - 1;
    const auto [left_top, right_bottom] = poly.get_extrem_points();

    const geom::Point left_top_proj     = project_coordinate(left_top);
    const geom::Point right_bottom_proj = project_coordinate(right_bottom);

    for(double x = left_top_proj.x; x <= right_bottom_proj.x; x += m_step)
      {
        for(double y = left_top_proj.y; y <= right_bottom_proj.y; y += m_step)
          {
            if(!poly.probe_point({ x, y }) && left_top != right_bottom)
              {
                continue;
              }

            const std::size_t x_proj = project_single_coord(x, m_start.x, m_end.x, m_step);
            const std::size_t y_proj = project_single_coord(y, m_start.y, m_end.y, m_step);

            if(metal_idx % 2 == 0)
              {
                if(is_via_blockage)
                  {
                    m_h_lines[y_proj].m_pins[x_proj].m_status = details::AccessNode::Status::VIA_BLOCKAGE;
                  }
                else
                  {
                    m_h_lines[y_proj].m_pins[x_proj].m_blocked_metals.emplace(poly.m_metal);

                    if(x_proj == m_h_lines[y_proj].m_pins.size() - 1)
                      {
                        m_h_lines[y_proj].m_right_pin.m_blocked_metals.emplace(metal);

                        if(m_right != nullptr)
                          {
                            m_right->m_h_lines[y_proj].m_left_pin.m_blocked_metals.emplace(metal);
                          }
                      }

                    if(x_proj == 0)
                      {
                        m_h_lines[y_proj].m_left_pin.m_blocked_metals.emplace(metal);

                        if(m_left != nullptr)
                          {
                            m_left->m_h_lines[y_proj].m_right_pin.m_blocked_metals.emplace(metal);
                          }
                      }
                  }
              }
            else
              {
                if(is_via_blockage)
                  {
                    m_v_lines[x_proj].m_pins[y_proj].m_status = details::AccessNode::Status::VIA_BLOCKAGE;
                  }
                else
                  {
                    m_v_lines[x_proj].m_pins[y_proj].m_blocked_metals.emplace(poly.m_metal);

                    if(y_proj == m_v_lines[x_proj].m_pins.size() - 1)
                      {
                        m_v_lines[x_proj].m_right_pin.m_blocked_metals.emplace(metal);

                        if(m_bottom != nullptr)
                          {
                            m_bottom->m_v_lines[x_proj].m_left_pin.m_blocked_metals.emplace(metal);
                          }
                      }

                    if(y_proj == 0)
                      {
                        m_v_lines[x_proj].m_left_pin.m_blocked_metals.emplace(metal);

                        if(m_top != nullptr)
                          {
                            m_top->m_v_lines[x_proj].m_right_pin.m_blocked_metals.emplace(metal);
                          }
                      }
                  }
              }
          }
      }
  }

  /**
   * @brief Add a pin on a grid.
   *
   * @param pin A pin to add.
   */
  bool
  add_pin(Pin* pin)
  {
    const types::Metal& metal        = pin->m_access_points.m_metal;
    const std::size_t   metal_idx    = (uint8_t(metal) - 1) / 2 - 1;

    geom::Point         optimal_real = { 0.0, 0.0 };
    geom::PointS        optimal_proj = { 0, 0 };
    double              min_cost     = __DBL_MAX__;

    const auto          find_optimal = [&](details::AccessNode::Status status) {
      bool found_place = false;

      for(const auto [point, proj] : pin->m_access_points.m_points)
        {
          if(metal_idx % 2 == 0 && m_h_lines[proj.y].m_pins[proj.x].m_blocked_metals.count(metal) != 0)
            {
              continue;
            }

          if(metal_idx % 2 != 0 && m_v_lines[proj.x].m_pins[proj.y].m_blocked_metals.count(metal) != 0)
            {
              continue;
            }

          if((metal_idx % 2 == 0 && m_h_lines[proj.y].m_pins[proj.x].m_status == status) || (metal_idx % 2 != 0 && m_v_lines[proj.x].m_pins[proj.y].m_status == status))
            {
              const double cost_v = line_heuristic(m_v_lines[proj.x].m_assigned + 1);
              const double cost_h = line_heuristic(m_h_lines[proj.y].m_assigned + 1);

              if(cost_v + cost_h < min_cost)
                {
                  min_cost     = cost_v + cost_h;
                  optimal_real = point;
                  optimal_proj = proj;
                  found_place  = true;
                }
            }
        }

      return found_place;
    };

    if(!find_optimal(details::AccessNode::Status::FREE))
      {
        if(!find_optimal(details::AccessNode::Status::VIA_BLOCKAGE))
          {
            return false;
          }

        if(optimal_proj.x < m_v_lines.size() / 2)
          {
            m_h_lines[optimal_proj.y].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;

            if(m_left != nullptr)
              {
                m_left->m_h_lines[optimal_proj.y].m_right_pin.m_status = details::AccessNode::Status::OCCUPIED;
              }
          }
        else
          {
            m_h_lines[optimal_proj.y].m_right_pin.m_status = details::AccessNode::Status::OCCUPIED;

            if(m_right != nullptr)
              {
                m_right->m_h_lines[optimal_proj.y].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;
              }
          }

        if(metal_idx % 2 != 0)
          {
            if(optimal_proj.y < m_h_lines.size() / 2)
              {
                m_v_lines[optimal_proj.x].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;

                if(m_top != nullptr)
                  {
                    m_top->m_v_lines[optimal_proj.x].m_right_pin.m_status = details::AccessNode::Status::OCCUPIED;
                  }
              }
            else
              {
                m_v_lines[optimal_proj.x].m_right_pin.m_status = details::AccessNode::Status::OCCUPIED;

                if(m_bottom != nullptr)
                  {
                    m_bottom->m_v_lines[optimal_proj.x].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;
                  }
              }
          }
      }

    const double old_v_cost                                   = line_heuristic(m_v_lines[optimal_proj.x].m_assigned);
    m_v_lines[optimal_proj.x].m_pins[optimal_proj.y].m_ptr    = pin;
    m_v_lines[optimal_proj.x].m_pins[optimal_proj.y].m_status = details::AccessNode::Status::OCCUPIED;
    m_v_lines[optimal_proj.x].m_assigned += 1;

    const double old_h_cost                                   = line_heuristic(m_h_lines[optimal_proj.y].m_assigned);
    m_h_lines[optimal_proj.y].m_pins[optimal_proj.x].m_ptr    = pin;
    m_h_lines[optimal_proj.y].m_pins[optimal_proj.x].m_status = details::AccessNode::Status::OCCUPIED;
    m_h_lines[optimal_proj.y].m_assigned += 1;

    m_cost               = m_cost - old_v_cost - old_h_cost + line_heuristic(m_v_lines[optimal_proj.x].m_assigned) + line_heuristic(m_h_lines[optimal_proj.y].m_assigned);

    pin->m_ptr->m_center = optimal_real;
    pin->m_center        = optimal_proj;

    return true;
  }

  /**
   * @brief Add a cross on a grid.
   *
   * @param pin A pin to add.
   */
  void
  add_cross_pin(Pin* pin)
  {
    if(pin->m_ptr->m_is_placed)
      {
        const types::Metal metal     = pin->m_access_points.m_metal;
        const std::size_t  metal_idx = (uint8_t(metal) - 1) / 2 - 1;
        const std::size_t  x         = metal_idx % 2 == 0 ? 0 : project_single_coord(pin->m_ptr->m_center.x, m_start.x, m_end.x, m_step);
        const std::size_t  y         = metal_idx % 2 == 0 ? project_single_coord(pin->m_ptr->m_center.y, m_start.y, m_end.y, m_step) : 0;

        pin->m_center                = { x, y };
        return;
      }

    const types::Metal metal        = pin->m_access_points.m_metal;
    const std::size_t  metal_idx    = (uint8_t(metal) - 1) / 2 - 1;

    geom::PointS       optimal_proj = { 0, 0 };
    geom::Point        optimal_real = { 0.0, 0.0 };
    double             min_cost     = __DBL_MAX__;
    bool               found_place  = true;

    for(const auto [point, proj] : pin->m_access_points.m_points)
      {
        const std::size_t x = metal_idx % 2 == 0 ? m_v_lines.size() - 1 : proj.x;
        const std::size_t y = metal_idx % 2 == 0 ? proj.y : m_h_lines.size() - 1;

        if(metal_idx % 2 == 0)
          {
            if(m_h_lines[y].m_right_pin.m_status != details::AccessNode::Status::FREE && m_h_lines[y].m_right_pin.m_status != details::AccessNode::Status::VIA_BLOCKAGE)
              {
                continue;
              }

            if(m_h_lines[y].m_right_pin.m_blocked_metals.count(metal) != 0)
              {
                continue;
              }
          }

        if(metal_idx % 2 != 0)
          {
            if(m_v_lines[x].m_right_pin.m_status != details::AccessNode::Status::FREE && m_v_lines[x].m_right_pin.m_status != details::AccessNode::Status::VIA_BLOCKAGE)
              {
                continue;
              }

            if(m_v_lines[x].m_right_pin.m_blocked_metals.count(metal) != 0)
              {
                continue;
              }
          }

        {
          auto&                      line = metal_idx % 2 == 0 ? m_h_lines.at(y) : m_v_lines.at(x);
          const details::AccessNode& node = line.m_assigned != 0 ? get_last_line_node(line) : line.m_right_pin;

          if(node.m_status == details::AccessNode::Status::OCCUPIED && node.m_ptr && node.m_ptr->m_net == pin->m_net)
            {
              optimal_proj = { x, y };
              optimal_real = point;
              found_place  = true;
              break;
            }
        }

        if(metal_idx % 2 == 0 && m_right)
          {
            auto&                      line = m_right->m_h_lines.at(y);
            const details::AccessNode& node = line.m_assigned != 0 ? get_first_line_node(line) : line.m_left_pin;

            if(node.m_status == details::AccessNode::Status::OCCUPIED && node.m_ptr && node.m_ptr->m_net == pin->m_net)
              {
                optimal_proj = { x, y };
                optimal_real = point;
                found_place  = true;
                break;
              }
          }

        if(metal_idx % 2 != 0 && m_bottom)
          {
            auto&                      line = m_bottom->m_v_lines.at(x);
            const details::AccessNode& node = line.m_assigned != 0 ? get_first_line_node(line) : line.m_left_pin;

            if(node.m_status == details::AccessNode::Status::OCCUPIED && node.m_ptr && node.m_ptr->m_net == pin->m_net)
              {
                optimal_proj = { x, y };
                optimal_real = point;
                found_place  = true;
                break;
              }
          }

        const double cost = metal_idx % 2 == 0 ? line_heuristic(m_h_lines[y].m_assigned + 1) : line_heuristic(m_v_lines[x].m_assigned + 1);

        if(cost < min_cost)
          {
            min_cost     = cost;
            optimal_proj = { x, y };
            optimal_real = point;
            found_place  = true;
          }
      }

    if(!found_place)
      {
        throw std::runtime_error("DEF AccessPointGrid Error: Unable to place right-cross-pin");
      }

    auto& line                = metal_idx % 2 == 0 ? m_h_lines.at(optimal_proj.y) : m_v_lines.at(optimal_proj.x);

    line.m_right_pin.m_ptr    = pin;
    line.m_right_pin.m_status = details::AccessNode::Status::OCCUPIED;
    line.m_assigned += 1;

    /** Align pin to the gcell side */
    if(metal_idx % 2 == 0)
      {
        if(m_right != nullptr)
          {
            m_right->m_h_lines[optimal_proj.y].m_left_pin.m_ptr    = pin;
            m_right->m_h_lines[optimal_proj.y].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;
          }

        optimal_real.x = pin->m_ptr->m_ports[0].m_points[0].x;
      }
    else
      {
        if(m_bottom != nullptr)
          {
            m_bottom->m_v_lines[optimal_proj.x].m_left_pin.m_ptr    = pin;
            m_bottom->m_v_lines[optimal_proj.x].m_left_pin.m_status = details::AccessNode::Status::OCCUPIED;
          }

        optimal_real.y = pin->m_ptr->m_ports[0].m_points[0].y;
      }

    pin->m_center           = optimal_proj;
    pin->m_ptr->m_center    = optimal_real;
    pin->m_ptr->m_is_placed = true;
  }

  /**
   * @brief Add a pin between stacks on a grid.
   *
   * @param pin A pin to add.
   */
  void
  add_between_stack_pin(Pin* bottom_pin, Pin* top_pin, const std::vector<std::tuple<geom::Point, geom::Point, geom::PointS>>& shared_access_points)
  {
    const types::Metal& bottom_metal        = bottom_pin->m_access_points.m_metal;
    const std::size_t   bottom_metal_idx    = (uint8_t(bottom_metal) - 1) / 2 - 1;

    const types::Metal& top_metal           = bottom_pin->m_access_points.m_metal;
    const std::size_t   top_metal_idx       = (uint8_t(top_metal) - 1) / 2 - 1;

    geom::PointS        optimal_proj        = { 0, 0 };
    geom::Point         optimal_real_top    = { 0.0, 0.0 };
    geom::Point         optimal_real_bottom = { 0.0, 0.0 };

    double              min_cost            = __DBL_MAX__;
    bool                found_place         = false;

    for(const auto [top_point, bottom_point, proj] : shared_access_points)
      {
        const std::size_t x                = proj.x;
        const std::size_t y                = proj.y;

        const bool        is_blocked       = m_h_lines[y].m_pins[x].m_blocked_metals.count(bottom_metal) != 0 || m_v_lines[x].m_pins[y].m_blocked_metals.count(top_metal) != 0;

        const bool        is_occupied_h    = m_h_lines[y].m_pins[x].m_status == details::AccessNode::Status::OCCUPIED && m_h_lines[y].m_pins[x].m_ptr->m_net != bottom_pin->m_net;
        const bool        is_occupied_v    = m_v_lines[x].m_pins[y].m_status == details::AccessNode::Status::OCCUPIED && m_v_lines[x].m_pins[y].m_ptr->m_net != top_pin->m_net;

        const bool        is_via_blocked_h = m_h_lines[y].m_pins[x].m_status == details::AccessNode::Status::VIA_BLOCKAGE;
        const bool        is_via_blocked_v = m_v_lines[x].m_pins[y].m_status == details::AccessNode::Status::VIA_BLOCKAGE;

        if(is_blocked || is_occupied_h || is_occupied_v)
          {
            continue;
          }

        if(is_via_blocked_h)
          {
            if(x > m_h_lines[y].m_pins.size() / 2)
              {
                if(m_h_lines[y].m_right_pin.m_status == details::AccessNode::Status::OCCUPIED && m_h_lines[y].m_right_pin.m_ptr != nullptr && m_h_lines[y].m_right_pin.m_ptr->m_net != bottom_pin->m_net)
                  {
                    continue;
                  }
              }
            else
              {
                if(m_h_lines[y].m_left_pin.m_status == details::AccessNode::Status::OCCUPIED && m_h_lines[y].m_left_pin.m_ptr != nullptr && m_h_lines[y].m_left_pin.m_ptr->m_net != bottom_pin->m_net)
                  {
                    continue;
                  }
              }
          }

        if(is_via_blocked_v)
          {
            if(y > m_v_lines[x].m_pins.size() / 2)
              {
                if(m_v_lines[x].m_right_pin.m_status == details::AccessNode::Status::OCCUPIED && m_v_lines[x].m_right_pin.m_ptr != nullptr && m_v_lines[x].m_right_pin.m_ptr->m_net != top_pin->m_net)
                  {
                    continue;
                  }
              }
            else
              {
                if(m_v_lines[x].m_left_pin.m_status == details::AccessNode::Status::OCCUPIED && m_v_lines[x].m_left_pin.m_ptr != nullptr && m_v_lines[x].m_left_pin.m_ptr->m_net != top_pin->m_net)
                  {
                    continue;
                  }
              }
          }

        const double cost_v = line_heuristic(m_v_lines[x].m_assigned + 1);
        const double cost_h = line_heuristic(m_h_lines[y].m_assigned + 1);

        if(cost_v + cost_h < min_cost)
          {
            min_cost            = cost_v + cost_h;
            optimal_real_top    = top_point;
            optimal_real_bottom = bottom_point;
            optimal_proj        = { x, y };
            found_place         = true;
          }
      }

    if(!found_place)
      {
        throw std::runtime_error("DEF AccessPointGrid Error: Unable to place between-stack-pin");
      }

    const double old_h_cost                                   = line_heuristic(m_h_lines[optimal_proj.y].m_assigned);
    m_h_lines[optimal_proj.y].m_pins[optimal_proj.x].m_ptr    = bottom_pin;
    m_h_lines[optimal_proj.y].m_pins[optimal_proj.x].m_status = details::AccessNode::Status::OCCUPIED;
    m_h_lines[optimal_proj.y].m_assigned += 1;

    const double old_v_cost                                   = line_heuristic(m_v_lines[optimal_proj.x].m_assigned);
    m_v_lines[optimal_proj.x].m_pins[optimal_proj.y].m_ptr    = top_pin;
    m_v_lines[optimal_proj.x].m_pins[optimal_proj.y].m_status = details::AccessNode::Status::OCCUPIED;
    m_v_lines[optimal_proj.x].m_assigned += 1;

    m_cost                      = m_cost - old_v_cost - old_h_cost + line_heuristic(m_v_lines[optimal_proj.x].m_assigned) + line_heuristic(m_h_lines[optimal_proj.x].m_assigned);

    bottom_pin->m_ptr->m_center = optimal_real_bottom;
    bottom_pin->m_center        = optimal_proj;

    top_pin->m_ptr->m_center    = optimal_real_top;
    top_pin->m_center           = optimal_proj;
  }

  /**
   * @brief Get obstacles for a metal layer.
   *
   * @param metal
   * @return std::vector<geom::PointS>
   */
  std::vector<geom::PointS>
  get_obstacles(const types::Metal metal)
  {
    const std::size_t         metal_idx = (uint8_t(metal) - 1) / 2 - 1;
    std::vector<geom::PointS> points;

    if(metal_idx % 2 == 0)
      {
        for(std::size_t y = 0, end_y = m_h_lines.size(); y < end_y; ++y)
          {
            for(std::size_t x = 0, end_x = m_h_lines[y].m_pins.size(); x < end_x; ++x)
              {
                if(m_h_lines[y].m_pins[x].m_blocked_metals.count(metal) != 0)
                  {
                    points.emplace_back(x, y);
                  }
              }
          }
      }
    else
      {
        for(std::size_t x = 0, end_x = m_v_lines.size(); x < end_x; ++x)
          {
            for(std::size_t y = 0, end_y = m_v_lines[x].m_pins.size(); y < end_y; ++y)
              {
                if(m_v_lines[x].m_pins[y].m_blocked_metals.count(metal) != 0)
                  {
                    points.emplace_back(x, y);
                  }
              }
          }
      }

    return points;
  };

private:
  /**
   * @brief Project single coordinate on some axis.
   *
   * @param target The tareget to project.
   * @param start The start of an axis.
   * @param end The end of an axis.
   * @param step The step of an axis.
   * @return double
   */
  std::size_t
  project_single_coord(const double target, const double start, const double end, double step) const
  {
    double projection = std::round((target - start) / step) * step + start;

    if(projection < start)
      {
        projection = start;
      }

    if(projection > end)
      {
        projection = end;
      }

    return (projection - start) / step;
  }

  /**
   * @brief Project coordinate on the base grid.
   *
   * @param target The tareget to project.
   * @param start The start of an axis.
   * @param end The end of an axis.
   * @param step The step of an axis.
   * @return double
   */
  geom::Point
  project_coordinate(const geom::Point& target) const noexcept(true)
  {
    double projection_x = std::round((target.x - m_start.x) / m_step) * m_step + m_start.x;
    projection_x        = std::max(projection_x, m_start.x);
    projection_x        = std::min(projection_x, m_end.x);

    double projection_y = std::round((target.y - m_start.y) / m_step) * m_step + m_start.y;
    projection_y        = std::max(projection_y, m_start.y);
    projection_y        = std::min(projection_y, m_end.y);

    return { projection_x, projection_y };
  }

  /**
   * @brief Calculate cost of a line.
   *
   * @param size The size of a line.
   * @return double
   */
  double
  line_heuristic(const std::size_t size)
  {
    return std::pow(size, 2.0);
  };

  /**
   * @brief Get the first node in a line.
   *
   * @param line
   * @return const details::AccessNode&
   */
  const details::AccessNode&
  get_first_line_node(const details::AccessLine& line)
  {
    for(std::size_t i = 0, end = line.m_pins.size(); i < end; ++i)
      {
        if(line.m_pins[i].m_status == details::AccessNode::Status::OCCUPIED)
          {
            return line.m_pins.at(i);
          }
      }

    return line.m_pins.front();
  }

  /**
   * @brief Get the last node in a line.
   *
   * @param line
   * @return const details::AccessNode&
   */
  const details::AccessNode&
  get_last_line_node(const details::AccessLine& line)
  {
    for(std::size_t i = line.m_pins.size(), end = 0; i > end; --i)
      {
        if(line.m_pins[i - 1].m_status == details::AccessNode::Status::OCCUPIED)
          {
            return line.m_pins.at(i - 1);
          }
      }

    return line.m_pins.front();
  }

public:
  /** Neighbors */
  AccessPointGrid* m_left   = nullptr; ///> Left side neighbor, even metal layers
  AccessPointGrid* m_right  = nullptr; ///> Right side neighbor, even metal layers
  AccessPointGrid* m_top    = nullptr; ///> Top side neighbor, odd metal layers
  AccessPointGrid* m_bottom = nullptr; ///> Bottom side neighbor, odd metal layers

private:
  geom::Point                      m_start;   ///> The start of a grid.
  geom::Point                      m_end;     ///> The end of a grid.
  double                           m_step;    ///> The step of a grid.
  double                           m_cost;    ///> The cost of a grid.
  std::vector<details::AccessLine> m_h_lines; ///> Horizontal lines(tracks)
  std::vector<details::AccessLine> m_v_lines; ///> Vertical lines(tracks)
};

} // namespace def

#endif