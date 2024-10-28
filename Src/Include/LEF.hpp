#ifndef __LEF_HPP__
#define __LEF_HPP__

#include <cstdint>
#include <filesystem>

#include <lefrReader.hpp>

namespace lef
{

class LEF
{
public:
  /** =============================== CONSTRUCTORS ============================================ */

  /**
   * @brief Constructs a new LEF parser object.
   *
   */
  LEF();

  /**
   * @brief Destroys the LEF parser object.
   *
   */
  virtual ~LEF();

protected:
  /** =============================== PROTECTED METHODS ==================================== */

  /**
   * @brief Parses a LEF file from the given file path.
   *
   * @param file_path The path to the LEF file to be parsed.
   */
  void
  parse(const std::filesystem::path& file_path, void* data) const;

protected:
  /** =============================== PROTECTED VIRTUAL METHODS ============================ */

  /**
   * @brief Callback function invoked when units are parsed in the LEF file.
   *
   * @param param Pointer to the lefiUnits structure containing unit param.
   * @param data Pointer to the user defined data.
   */
  virtual void
  units_callback(lefiUnits* param, void* data) const {};

  /**
   * @brief Callback function invoked when a layer is parsed in the LEF file.
   *
   * @param param Pointer to the lefiLayer structure containing layer param.
   * @param data Pointer to the user defined data.
   */
  virtual void
  layer_callback(lefiLayer* param, void* data) const {};

  /**
   * @brief Callback function invoked when a macro is parsed in the LEF file.
   *
   * @param param The name of the macro.
   * @param data Pointer to the user defined data.
   */
  virtual void
  macro_begin_callback(const char* param, void* data) const {};

  /**
   * @brief Callback function invoked when the size of a macro is parsed.
   *
   * @param param The size of the macro.
   * @param data Pointer to the user defined data.
   */
  virtual void
  macro_size_callback(lefiNum param, void* data) const {};

  /**
   * @brief Callback function invoked when a pin is parsed in the LEF file.
   *
   * @param param Pointer to the lefiPin structure containing pin param.
   * @param data Pointer to the user defined data.
   */
  virtual void
  pin_callback(lefiPin* param, void* data) const {};

  /**
   * @brief Callback function invoked when an obstruction is parsed.
   *
   * @param param Pointer to the lefiObstruction structure containing obstruction param.
   * @param data Pointer to the user defined data.
   */
  virtual void
  obstruction_callback(lefiObstruction* param, void* data) const {};

private:
  /** =============================== PRIVATE STATIC METHODS =================================== */

  /**
   * @brief Prints internal lefrRead function error.
   *
   * @param msg Error message.
   */
  static void
  error_callback(const char* msg);

  /**
   * @brief Static callback function for units parsing.
   *
   * @param type The type of callback.
   * @param param Pointer to the lefiUnits structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_units_callback(lefrCallbackType_e type, lefiUnits* param, void* instance);

  /**
   * @brief Static callback function for layer parsing.
   *
   * @param type The type of callback.
   * @param param Pointer to the lefiLayer structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_layer_callback(lefrCallbackType_e type, lefiLayer* param, void* instance);

  /**
   * @brief Static callback function for macro parsing.
   *
   * @param type The type of callback.
   * @param param The macro name.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_macro_begin_callback(lefrCallbackType_e type, const char* param, void* instance);

  /**
   * @brief Static callback function for macro size parsing.
   *
   * @param type The type of callback.
   * @param param The macro size.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_macro_size_callback(lefrCallbackType_e type, lefiNum param, void* instance);

  /**
   * @brief Static callback function for pin parsing.
   *
   * @param type The type of callback.
   * @param param Pointer to the lefiPin structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_pin_callback(lefrCallbackType_e type, lefiPin* param, void* instance);

  /**
   * @brief Static callback function for obstruction parsing.
   *
   * @param type The type of callback.
   * @param param Pointer to the lefiObstruction structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_obstruction_callback(lefrCallbackType_e type, lefiObstruction* param, void* instance);

protected:
  mutable void* m_data; ///> User defined data
};

} // namespace lef

#endif