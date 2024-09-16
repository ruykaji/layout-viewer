#include <cstdio>
#include <iostream>

#include <lefrReader.hpp>

#include "Include/LEF.hpp"

#define DEFINE_LEF_CALLBACK(callback_type, callback_name)                                                                                  \
  {                                                                                                                                        \
    if(type != (callback_type))                                                                                                            \
      {                                                                                                                                    \
        return 2;                                                                                                                          \
      }                                                                                                                                    \
    if(!(instance))                                                                                                                        \
      {                                                                                                                                    \
        std::cerr << "LEF Error: instance is null." << std::endl;                                                                          \
        return 2;                                                                                                                          \
      }                                                                                                                                    \
    LEF* lef_instance = static_cast<LEF*>(instance);                                                                                       \
    try                                                                                                                                    \
      {                                                                                                                                    \
        lef_instance->callback_name(data);                                                                                                 \
      }                                                                                                                                    \
    catch(const std::exception& e)                                                                                                         \
      {                                                                                                                                    \
        std::cerr << "LEF Error: " << e.what() << std::endl;                                                                               \
        return 2;                                                                                                                          \
      }                                                                                                                                    \
    catch(...)                                                                                                                             \
      {                                                                                                                                    \
        std::cerr << "LEF Error: Unknown exception occurred." << std::endl;                                                                \
        return 2;                                                                                                                          \
      }                                                                                                                                    \
    return 0;                                                                                                                              \
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

  lefrSetUnitsCbk(&d_units_callback);
  lefrSetLayerCbk(&d_layer_callback);
  lefrSetMacroBeginCbk(&d_macro_callback);
  lefrSetMacroSizeCbk(&d_macro_size_callback);
  lefrSetPinCbk(&d_pin_callback);
  lefrSetObstructionCbk(&d_obstruction_callback);
}

LEF::~LEF()
{
  lefrClear();
}

/** =============================== PROTECTED METHODS ==================================== */

void
LEF::parse(const std::filesystem::path& file_path)
{
  FILE* file = fopen(file_path.c_str(), "r");

  if(lefrRead(file, file_path.c_str(), this) != 0)
    {
      throw std::runtime_error("LEF Error: Unable to parse file - \"" + file_path.string() + "\"");
    }
};

/** =============================== PRIVATE STATIC METHODS =================================== */

int32_t
LEF::d_units_callback(lefrCallbackType_e type, lefiUnits* data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrUnitsCbkType, units_callback);
}

int32_t
LEF::d_layer_callback(lefrCallbackType_e type, lefiLayer* data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrLayerCbkType, layer_callback);
}

int32_t
LEF::d_macro_callback(lefrCallbackType_e type, const char* data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrMacroCbkType, macro_callback);
}

int32_t
LEF::d_macro_size_callback(lefrCallbackType_e type, lefiNum data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrMacroCbkType, macro_size_callback);
}

int32_t
LEF::d_pin_callback(lefrCallbackType_e type, lefiPin* data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrPinCbkType, pin_callback);
}

int32_t
LEF::d_obstruction_callback(lefrCallbackType_e type, lefiObstruction* data, void* instance)
{
  DEFINE_LEF_CALLBACK(lefrPinCbkType, obstruction_callback);
}

} // namespace lef
