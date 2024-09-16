#ifndef __LEF_HPP__
#define __LEF_HPP__

#include <cstdint>
#include <filesystem>

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
  parse(const std::filesystem::path& file_path);

protected:
  /** =============================== PROTECTED VIRTUAL METHODS ============================ */

  /**
   * @brief Callback function invoked when units are parsed in the LEF file.
   *
   * @param data Pointer to the lefiUnits structure containing unit data.
   */
  virtual void
  units_callback(lefiUnits* data) {};

  /**
   * @brief Callback function invoked when a layer is parsed in the LEF file.
   *
   * @param data Pointer to the lefiLayer structure containing layer data.
   */
  virtual void
  layer_callback(lefiLayer* data) {};

  /**
   * @brief Callback function invoked when a macro is parsed in the LEF file.
   *
   * @param data The name of the macro.
   */
  virtual void
  macro_callback(const char* data) {};

  /**
   * @brief Callback function invoked when the size of a macro is parsed.
   *
   * @param data The size of the macro.
   */
  virtual void
  macro_size_callback(lefiNum data) {};

  /**
   * @brief Callback function invoked when a pin is parsed in the LEF file.
   *
   * @param data Pointer to the lefiPin structure containing pin data.
   */
  virtual void
  pin_callback(lefiPin* data) {};

  /**
   * @brief Callback function invoked when an obstruction is parsed.
   *
   * @param data Pointer to the lefiObstruction structure containing obstruction data.
   */
  virtual void
  obstruction_callback(lefiObstruction* data) {};

private:
  /** =============================== PRIVATE STATIC METHODS =================================== */

  /**
   * @brief Static callback function for units parsing.
   *
   * @param type The type of callback.
   * @param data Pointer to the lefiUnits structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_units_callback(lefrCallbackType_e type, lefiUnits* data, void* instance);

  /**
   * @brief Static callback function for layer parsing.
   *
   * @param type The type of callback.
   * @param data Pointer to the lefiLayer structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_layer_callback(lefrCallbackType_e type, lefiLayer* data, void* instance);

  /**
   * @brief Static callback function for macro parsing.
   *
   * @param type The type of callback.
   * @param data The macro name.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_macro_callback(lefrCallbackType_e type, const char* data, void* instance);

  /**
   * @brief Static callback function for macro size parsing.
   *
   * @param type The type of callback.
   * @param data The macro size.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_macro_size_callback(lefrCallbackType_e type, lefiNum data, void* instance);

  /**
   * @brief Static callback function for pin parsing.
   *
   * @param type The type of callback.
   * @param data Pointer to the lefiPin structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_pin_callback(lefrCallbackType_e type, lefiPin* data, void* instance);

  /**
   * @brief Static callback function for obstruction parsing.
   *
   * @param type The type of callback.
   * @param data Pointer to the lefiObstruction structure.
   * @param instance Pointer to the LEF instance.
   * @return Status code (0 for success or 2 for error).
   */
  static int32_t
  d_obstruction_callback(lefrCallbackType_e type, lefiObstruction* data, void* instance);
};

} // namespace lef

#endif