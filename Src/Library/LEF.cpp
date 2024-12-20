#include <cstdio>
#include <iostream>
#include <string_view>
#include <vector>

#include "Include/LEF.hpp"
#include "Include/Utils.hpp"

#define DEFINE_LEF_CALLBACK(callback_type, callback_name)                   \
  {                                                                         \
    if(type != (callback_type))                                             \
      {                                                                     \
        std::cerr << "LEF Error: Wrong callback type." << std::endl;        \
        return 2;                                                           \
      }                                                                     \
    if(!(instance))                                                         \
      {                                                                     \
        std::cerr << "LEF Error: instance is null." << std::endl;           \
        return 2;                                                           \
      }                                                                     \
    LEF* lef_instance = static_cast<LEF*>(instance);                        \
    try                                                                     \
      {                                                                     \
        lef_instance->callback_name(param, lef_instance->m_data);           \
      }                                                                     \
    catch(const std::exception& e)                                          \
      {                                                                     \
        std::cerr << "LEF Error: " << e.what() << std::endl;                \
        return 2;                                                           \
      }                                                                     \
    catch(...)                                                              \
      {                                                                     \
        std::cerr << "LEF Error: Unknown exception occurred." << std::endl; \
        return 2;                                                           \
      }                                                                     \
    return 0;                                                               \
  }

namespace lef
{

/** =============================== CONSTRUCTORS ============================================ */

LEF::LEF()
{
  if(lefrInitSession() != 0)
    {
      throw std::runtime_error("LEF Error: Unknown exception occurred while trying to initialize.");
    }

  lefrSetLogFunction(&error_callback);

  lefrSetUnitsCbk(&d_units_callback);
  lefrSetLayerCbk(&d_layer_callback);
  lefrSetMacroBeginCbk(&d_macro_begin_callback);
  lefrSetMacroSizeCbk(&d_macro_size_callback);
  lefrSetPinCbk(&d_pin_callback);
  lefrSetObstructionCbk(&d_obstruction_callback);
}

LEF::~LEF()
{
  lefrClear();
}

/** =============================== PUBLIC METHODS ==================================== */

Data
LEF::parse(const std::filesystem::path& dir_path) const
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

  for(const auto& path : tech_lef_files)
    {
      FILE* file = fopen(path.c_str(), "r");

      if(file == nullptr)
        {
          throw std::runtime_error("Lef Error: Can't open the file = \"" + path.string() + "\"");
        }

      int32_t status = lefrRead(file, path.c_str(), (void*)this);

      fclose(file);

      if(status != 0)
        {
          std::cout << "LEF Error: Unable to parse the file - \"" + path.string() + "\"" << std::endl;
        }
    }

  for(const auto& path : lef_files)
    {
      FILE* file = fopen(path.c_str(), "r");

      if(file == nullptr)
        {
          throw std::runtime_error("Lef Error: Can't open the file = \"" + path.string() + "\"");
        }

      int32_t status = lefrRead(file, path.c_str(), (void*)this);

      fclose(file);

      if(status != 0)
        {
          std::cout << "LEF Error: Unable to parse the file - \"" + path.string() + "\"" << std::endl;
        }
    }

  return m_data;
};

/** =============================== PRIVATE METHODS =================================== */

void
LEF::units_callback(lefiUnits* param, Data& data)
{
  if(!param->hasDatabase())
    {
      throw std::runtime_error("Units database not defined.");
    }

  data.m_database_number = param->databaseNumber();
}

void
LEF::layer_callback(lefiLayer* param, Data& data)
{
  if(!param->hasType())
    {
      throw std::runtime_error("Layer TYPE not defined.");
    }

  types::Metal metal = utils::get_skywater130_metal(param->name());

  if(metal == types::Metal::NONE)
    {
      return;
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

  data.m_layers[metal] = std::move(layer);
}

void
LEF::macro_begin_callback(const char* param, Data& data)
{
  const std::string_view macro_name = param;

  if(macro_name.empty())
    {
      throw std::runtime_error("Macro name was not found.");
    }

  m_last_macro_name = macro_name;
}

void
LEF::macro_size_callback(lefiNum param, Data& data)
{
  if(m_last_macro_name.empty())
    {
      throw std::runtime_error("Macro name was not found.");
    }

  data.m_macros[m_last_macro_name].m_width  = param.x;
  data.m_macros[m_last_macro_name].m_height = param.y;
}

void
LEF::pin_callback(lefiPin* param, Data& data)
{
  const std::string name = param->name();

  if(name == "VPWR" || name == "VGND")
    {
      return;
    }

  std::vector<std::pair<types::Polygon, types::Metal>> rects;
  types::Metal                                         top_metal;

  for(std::size_t i = 0, end = param->numPorts(); i < end; ++i)
    {
      lefiGeometries* geometries = param->port(i);
      types::Metal    metal;

      for(std::size_t j = 0, end = geometries->numItems(); j < end; ++j)
        {
          switch(geometries->itemType(j))
            {
            case lefiGeomEnum::lefiGeomLayerE:
              {
                metal = utils::get_skywater130_metal(geometries->getLayer(j));

                if(top_metal < metal)
                  {
                    top_metal = metal;
                  }

                break;
              }
            case lefiGeomEnum::lefiGeomRectE:
              {
                if(types::Metal::NONE == metal)
                  {
                    break;
                  }

                lefiGeomRect* rect = geometries->getRect(j);
                rects.emplace_back(utils::make_clockwise_rectangle({ rect->xl, rect->yl, rect->xh, rect->yh }), metal);
                break;
              }
            default: break;
            }
        }
    }

  pin::Pin pin;

  if(param->hasDirection())
    {
      pin::set_direction(pin, param->direction());
    }

  if(param->hasUse())
    {
      pin::set_use(pin, param->use());
    }

  for(const auto& [rect, metal] : rects)
    {
      if(metal == top_metal)
        {
          pin.m_ports.emplace_back(std::move(rect), metal);
        }
      else
        {
          pin.m_obs.emplace_back(std::move(rect), metal);
        }
    }

  data.m_macros[m_last_macro_name].m_pins.emplace(std::move(name), std::move(pin));
}

void
LEF::obstruction_callback(lefiObstruction* param, Data& data)
{
  lefiGeometries* geometries = param->geometries();
  OBSMetalGroup   obs_metal_group;

  for(std::size_t i = 0, end = geometries->numItems(); i < end; ++i)
    {
      switch(geometries->itemType(i))
        {
        case lefiGeomEnum::lefiGeomLayerE:
          {
            data.m_macros[m_last_macro_name].m_obs.push_back(obs_metal_group);

            obs_metal_group.m_metal = utils::get_skywater130_metal(geometries->getLayer(i));
            obs_metal_group.m_rect.clear();
            break;
          }
        case lefiGeomEnum::lefiGeomRectE:
          {
            lefiGeomRect* rect = geometries->getRect(i);
            obs_metal_group.m_rect.emplace_back(utils::make_clockwise_rectangle({ rect->xl, rect->yl, rect->xh, rect->yh }));
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
};

/** =============================== PRIVATE STATIC METHODS =================================== */

void
LEF::error_callback(const char* msg)
{
  std::cout << "LEF Error: " << msg << std::endl;
}

int32_t
LEF::d_units_callback(lefrCallbackType_e type, lefiUnits* param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrUnitsCbkType, units_callback);
}

int32_t
LEF::d_layer_callback(lefrCallbackType_e type, lefiLayer* param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrLayerCbkType, layer_callback);
}

int32_t
LEF::d_macro_begin_callback(lefrCallbackType_e type, const char* param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrMacroBeginCbkType, macro_begin_callback);
}

int32_t
LEF::d_macro_size_callback(lefrCallbackType_e type, lefiNum param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrMacroSizeCbkType, macro_size_callback);
}

int32_t
LEF::d_pin_callback(lefrCallbackType_e type, lefiPin* param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrPinCbkType, pin_callback);
}

int32_t
LEF::d_obstruction_callback(lefrCallbackType_e type, lefiObstruction* param, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrObstructionCbkType, obstruction_callback);
}

} // namespace lef
