#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "Include/DEF/DEF.hpp"
#include "Include/GlobalUtils.hpp"

#define DEFINE_DEF_CALLBACK(callback_type, callback_name)                   \
  {                                                                         \
    if(type != (callback_type))                                             \
      {                                                                     \
        std::cerr << "DEF Error: Wrong callback type." << std::endl;        \
        return 2;                                                           \
      }                                                                     \
    if(!(instance))                                                         \
      {                                                                     \
        std::cerr << "DEF Error: instance is null." << std::endl;           \
        return 2;                                                           \
      }                                                                     \
    DEF* def_instance = static_cast<DEF*>(instance);                        \
    try                                                                     \
      {                                                                     \
        def_instance->callback_name(param, def_instance->m_data);           \
      }                                                                     \
    catch(const std::exception& e)                                          \
      {                                                                     \
        std::cerr << "DEF Error: " << e.what() << std::endl;                \
        return 2;                                                           \
      }                                                                     \
    catch(...)                                                              \
      {                                                                     \
        std::cerr << "DEF Error: Unknown exception occurred." << std::endl; \
        return 2;                                                           \
      }                                                                     \
    return 0;                                                               \
  }

namespace def
{

DEF::DEF()
    : m_data()
{
  if(defrInitSession() != 0)
    {
      throw std::runtime_error("DEF Error: Unknown exception occurred while trying to initialize.");
    }

  defrSetAddPathToNet();

  defrSetLogFunction(&error_callback);

  defrSetDieAreaCbk(&d_die_area_callback);
  defrSetRowCbk(&d_row_callback);
  defrSetTrackCbk(&d_track_callback);
  defrSetGcellGridCbk(&d_gcell_callback);
  defrSetPinCbk(&d_pin_callback);
  defrSetComponentStartCbk(&d_component_start_callback);
  defrSetComponentCbk(&d_component_callback);
  defrSetSNetCbk(&d_special_net_callback);
  defrSetNetCbk(&d_net_callback);
}

DEF::~DEF()
{
  defrClear();
}

Data
DEF::parse(const std::filesystem::path& file_path) const
{
  if(!std::filesystem::exists(file_path) || file_path.extension() != ".def")
    {
      throw std::invalid_argument("Can't find the def file by path - \"" + file_path.string() + "\".");
    }

  FILE* file = fopen(file_path.c_str(), "r");

  if(file == nullptr)
    {
      throw std::runtime_error("DEF Error: Can't open the file = \"" + file_path.string() + "\"");
    }

  int32_t status = defrRead(file, file_path.c_str(), (void*)this, 1);

  fclose(file);

  if(status != 0)
    {
      std::cout << "DEF Error: Unable to parse the file - \"" + file_path.string() + "\"" << std::endl;
    }

  return m_data;
}

void
DEF::die_area_callback(defiBox* param, Data& data)
{
  data.m_box[0] = param->xl();
  data.m_box[1] = param->yl();
  data.m_box[2] = param->xh();
  data.m_box[3] = param->yh();
};

void
DEF::row_callback(defiRow* param, Data& data)
{
  RowTemplate row;
  row.m_x                              = param->x();
  row.m_step_x                         = param->xStep();
  row.m_num_x                          = param->xNum();
  row.m_y                              = param->y();
  row.m_step_y                         = param->yStep();
  row.m_num_y                          = param->yNum();

  const types::Orientation orientation = static_cast<types::Orientation>(param->orient());

  switch(orientation)
    {
    case types::Orientation::E:
    case types::Orientation::FE:
      {
        std::swap(row.m_x, row.m_y);
        std::swap(row.m_step_x, row.m_step_y);
        break;
      }
    case types::Orientation::S:
    case types::Orientation::FS:
      {
        row.m_x = (data.m_box[2] - data.m_box[0]) - row.m_x;
        row.m_y = (data.m_box[3] - data.m_box[1]) - row.m_y;
        break;
      }
    case types::Orientation::W:
    case types::Orientation::FW:
      {
        std::swap(row.m_x, row.m_y);
        std::swap(row.m_step_x, row.m_step_y);
        row.m_x = (data.m_box[2] - data.m_box[0]) - row.m_x;
        row.m_y = (data.m_box[3] - data.m_box[1]) - row.m_y;
        break;
      }
    default:
      break;
    }

  m_rows.emplace_back(std::move(row));
}

void
DEF::track_callback(defiTrack* param, Data& data)
{
  const types::Metal metal = ::utils::get_skywater130_metal(param->layer(0));

  /** L1 is not used for routing */
  if(metal == types::Metal::L1)
    {
      return;
    }

  TrackTemplate track;
  track.m_start   = param->x();
  track.m_num     = param->xNum();
  track.m_spacing = param->xStep();
  track.m_metal   = metal;

  if(param->macro()[0] == 'X')
    {
      data.m_tracks.emplace_back(std::move(track));
    }
}

void
DEF::gcell_callback(defiGcellGrid* param, Data& data)
{
  GCellGridTemplate gcell_grid;
  gcell_grid.m_start = param->x();
  gcell_grid.m_num   = param->xNum();
  gcell_grid.m_step  = param->xStep();

  if(std::strcmp(param->macro(), "X") == 0)
    {
      m_gcell_grid_x.emplace_back(std::move(gcell_grid));
    }
  else
    {
      m_gcell_grid_y.emplace_back(std::move(gcell_grid));
    }
};

void
DEF::component_start_callback(int32_t param, Data& data)
{
  /** Collect all columns, handles case with multiple grids along x axis */
  std::vector<double> columns;
  double              max_grid_x = 0.0;

  for(std::size_t i = 0, end_i = m_gcell_grid_x.size(); i < end_i; ++i)
    {
      const double start = m_gcell_grid_x[i].m_start;
      const double step  = m_gcell_grid_x[i].m_step;

      for(std::size_t j = 0, end_j = m_gcell_grid_x[i].m_num + 1; j < end_j; ++j)
        {
          columns.emplace_back(start + j * step);
        }

      max_grid_x = start + (m_gcell_grid_x[i].m_num) * step;
    }

  std::sort(columns.begin(), columns.end());

  /** Collect all rows, handles case with multiple grids along y axis */
  std::vector<double> rows;
  double              max_grid_y = 0.0;

  for(std::size_t i = 0, end_i = m_gcell_grid_y.size(); i < end_i; ++i)
    {
      const double start = m_gcell_grid_y[i].m_start;
      const double step  = m_gcell_grid_y[i].m_step;

      for(std::size_t j = 0, end_j = m_gcell_grid_y[i].m_num + 1; j < end_j; ++j)
        {
          rows.emplace_back(start + j * step);
        }

      max_grid_y = start + (m_gcell_grid_y[i].m_num) * step;
    }

  std::sort(rows.begin(), rows.end());

  /** Create all gcells */
  std::vector<std::vector<GCell*>> gcells;
  gcells.reserve(rows.size());

  for(std::size_t y = 0, end_y = rows.size() - 1; y < end_y; ++y)
    {
      std::vector<GCell*> gcell_row;
      gcell_row.reserve(columns.size());

      for(std::size_t x = 0, end_x = columns.size() - 1; x < end_x; ++x)
        {
          GCell*      gcell      = new GCell(x, y, geom::Polygon({ columns[x], rows[y], columns[x + 1], rows[y + 1] }));
          const auto& box_points = gcell->m_box.m_points;

          /** Add x tracks to the gcell */
          for(std::size_t i = 0, end_i = m_data.m_tracks.size(); i < end_i; ++i)
            {
              const double start        = data.m_tracks[i].m_start;
              const double step         = data.m_tracks[i].m_spacing;

              const double left_edge_y  = start + std::ceil((box_points[3].y - start) / step) * step;
              const double right_edge_y = start + std::floor((box_points[0].y - start) / step) * step;

              const double left_edge_x  = start + std::ceil((box_points[1].x - start) / step) * step;
              const double right_edge_x = start + std::floor((box_points[0].x - start) / step) * step;

              if(i == 0)
                {
                  gcell->set_base(geom::Point(left_edge_x, left_edge_y), geom::Point(right_edge_x, right_edge_y), step, end_i);
                }
              else
                {
                  gcell->add_track(geom::Point(left_edge_x, left_edge_y), geom::Point(right_edge_x, right_edge_y), step, data.m_tracks[i].m_metal);
                }
            }

          gcell_row.emplace_back(std::move(gcell));
        }

      gcells.emplace_back(std::move(gcell_row));
    }

  data.m_max_gcell_x = max_grid_x;
  data.m_max_gcell_y = max_grid_y;
  data.m_gcells      = std::move(gcells);
  data.m_components.reserve(param);
}

void
DEF::component_callback(defiComponent* param, Data& data)
{
  ComponentTemplate component;
  component.m_id          = param->id();
  component.m_name        = param->name();
  component.m_x           = param->placementX();
  component.m_y           = param->placementY();
  component.m_orientation = static_cast<types::Orientation>(param->placementOrient());

  data.m_components.emplace_back(std::move(component));
}

void
DEF::pin_callback(defiPin* param, Data& data)
{
  std::unordered_map<types::Metal, std::vector<geom::Polygon>> polygons;
  types::Metal                                                 top_metal = types::Metal ::NONE;

  for(std::size_t i = 0, end_i = param->numPorts(); i < end_i; ++i)
    {
      defiPinPort* port = param->pinPort(i);
      int32_t      x    = port->placementX();
      int32_t      y    = port->placementY();

      for(std::size_t j = 0, end_j = port->numLayer(); j < end_j; ++j)
        {
          int32_t xl;
          int32_t yl;
          int32_t xh;
          int32_t yh;

          port->bounds(j, &xl, &yl, &xh, &yh);

          types::Metal metal = ::utils::get_skywater130_metal(port->layer(j));

          if(types::Metal::NONE == metal)
            {
              continue;
            }
          else if(top_metal < metal)
            {
              top_metal = metal;
            }

          geom::Polygon target({ static_cast<double>(xl) + x, static_cast<double>(yl) + y, static_cast<double>(xh) + x, static_cast<double>(yh) + y }, metal);

          // TODO Find a way to optimize this part
          auto          remove_itr = std::remove_if(polygons[metal].begin(), polygons[metal].end(), [&target](const auto& polygon) {
            if(!(target / polygon))
              {
                return false;
              }

            target += polygon;
            return true;
          });

          if(remove_itr != polygons[metal].end())
            {
              polygons[metal].erase(remove_itr, polygons[metal].end());
            }

          polygons[metal].emplace_back(std::move(target));
        }
    }

  if(polygons.size() == 0)
    {
      return;
    }

  std::vector<geom::Polygon> obs;

  for(const auto& [metal, v_polygons] : polygons)
    {
      if(metal == top_metal)
        {
          continue;
        }

      obs.insert(obs.end(), std::make_move_iterator(v_polygons.begin()), std::make_move_iterator(v_polygons.end()));
    }

  if(std::strcmp(param->use(), "GROUND") != 0 && std::strcmp(param->use(), "POWER") != 0)
    {
      pin::Pin* pin = new pin::Pin();
      pin->m_ports  = std::move(polygons[top_metal]);
      pin->m_obs.insert(pin->m_obs.end(), std::make_move_iterator(obs.begin()), std::make_move_iterator(obs.end()));
      pin->m_name              = "PIN:" + std::string(param->pinName());

      data.m_pins[pin->m_name] = pin;
    }
  else
    {
      data.m_obstacles.insert(data.m_obstacles.end(), std::make_move_iterator(obs.begin()), std::make_move_iterator(obs.end()));
    }
}

void
DEF::net_callback(defiNet* param, Data& data)
{
  if(std::strcmp(param->use(), "SIGNAL") == 0 || std::strcmp(param->use(), "CLOCK") == 0)
    {
      Net* net    = new Net();
      net->m_idx  = data.m_nets.size();
      net->m_name = param->name();

      for(std::size_t i = 0, end = param->numConnections(); i < end; ++i)
        {
          const std::string instance_name = param->instance(i);
          const std::string pin_name      = param->pin(i);

          net->m_pins.emplace(std::move(instance_name + ":" + pin_name));
        }

      data.m_nets[net->m_name] = net;
      return;
    }

  defiPath* path;
  defiWire* wire;

  for(std::size_t i = 0, end_i = param->numWires(); i < end_i; ++i)
    {
      wire = param->wire(i);

      for(std::size_t j = 0, end_j = wire->numPaths(); j < end_j; ++j)
        {
          defiPath_e   path_element_type;
          int32_t      ext    = -1;
          int32_t      x      = -1;
          int32_t      y      = -1;
          int32_t      prev_x = -1;
          int32_t      prev_y = -1;
          int32_t      width  = 0;
          types::Metal metal;

          path = wire->path(j);
          path->initTraverse();

          while((path_element_type = static_cast<defiPath_e>(path->next())) != defiPath_e::DEFIPATH_DONE)
            {
              switch(path_element_type)
                {
                case defiPath_e::DEFIPATH_LAYER:
                  {
                    metal = ::utils::get_skywater130_metal(path->getLayer());
                    break;
                  }
                case defiPath_e::DEFIPATH_WIDTH:
                  {
                    width = path->getWidth();
                    break;
                  }
                case defiPath_e::DEFIPATH_POINT:
                case defiPath_e::DEFIPATH_FLUSHPOINT:
                  {
                    if(path_element_type == defiPath_e::DEFIPATH_POINT)
                      {
                        path->getPoint(&x, &y);
                      }
                    else
                      {
                        path->getFlushPoint(&x, &y, &ext);
                      }

                    if(prev_x >= 0 && prev_y >= 0 && x >= 0 && y >= 0)
                      {
                        if(prev_x == x)
                          {
                            prev_x -= width / 2;
                            x += width / 2;
                          }
                        else
                          {
                            prev_y -= width / 2;
                            y += width / 2;
                          }

                        const geom::Polygon target({ prev_x - 10.0, prev_y - 10.0, x + 10.0, y + 10.0 }, metal);
                        auto                gcell_with_overlaps = GCell::find_overlaps(target, data.m_gcells, data.m_max_gcell_x, data.m_max_gcell_y);

                        for(auto& [gcell, overlap] : gcell_with_overlaps)
                          {
                            gcell->m_obstacles.emplace_back(std::move(overlap));
                          }

                        x      = -1;
                        y      = -1;
                        prev_x = -1;
                        prev_y = -1;
                        width  = 0;
                      }

                    prev_x = x;
                    prev_y = y;
                    break;
                  }
                default:
                  break;
                }
            }
        }
    }
}

void
DEF::error_callback(const char* msg)
{
  std::cout << "DEF Error: " << msg << std::endl;
}

int32_t
DEF::d_die_area_callback(defrCallbackType_e type, defiBox* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrDieAreaCbkType, die_area_callback);
}

int32_t
DEF::d_row_callback(defrCallbackType_e type, defiRow* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrRowCbkType, row_callback);
}

int32_t
DEF::d_track_callback(defrCallbackType_e type, defiTrack* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrTrackCbkType, track_callback);
}

int32_t
DEF::d_gcell_callback(defrCallbackType_e type, defiGcellGrid* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrGcellGridCbkType, gcell_callback);
}

int32_t
DEF::d_pin_callback(defrCallbackType_e type, defiPin* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrPinCbkType, pin_callback);
}

int32_t
DEF::d_component_start_callback(defrCallbackType_e type, int32_t param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrComponentStartCbkType, component_start_callback);
}

int32_t
DEF::d_component_callback(defrCallbackType_e type, defiComponent* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrComponentCbkType, component_callback);
}

int32_t
DEF::d_net_callback(defrCallbackType_e type, defiNet* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrNetCbkType, net_callback);
}

int32_t
DEF::d_special_net_callback(defrCallbackType_e type, defiNet* param, void* instance)
{
  DEFINE_DEF_CALLBACK(defrSNetCbkType, net_callback);
}

} // namespace def
