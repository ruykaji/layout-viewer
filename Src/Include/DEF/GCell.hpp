#ifndef __GCELL_HPP__
#define __GCELL_HPP__

#include "Include/DEF/AccessPointGrid.hpp"
#include "Include/DEF/Stack.hpp"

namespace def
{

struct Response
{
  Net*                      m_ptr       = nullptr;

  bool                      m_is_failed = false;
  std::string               m_error;

  std::vector<geom::LineS>  m_paths;     ///> Result path for requested net.
  std::vector<geom::PointS> m_inner_via; ///> As for now stack have only two metal layers, so inner via can be only between 0 and 1 layers
  std::vector<geom::PointS> m_outer_via; ///> Outer via don't need metal layer as they suppose to be on the first layer of a next stack.
};

class GCell
{
public:
  GCell(const std::size_t x, const std::size_t y, const geom::Polygon box)
      : m_x(x), m_y(y), m_box(box) {};

  ~GCell()
  {
    delete m_access_point_grid;
  }

public:
  /**
   * @brief Finds overlaps between gcell grid and polygon.
   *
   * @param poly A polygon.
   * @param gcells A gcell grid.
   * @param width The width of a single gcell.
   * @param height The height of a single gcell.
   * @return std::vector<std::pair<GCell*, geom::Polygon>>
   */
  static std::vector<std::pair<GCell*, geom::Polygon>>
  find_overlaps(const geom::Polygon& poly, const std::vector<std::vector<GCell*>>& gcells, const uint32_t width, const uint32_t height);

  static void
  connect(GCell* lhs, GCell* rhs, types::Metal metal)
  {
    const std::size_t metal_idx = (uint8_t(metal) - 1) / 2 - 1;

    if(metal_idx % 2 == 0)
      {
        if(lhs->m_x > rhs->m_x)
          {
            std::swap(lhs, rhs);
          }

        lhs->m_access_point_grid->m_right = rhs->m_access_point_grid;
        rhs->m_access_point_grid->m_left  = lhs->m_access_point_grid;
      }
    else
      {
        if(lhs->m_y > rhs->m_y)
          {
            std::swap(lhs, rhs);
          }

        lhs->m_access_point_grid->m_bottom = rhs->m_access_point_grid;
        rhs->m_access_point_grid->m_top    = lhs->m_access_point_grid;
      }
  }

public:
  void
  set_base(const geom::Point& start, const geom::Point& end, const double step, const std::size_t total_layers)
  {
    m_access_point_grid = new AccessPointGrid(start, end, step);
    m_stacks.resize(std::ceil(total_layers / 2.0));

    add_track(start, end, step, types::Metal::M1);
  }

  void
  add_track(const geom::Point& start, const geom::Point& end, const double step, const types::Metal metal)
  {
    m_grids[metal] = { start, end, step };
  }

  void
  add_obstacle(const geom::Polygon& obstacle)
  {
    m_obstacles.emplace_back(std::move(obstacle));
  }

  void
  add_net(const std::vector<pin::Pin*> pins, Net* net)
  {
    constexpr std::size_t SINGLE_POINT_PIN_SIZE = 4;

    if(!m_nets.empty() && m_nets.back() == net)
      {
        std::cout << "DEF GCell Warning: Attempt to add the same net twice in GCell_x" << std::to_string(m_x) << "_y_" << std::to_string(m_y) << std::endl;
        return;
      }

    for(const auto pin : pins)
      {
        if(pin->m_ports[0].m_metal != types::Metal::L1 && pin->m_ports[0].m_metal != types::Metal::M1 && pin->m_ports[0].m_metal != types::Metal::M2)
          {
            return;
          }
      }

    m_nets.emplace_back(net);

    std::vector<std::vector<Pin*>> def_pins;
    std::vector<uint8_t>           is_stack_used;

    is_stack_used.resize(m_stacks.size(), 0);
    def_pins.resize(m_stacks.size(), {});

    bool        is_min_stack_set = false;
    std::size_t min_stack        = __SIZE_MAX__;
    std::size_t max_stack        = 0;

    for(auto pin : pins)
      {
        const geom::Polygon&    port       = pin->m_ports.at(0);
        const types::Metal      metal      = port.m_metal == types::Metal::L1 ? types::Metal::M1 : port.m_metal;
        const std::size_t       stack_idx  = ((uint8_t(metal) - 1) / 2 - 1) / 2;
        const utils::MetalGrid& metal_grid = m_grids.at(metal);

        Pin*                    new_pin    = new Pin(pin, net);

        /** Check if pin is a single point */
        if(port.m_points.size() == SINGLE_POINT_PIN_SIZE && (port.m_points[0].x == port.m_points[2].x || port.m_points[0].y == port.m_points[2].y))
          {
            new_pin->m_type = Pin::Type::CROSS;

            if(!pin->m_is_placed)
              {
                new_pin->m_access_points.m_points = utils::find_access_points(new_pin->m_ptr->m_ports.at(0), new_pin->m_access_points.m_metal, m_grids);
              }

            m_cross_pins.emplace_back(new_pin);
            def_pins[stack_idx].emplace_back(new_pin);
          }
        else
          {
            /** Inners pins doesn't add to stacks in this step 'cause they access points can change metal layer and so stack */
            new_pin->m_type                   = Pin::Type::INNER;
            new_pin->m_access_points.m_points = utils::find_access_points(new_pin->m_ptr->m_ports.at(0), new_pin->m_access_points.m_metal, m_grids);

            m_inner_pins.emplace_back(new_pin);
          }

        is_stack_used[stack_idx] = 1;

        min_stack                = std::min(stack_idx, min_stack);
        max_stack                = std::max(stack_idx, max_stack);
      }

    for(std::size_t i = min_stack; i <= max_stack; ++i)
      {
        if(i < max_stack && is_stack_used[i] == 1 && is_stack_used[i + 1] == 1)
          {
            std::pair<Pin*, Pin*> both_sides = { nullptr, nullptr };

            /** Add a bottom pin */
            {
              const types::Metal metal = types::Metal((i * 2 + 1) * 2 + 3);

              pin::Pin*          pin   = new pin::Pin();
              pin->m_ports.emplace_back(geom::Polygon{ { 0, 0, 0, 0 }, metal });

              Pin* new_pin           = new Pin(pin, net);
              new_pin->m_ptr->m_name = "BETWEEN_STACKS";
              new_pin->m_type        = Pin::Type::BETWEEN_STACKS;
              both_sides.first       = new_pin;

              def_pins[i].emplace_back(new_pin);
            }

            /** Add a top pin */
            {
              const types::Metal metal = types::Metal((i * 2 + 1) * 2 + 5);

              pin::Pin*          pin   = new pin::Pin();
              pin->m_ports.emplace_back(geom::Polygon{ { 0, 0, 0, 0 }, metal });

              Pin* new_pin           = new Pin(pin, net);
              new_pin->m_ptr->m_name = "BETWEEN_STACKS";
              new_pin->m_type        = Pin::Type::BETWEEN_STACKS;
              both_sides.second      = new_pin;

              def_pins[i + 1].emplace_back(new_pin);
            }

            m_between_stack_pins.emplace_back(both_sides);
          }

        if(def_pins[i].empty())
          {
            continue;
          }

        m_stacks[i].add_net(def_pins[i], net);
      }
  }

  void
  setup_global_obstacles()
  {
    /** Balance pins on the base gird */
    for(const auto& obs : m_obstacles)
      {
        m_access_point_grid->add_obstacle(obs);
      }
  }

  void
  setup_inner_pins()
  {
    std::sort(m_inner_pins.begin(), m_inner_pins.end(), [](const Pin* lhs, const Pin* rhs) { return lhs->m_access_points.m_points.size() > rhs->m_access_points.m_points.size(); });

    for(auto pin : m_inner_pins)
      {
        std::size_t metal_idx = (uint8_t(pin->m_access_points.m_metal) - 1) / 2 - 1;

        try
          {
            while(!m_access_point_grid->add_pin(pin))
              {
                // TODO: Remove hardcode of number of metal layers
                if(++metal_idx > 4)
                  {
                    throw std::runtime_error("DEF AccessPointGrid Error: Unable to place the pin: " + pin->m_ptr->m_name);
                  }

                const types::Metal      metal      = types::Metal((metal_idx + 1) * 2 + 1);
                const utils::MetalGrid& metal_grid = m_grids.at(metal);

                pin->m_access_points.m_metal       = metal;
                pin->m_access_points.m_points      = utils::find_access_points(pin->m_ptr->m_ports[0], metal, m_grids);
              }

            {
              /** Place pins obstacles */
              for(const auto& obs : pin->m_ptr->m_obs)
                {
                  m_access_point_grid->add_obstacle(obs);
                }

              if(pin->m_ptr->m_ports[0].m_metal != types::Metal::L1 && pin->m_type != Pin::Type::BETWEEN_STACKS)
                {
                  m_access_point_grid->add_obstacle(pin->m_ptr->m_ports[0]);
                }

              if(m_bounding_boxes.count(pin->m_net) == 0)
                {
                  m_bounding_boxes[pin->m_net] = std::make_pair(geom::Point{ __DBL_MAX__, __DBL_MAX__ }, geom::Point{ 0.0, 0.0 });
                }

              const geom::Point& center      = pin->m_ptr->m_center;
              auto& [left_top, right_bottom] = m_bounding_boxes.at(pin->m_net);

              left_top                       = { std::min(center.x, left_top.x), std::min(center.y, left_top.y) };
              right_bottom                   = { std::max(center.x, right_bottom.x), std::max(center.y, right_bottom.y) };
            }
          }
        catch(const std::exception& e)
          {
            m_is_error = true;

            std::cout << "DEF GCell Error: GCell_x_" << std::to_string(m_x) << "_y_" << std::to_string(m_y) << std::endl;
            std::cout << " -- NET " << pin->m_net->m_name << std::endl;
            std::cout << " -- " << e.what() << std::endl;
            std::cout << std::endl;
          }
      }
  }

  void
  setup_cross_pins()
  {
    std::sort(m_cross_pins.begin(), m_cross_pins.end(), [](const Pin* lhs, const Pin* rhs) { return uint8_t(lhs->m_ptr->m_is_placed) > uint8_t(rhs->m_ptr->m_is_placed); });

    /** Place cross pins */
    for(auto pin : m_cross_pins)
      {
        try
          {
            m_access_point_grid->add_cross_pin(pin);

            /** Collecting bounding boxes of nets, to find between stack pin if needed */
            {
              if(m_bounding_boxes.count(pin->m_net) == 0)
                {
                  m_bounding_boxes[pin->m_net] = std::make_pair(geom::Point{ __DBL_MAX__, __DBL_MAX__ }, geom::Point{ 0.0, 0.0 });
                }

              const geom::Point& center      = pin->m_ptr->m_center;
              auto& [left_top, right_bottom] = m_bounding_boxes.at(pin->m_net);

              left_top                       = { std::min(center.x, left_top.x), std::min(center.y, left_top.y) };
              right_bottom                   = { std::max(center.x, right_bottom.x), std::max(center.y, right_bottom.y) };
            }
          }
        catch(const std::exception& e)
          {
            m_is_error = true;

            std::cout << "DEF GCell Error: GCell_x_" << std::to_string(m_x) << "_y_" << std::to_string(m_y) << std::endl;
            std::cout << " -- NET " << pin->m_net->m_name << std::endl;
            std::cout << " -- " << e.what() << std::endl;
            std::cout << std::endl;
          }
      }
  }

  void
  setup_between_stack_pins()
  {
    const auto& [bb_left_top, bb_right_bottom] = m_box.get_extrem_points();
    const utils::MetalGrid& m1_metal_grid      = m_grids.at(types::Metal::M1);

    /** Place between stack pins */
    for(auto [bottom_pin, top_pin] : m_between_stack_pins)
      {
        const types::Metal      bottom_metal      = bottom_pin->m_ptr->m_ports[0].m_metal;
        const utils::MetalGrid& bottom_metal_grid = m_grids.at(bottom_metal);

        const types::Metal      top_metal         = top_pin->m_ptr->m_ports[0].m_metal;
        const utils::MetalGrid& top_metal_grid    = m_grids.at(top_metal);

        auto& [left_top, right_bottom]            = m_bounding_boxes.at(bottom_pin->m_net);
        left_top.x                                = std::max(bb_left_top.x, left_top.x - top_metal_grid.m_step);
        left_top.y                                = std::max(bb_left_top.y, left_top.y - top_metal_grid.m_step);
        right_bottom.x                            = std::min(bb_right_bottom.x, right_bottom.x + top_metal_grid.m_step);
        right_bottom.y                            = std::min(bb_right_bottom.y, right_bottom.y + top_metal_grid.m_step);

        bottom_pin->m_ptr->m_ports[0]             = std::move(geom::Polygon{ { left_top.x, left_top.y, right_bottom.x, right_bottom.y }, bottom_metal });
        bottom_pin->m_access_points.m_points      = utils::find_access_points(bottom_pin->m_ptr->m_ports.at(0), bottom_metal, m_grids);

        top_pin->m_ptr->m_ports[0]                = std::move(geom::Polygon{ { left_top.x, left_top.y, right_bottom.x, right_bottom.y }, top_metal });
        top_pin->m_access_points.m_points         = utils::find_access_points(top_pin->m_ptr->m_ports.at(0), top_metal, m_grids);

        std::vector<std::tuple<geom::Point, geom::Point, geom::PointS>> shared_access_points;

        for(const auto& [top_point, top_proj] : top_pin->m_access_points.m_points)
          {
            for(const auto& [bottom_point, bottom_proj] : bottom_pin->m_access_points.m_points)
              {
                if(top_proj == bottom_proj)
                  {
                    shared_access_points.emplace_back(top_point, bottom_point, top_proj);
                  }
              }
          }

        try
          {
            m_access_point_grid->add_between_stack_pin(bottom_pin, top_pin, shared_access_points);
          }
        catch(const std::exception& e)
          {
            m_is_error = true;

            std::cout << "DEF GCell Error: GCell_x" << std::to_string(m_x) << "_y_" << std::to_string(m_y) << std::endl;
            std::cout << " -- NET " << bottom_pin->m_net->m_name << std::endl;
            std::cout << " -- " << e.what() << std::endl;
            std::cout << std::endl;
          }
      }
  }

  void
  setup_stacks()
  {
    for(const auto& pin : m_inner_pins)
      {
        const types::Metal metal     = pin->m_access_points.m_metal;
        const std::size_t  stack_idx = ((uint8_t(metal) - 1) / 2 - 1) / 2;

        m_stacks[stack_idx].add_pin(pin);
      }

    for(std::size_t i = 0, end = m_stacks.size(); i < end; ++i)
      {
        if(m_stacks[i].is_empty())
          {
            continue;
          }

        /** Add top and bottom grid on a stack */
        m_stacks[i].set_all_grids(m_grids);

        const types::Metal      bottom_metal = types::Metal((i * 2 + 1) * 2 + 1);
        const utils::MetalGrid& bottom_grid  = m_grids[bottom_metal];
        m_stacks[i].add_grid(bottom_grid, bottom_metal);

        const types::Metal      top_metal = types::Metal((i * 2 + 1) * 2 + 3);
        const utils::MetalGrid& top_grid  = m_grids[top_metal];
        m_stacks[i].add_grid(top_grid, top_metal);

        /** Add obstacles on a stack */
        const std::vector<geom::PointS> bottom_obs = m_access_point_grid->get_obstacles(bottom_metal);
        m_stacks[i].add_obstacle(bottom_obs);

        const std::vector<geom::PointS> top_obs = m_access_point_grid->get_obstacles(top_metal);
        m_stacks[i].add_obstacle(top_obs);
      }
  }

  std::pair<Stack&, bool>
  get_next_stack()
  {
    Stack& next_stack = m_stacks[m_stack_itr];
    ++m_stack_itr;

    return { next_stack, m_stack_itr == m_stacks.size() };
  }

public:
  bool                                                                m_is_error;
  std::size_t                                                         m_x;                           ///> Position by x axis in gcell grid.
  std::size_t                                                         m_y;                           ///> Position by y axis in gcell grid.
  geom::Polygon                                                       m_box;                         ///> Bounding box of the gcell.
  std::vector<geom::Polygon>                                          m_obstacles;                   ///> All obstacles within the gcell.
  AccessPointGrid*                                                    m_access_point_grid = nullptr; ///> Access point grid.
  std::vector<Pin*>                                                   m_inner_pins;                  ///> Inner pins.
  std::vector<std::pair<Pin*, Pin*>>                                  m_between_stack_pins;          ///> Between stack pins.
  std::vector<Pin*>                                                   m_cross_pins;                  ///> Cross pins.
  std::vector<Net*>                                                   m_nets;                        ///> Nets.
  std::vector<Stack>                                                  m_stacks;                      ///> Stacks.
  std::size_t                                                         m_stack_itr = 0;               ///> Iterator on the current stack.
  std::map<types::Metal, utils::MetalGrid, utils::MetalGrid::Compare> m_grids;                       ///> Map of grids for each used metal layer.
  std::unordered_map<Net*, std::pair<geom::Point, geom::Point>>       m_bounding_boxes;              ///> Bounding boxes for all nets.
};

} // namespace def

#endif