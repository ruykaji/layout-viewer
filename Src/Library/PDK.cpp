#include <iostream>
#include <string_view>

#include "Include/PDK.hpp"
#include "Include/Utils.hpp"

#define GET_PDK()                                                                                                                          \
  if(data == nullptr)                                                                                                                      \
    {                                                                                                                                      \
      throw std::runtime_error("PDK Error: Loss of context.");                                                                             \
    }                                                                                                                                      \
  PDK* pdk = reinterpret_cast<PDK*>(data)

namespace pdk
{

void
Pin::set_direction(Pin& pin, const std::string_view direction)
{
  if(direction == "FEEDTHRU")
    {
      pin.m_direction = Direction::FEEDTHRU;
    }
  else if(direction == "INPUT")
    {
      pin.m_direction = Direction::INPUT;
    }
  else if(direction == "OUTPUT")
    {
      pin.m_direction = Direction::OUTPUT;
    }
  else if(direction == "INOUT")
    {
      pin.m_direction = Direction::INOUT;
    }
}

void
Pin::set_use(Pin& pin, const std::string_view use)
{
  if(use == "ANALOG")
    {
      pin.m_use = Use::ANALOG;
    }
  else if(use == "SCAN")
    {
      pin.m_use = Use::SCAN;
    }
  else if(use == "TIEOFF")
    {
      pin.m_use = Use::TIEOFF;
    }
  else if(use == "RESET")
    {
      pin.m_use = Use::RESET;
    }
  else if(use == "SIGNAL")
    {
      pin.m_use = Use::SIGNAL;
    }
  else if(use == "CLOCK")
    {
      pin.m_use = Use::CLOCK;
    }
  else if(use == "POWER")
    {
      pin.m_use = Use::POWER;
    }
  else if(use == "GROUND")
    {
      pin.m_use = Use::GROUND;
    }
}

PDK
Reader::compile(const std::filesystem::path& dir_path) const
{
  if(!std::filesystem::exists(dir_path))
    {
      throw std::invalid_argument("Can't find directory with pdk by path - \"" + dir_path.string() + "\".");
    }

  std::vector<std::filesystem::path> lef_files;
  std::vector<std::filesystem::path> tech_lef_files;

  for(auto& itr : std::filesystem::recursive_directory_iterator(dir_path))
    {
      if(itr.is_regular_file())
        {
          const std::filesystem::path file_path      = itr.path();
          const std::string_view      file_path_view = file_path.c_str();
          const std::size_t           extension_pos  = file_path_view.find_first_of('.');

          if(extension_pos != std::string::npos)
            {
              const std::string_view extension = file_path_view.substr(extension_pos);

              if(extension == ".lef")
                {
                  lef_files.emplace_back(std::move(file_path));
                }
              else if(extension == ".tlef")
                {
                  tech_lef_files.emplace_back(std::move(file_path));
                }
            }
        }
    }

  PDK pdk;

  for(const auto& path : tech_lef_files)
    {
      parse(path, reinterpret_cast<void*>(&pdk));
    }

  for(const auto& path : lef_files)
    {
      parse(path, reinterpret_cast<void*>(&pdk));
    }

  return pdk;
}

void
Reader::units_callback(lefiUnits* param, void* data) const
{
  GET_PDK();

  if(!param->hasDatabase())
    {
      throw std::runtime_error("Units database not defined.");
    }

  pdk->m_database_number = param->databaseNumber();
}

void
Reader::layer_callback(lefiLayer* param, void* data) const
{
  GET_PDK();

  if(!param->hasType())
    {
      throw std::runtime_error("Layer TYPE not defined.");
    }

  Layer                  layer;

  const std::string_view type = param->type();

  if(type == "ROUTING")
    {
      layer.m_type = Layer::Type::ROUTING;

      if(!param->hasDirection())
        {
          throw std::runtime_error("Layer DIRECTION not defined");
        }

      const std::string_view direction = param->direction();

      if(direction == "VERTICAL")
        {
          layer.m_direction = Layer::Direction::VERTICAL;
        }
      else if(direction == "HORIZONTAL")
        {
          layer.m_direction = Layer::Direction::HORIZONTAL;
        }
      else
        {
          throw std::runtime_error("Layer DIRECTION is neither VERTICAL or HORIZONTAL");
        }

      if(!param->hasPitch() && !param->hasXYPitch())
        {
          throw std::runtime_error("Layer PITCH not defined.");
        }

      if(param->hasXYPitch())
        {
          layer.m_pitch_x = param->pitchX();
          layer.m_pitch_y = param->pitchY();
        }
      else
        {
          layer.m_pitch_x = layer.m_pitch_y = param->pitch();
        }

      if(!param->hasWidth())
        {
          throw std::runtime_error("Layer WIDTH not defined.");
        }

      layer.m_width = param->width();
    }
}

void
Reader::macro_begin_callback(const char* param, void* data) const
{
  GET_PDK();

  const std::string_view macro_name = param;

  if(macro_name.empty())
    {
      throw std::runtime_error("Macro name was not found.");
    }

  m_last_macro_name = macro_name;
}

void
Reader::macro_size_callback(lefiNum param, void* data) const
{
  GET_PDK();

  if(m_last_macro_name.empty())
    {
      throw std::runtime_error("Macro name was not found.");
    }

  pdk->m_macros[m_last_macro_name].m_width  = param.x;
  pdk->m_macros[m_last_macro_name].m_height = param.y;
}

void
Reader::pin_callback(lefiPin* param, void* data) const
{
  GET_PDK();

  Pin pin;
  pin.m_name = param->name();

  if(param->hasDirection())
    {
      Pin::set_direction(pin, param->direction());
    }

  if(param->hasUse())
    {
      Pin::set_use(pin, param->use());
    }

  for(std::size_t i = 0, end = param->numPorts(); i < end; ++i)
    {
      lefiGeometries* geometries = param->port(i);
      Pin::Port       port;

      for(std::size_t j = 0, end = geometries->numItems(); j < end; ++j)
        {
          switch(geometries->itemType(j))
            {
            case lefiGeomEnum::lefiGeomLayerE:
              {
                port.m_metal = utils::get_skywater130_metal(geometries->getLayer(j));
                break;
              }
            case lefiGeomEnum::lefiGeomRectE:
              {
                lefiGeomRect* rect = geometries->getRect(j);
                port.m_rect        = utils::make_clockwise_rectangle({ rect->xl, rect->yl, rect->xh, rect->yh });
                break;
              }
            default: break;
            }
        }

      pin.m_ports.emplace_back(std::move(port));
    }

  pdk->m_macros[m_last_macro_name].m_pins.emplace_back(std::move(pin));
}

void
Reader::obstruction_callback(lefiObstruction* param, void* data) const
{
  GET_PDK();

  lefiGeometries* geometries = param->geometries();

  for(std::size_t i = 0, end = geometries->numItems(); i < end; ++i)
    {
      switch(geometries->itemType(i))
        {
        case lefiGeomEnum::lefiGeomRectE:
          {
            lefiGeomRect* rect = geometries->getRect(i);
            pdk->m_macros[m_last_macro_name].m_geometry.emplace_back(utils::make_clockwise_rectangle({ rect->xl, rect->yl, rect->xh, rect->yh }));
            break;
          }
        case lefiGeomEnum::lefiGeomPolygonE:
          {
            std::cout << "LEF Warning: Polygons obstructions are not supported." << std::endl;
            break;
          }
        default: break;
        }
    }
}

} // namespace pdk
