#include <cstdio>
#include <iostream>

#include "Include/LEF.hpp"

#define DEFINE_LEF_CALLBACK(callback_type, callback_name)                                                                                  \
  {                                                                                                                                        \
    if(type != (callback_type))                                                                                                            \
      {                                                                                                                                    \
        std::cerr << "LEF Error: Wrong callback type." << std::endl;                                                                       \
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
        lef_instance->callback_name(param, lef_instance->m_data);                                                                          \
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

/** =============================== PROTECTED METHODS ==================================== */

void
LEF::parse(const std::filesystem::path& file_path, void* data) const
{
  FILE* file = fopen(file_path.c_str(), "r");

  if(file == nullptr)
    {
      throw std::runtime_error("Lef Error: Can't open the file = \"" + file_path.string() + "\"");
    }

  m_data         = data;
  int32_t status = lefrRead(file, file_path.c_str(), (void*)this);

  fclose(file);

  if(status != 0)
    {
      std::cout << "LEF Error: Unable to parse the file - \"" + file_path.string() + "\"" << std::endl;
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
