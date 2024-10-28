#ifndef __PDK_HPP__
#define __PDK_HPP__

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "LEF.hpp"
#include "Types.hpp"

namespace pdk
{

struct Layer
{
  enum class Type
  {
    ROUTING = 0,
    CUT,
  } m_type;

  enum class Direction
  {
    HORIZONTAL = 0,
    VERTICAL
  } m_direction;

  double m_width;
  double m_pitch_x;
  double m_pitch_y;
  double m_offset;
  double m_spacing;
};

struct Pin
{
  struct Port
  {
    types::Metal     m_metal;
    types::Rectangle m_rect;
  };

  enum class Use
  {
    SIGNAL = 0,
    ANALOG,
    CLOCK,
    GROUND,
    POWER,
    RESET,
    SCAN,
    TIEOFF
  } m_use;

  enum class Direction
  {
    FEEDTHRU = 0,
    INPUT,
    OUTPUT,
    INOUT,
  } m_direction;

  std::string       m_name;
  std::vector<Port> m_ports;

  /**
   * @brief Sets a direction to the pin.
   *
   * @param pin The Pin.
   * @param direction The direction.
   */
  static void
  set_direction(Pin& pin, const std::string_view direction);

  /**
   * @brief Sets a use to the pin.
   *
   * @param pin The pin.
   * @param use The use.
   */
  static void
  set_use(Pin& pin, const std::string_view use);
};

struct Macro
{
  double                        m_width;
  double                        m_height;
  std::vector<Pin>              m_pins;
  std::vector<types::Rectangle> m_geometry;
};

struct PDK
{
  double                                  m_database_number;
  std::unordered_map<types::Metal, Layer> m_layers;
  std::unordered_map<std::string, Macro>  m_macros;
};

class Reader : public lef::LEF
{
public:
  Reader() : lef::LEF(), m_last_macro_name() {};

  /**
   * @brief Compiles all LEF files into single PDK.
   *
   * @param dir_path The path to LEF files.
   * @return PDK
   */
  PDK
  compile(const std::filesystem::path& dir_path) const;

protected:
  virtual void
  units_callback(lefiUnits* param, void* data) const override;

  virtual void
  layer_callback(lefiLayer* param, void* data) const override;

  virtual void
  macro_begin_callback(const char* param, void* data) const override;

  virtual void
  macro_size_callback(lefiNum param, void* data) const override;

  virtual void
  pin_callback(lefiPin* param, void* data) const override;

  virtual void
  obstruction_callback(lefiObstruction* param, void* data) const override;

private:
  mutable std::string m_last_macro_name;
};

} // namespace pdk

#endif