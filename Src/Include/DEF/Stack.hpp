#ifndef __STACK_HPP__
#define __STACK_HPP__

#include "Include/DEF/Pin.hpp"
#include "Include/Graph.hpp"
#include "Include/Matrix.hpp"

namespace def::details
{

struct Net
{
  def::Net*          m_ptr;
  std::size_t        m_idx;       ///> Index of a net in a stack.
  std::vector<Pin*>  m_pins;      ///> All pins of a net.
  matrix::SetOfNodes m_terminals; ///> All placed on matrix terminals.
};

} // namespace def::details

namespace def
{

class Stack
{
public:
  void
  set_all_grids(const std::map<types::Metal, utils::MetalGrid, utils::MetalGrid::Compare>& grids) noexcept(true)
  {
    m_all_grids = grids;
  }

  void
  add_grid(const utils::MetalGrid& grid, const types::Metal metal)
  {
    m_used_grids.emplace_back(grid);
    m_used_metals.emplace_back(metal);
  }

  /**
   * @brief Add a pin to a stack.
   *
   * @param ptr A pointer to the net.
   */
  void
  add_net(const std::vector<Pin*>& pins, Net* net)
  {
    if(net == nullptr)
      {
        throw std::runtime_error("Stack Error: Nullptr net");
      }

    if(m_nets.count(net) != 0)
      {
        throw std::runtime_error("Stack Error: Trying to add the same net twice");
      }

    m_nets.emplace(net, details::Net{ net, m_nets.size(), std::move(pins) });
  }

  /**
   * @brief Add a pin to a stack.
   *
   * @param ptr A pointer to the net.
   */
  void
  add_pin(Pin* pin)
  {
    if(pin == nullptr)
      {
        throw std::runtime_error("Stack Error: Nullptr pin");
      }

    if(pin->m_net == nullptr)
      {
        throw std::runtime_error("Stack Error: Nullptr net");
      }

    if(m_nets.count(pin->m_net) == 0)
      {
        m_nets.emplace(pin->m_net, details::Net{ pin->m_net, m_nets.size(), { pin } });
      }
    else
      {
        m_nets[pin->m_net].m_pins.emplace_back(pin);
      }
  }

  /**
   * @brief Checks if this stack contains any nets.
   *
   * @return true
   * @return false
   */
  bool
  is_empty()
  {
    return m_nets.empty();
  }

  void
  add_obstacle(const std::vector<geom::PointS>& points)
  {
    m_obstacles.emplace_back(std::move(points));
  }

  void
  create_matrix(const std::size_t size, const std::size_t step)
  {
    constexpr std::size_t CROSS_PIN_PADDING = 2;
    constexpr std::size_t INDEX_SHIFT       = 1;

    auto [size_x, size_y]                   = utils::project<geom::PointS>(m_all_grids.at(types::Metal::M1).m_end, m_all_grids.at(types::Metal::M1));
    const geom::PointS end_base             = { size_x, size_y };

    size_x                                  = size_x * step + INDEX_SHIFT + CROSS_PIN_PADDING;
    size_y                                  = size_y * step + INDEX_SHIFT + CROSS_PIN_PADDING;
    m_matrix                                = matrix::Matrix({ size_x, size_y, 2 });

    /** Place grids */
    for(std::size_t z = 0, end_z = m_used_grids.size(); z < end_z; ++z)
      {
        const utils::MetalGrid& grid  = m_used_grids.at(z);
        const types::Metal      metal = m_used_metals.at(z);

        if(grid.m_step == 0.0)
          {
            break;
          }

        if(z % 2 == 0)
          {
            for(double y = grid.m_start.y; y <= grid.m_end.y; y += grid.m_step)
              {
                const geom::PointS proj_int = utils::project_down<geom::PointS>({ grid.m_start.x, y }, metal, m_all_grids);

                for(std::size_t x = INDEX_SHIFT; x <= end_base.x * step + 1; ++x)
                  {
                    m_matrix.set_at(double(types::Cell::PATH), x, proj_int.y * step + 1, z);
                  }
              }
          }
        else
          {
            for(double x = grid.m_start.x; x <= grid.m_end.x; x += grid.m_step)
              {
                const geom::PointS proj_int = utils::project_down<geom::PointS>({ x, grid.m_start.y }, metal, m_all_grids);

                for(std::size_t y = INDEX_SHIFT; y <= end_base.y * step + 1; ++y)
                  {
                    m_matrix.set_at(double(types::Cell::PATH), proj_int.x * step + 1, y, z);
                  }
              }
          }
      }

    /** Set obstacles including outer layers */
    for(std::size_t z = 0, end = m_obstacles.size(); z < end; ++z)
      {
        if(m_obstacles[z].empty())
          {
            continue;
          }

        geom::PointS start_point;
        geom::PointS end_point;

        for(std::size_t i = 0, end_i = m_obstacles[z].size() - 1; i < end_i; ++i)
          {
            start_point = m_obstacles[z][i];
            end_point   = m_obstacles[z][i + 1];

            if(end_point.x - start_point.x > 1 || end_point.y - start_point.y > 1)
              {
                m_matrix.set_at(0, start_point.x * step + 1, start_point.y * step + 1, z);
                m_matrix.set_at(0, end_point.x * step + 1, end_point.y * step + 1, z);
                continue;
              }

            for(std::size_t y = start_point.y * step + 1; y <= end_point.y * step + 1; ++y)
              {
                for(std::size_t x = start_point.x * step + 1; x <= end_point.x * step + 1; ++x)
                  {
                    m_matrix.set_at(0, x, y, z);
                  }
              }
          }
      }

    /** Place pins */
    for(auto& [_, net] : m_nets)
      {
        for(const auto pin : net.m_pins)
          {
            const types::Metal metal     = pin->m_access_points.m_metal;
            const std::size_t  metal_idx = (uint8_t(metal) - 1) / 2 - 1;

            geom::PointS       pos       = { pin->m_center.x * step, pin->m_center.y * step };

            if(pin->m_type != Pin::Type::CROSS)
              {
                ++pos.x;
                ++pos.y;
              }
            else
              {
                if(metal_idx % 2 == 0)
                  {
                    ++pos.y;

                    if(pos.x == end_base.x * step)
                      {
                        pos.x += CROSS_PIN_PADDING;
                      }
                  }
                else
                  {
                    ++pos.x;

                    if(pos.y == end_base.y * step)
                      {
                        pos.y += CROSS_PIN_PADDING;
                      }
                  }
              }

            pin->m_matrix_pos = { pos.x, pos.y };
            m_matrix.set_at(double(types::Cell::TERMINAL), pos.x, pos.y, metal_idx % 2);

            net.m_terminals.emplace(pos.x, pos.y, metal_idx % 2);
            m_terminals.emplace(pos.x, pos.y, metal_idx % 2);
          }
      }
  };

  void
  create_graph()
  {
    std::queue<matrix::Node> queue;

    for(uint8_t x = 0; x < m_matrix.m_shape.m_x; ++x)
      {
        for(uint8_t y = 0; y < m_matrix.m_shape.m_y; ++y)
          {
            if(m_matrix.get_at(x, y, 0) != 0 && m_matrix.get_at(x, y, 1) != 0)
              {
                const matrix::Node new_node{ x, y, 0 };

                m_node_map[new_node] = m_nodes.size();
                m_nodes.emplace_back(new_node);
                m_graph.place_node();
                queue.push(new_node);
              }
          }
      }

    const auto add_edge = [&](const matrix::Node& front, const matrix::Node next_node, const uint32_t weight) {
      uint32_t dest_idx;

      if(m_node_map.count(next_node) == 0)
        {
          dest_idx              = m_nodes.size();

          m_node_map[next_node] = m_nodes.size();
          m_nodes.push_back(next_node);
          m_graph.place_node();
          queue.push(next_node);
        }
      else
        {
          dest_idx = m_node_map[next_node];
        }

      const uint32_t source_idx = m_node_map[front];

      m_graph.add_edge(weight, source_idx, dest_idx);
    };

    const auto search_direction = [&](int8_t dx, int8_t dy, int8_t dz, const matrix::Node& front) {
      uint8_t        x            = front.m_x;
      uint8_t        y            = front.m_y;
      uint8_t        z            = front.m_z;

      const uint8_t& matrix_value = m_matrix.get_at(x, y, z);

      uint8_t        new_x        = x;
      uint8_t        new_y        = y;
      uint8_t        new_z        = z;

      while(true)
        {
          if((dx > 0 && new_x < m_matrix.m_shape.m_x - 1) || (dx < 0 && new_x > 0))
            {
              new_x += dx;
            }
          else if(dx != 0)
            {
              break;
            }

          if((dy > 0 && new_y < m_matrix.m_shape.m_y - 1) || dy < 0 && new_y > 0)
            {
              new_y += dy;
            }
          else if(dy != 0)
            {
              break;
            }

          if((dz > 0 && new_z < m_matrix.m_shape.m_z - 1) || dz < 0 && new_z > 0)
            {
              new_z += dz;
            }
          else if(dz != 0)
            {
              break;
            }

          const uint8_t& next_matrix_value = m_matrix.get_at(new_x, new_y, new_z);
          bool           is_via            = new_z % 2 == 0 ? m_matrix.get_at(new_x, new_y, 1) != 0 : m_matrix.get_at(new_x, new_y, 0) != 0;

          if(is_via || next_matrix_value == uint8_t(types::Cell::TERMINAL))
            {
              const uint32_t weight = new_z != z ? 4 * std::max(new_z, z) : (std::abs(new_x - x) + std::abs(new_y - y));
              add_edge(front, { new_x, new_y, new_z }, weight);
              break;
            }

          if(next_matrix_value == 0)
            {
              break;
            }
        }
    };

    while(!queue.empty())
      {
        const auto front = queue.front();
        queue.pop();

        if(front.m_z % 2 == 0)
          {
            search_direction(1, 0, 0, front);
            search_direction(-1, 0, 0, front);
          }
        else
          {
            search_direction(0, 1, 0, front);
            search_direction(0, -1, 0, front);
          }

        search_direction(0, 0, 1, front);
        search_direction(0, 0, -1, front);
      }
  }

public:
  matrix::Matrix                                       m_matrix;    ///> Level sized matrix.
  graph::Graph                                         m_graph;     ///> Graph that represent the matrix.
  std::vector<matrix::Node>                            m_nodes;     ///> Map node to position on the matrix.
  matrix::MapOfNodes                                   m_node_map;  ///> Map position on matrix to the graph nodes.
  matrix::SetOfNodes                                   m_terminals; ///> All terminals of all nets.
  std::unordered_map<Net*, details::Net, Net::HashPtr> m_nets;      ///> All nets within a stack.

private:
  std::vector<utils::MetalGrid>                                       m_used_grids; ///> Metal grids used by this stack.
  std::vector<types::Metal>                                           m_used_metals;
  std::map<types::Metal, utils::MetalGrid, utils::MetalGrid::Compare> m_all_grids; ///> All available metal grids.
  std::vector<std::vector<geom::PointS>>                              m_obstacles; ///> Obstacles for each grid.
};

} // namespace def

#endif