#ifndef __DEF_HPP__
#define __DEF_HPP__

#include <cstdint>
#include <filesystem>
#include <unordered_map>

#include <defrReader.hpp>

#include "Include/Pin.hpp"
#include "Include/Types.hpp"

namespace def
{

struct RowTemplate
{
  double   m_x;
  double   m_step_x;
  uint32_t m_num_x;

  double   m_y;
  double   m_step_y;
  uint32_t m_num_y;
};

struct TrackTemplate
{
  double       m_start;
  double       m_spacing;
  uint32_t     m_num;
  types::Metal m_metal;
};

struct GCellGridTemplate
{
  double   m_start;
  double   m_step;
  uint32_t m_num;
};

struct ComponentTemplate
{
  std::string        m_id;
  std::string        m_name;
  uint32_t           m_x;
  uint32_t           m_y;
  types::Orientation m_orientation;
};

struct Net
{
  std::size_t              m_idx;
  std::vector<std::string> m_pins;
};

struct GCell
{
  struct Track
  {
    types::Rectangle m_box;
    types::Metal     m_metal;

    std::size_t      m_ln = 0;
    std::size_t      m_rn = 0;
  };

  types::Rectangle                                       m_box;
  std::vector<Track>                                     m_tracks_x;
  std::vector<Track>                                     m_tracks_y;
  std::vector<std::pair<types::Rectangle, types::Metal>> m_obstacles;

  std::unordered_map<std::string, pin::Pin*>             m_pins;
  std::unordered_map<std::string, Net>                   m_nets;

  static std::vector<std::pair<GCell*, types::Rectangle>>
  find_overlaps(const types::Rectangle& rect, const std::vector<std::vector<GCell*>>& gcells, const uint32_t width, const uint32_t height);
};

struct Data
{
  std::array<uint32_t, 4UL>                              m_box;
  std::vector<ComponentTemplate>                         m_components;
  std::vector<std::pair<types::Rectangle, types::Metal>> m_obstacles;

  std::unordered_map<std::string, pin::Pin*>             m_pins;
  std::unordered_map<std::string, Net>                   m_nets;

  std::size_t                                            m_max_gcell_x;
  std::size_t                                            m_max_gcell_y;
  std::vector<std::vector<GCell*>>                       m_gcells;
};

class DEF
{
public:
  /** =============================== CONSTRUCTORS ============================================ */

  /**
   * @brief Constructs a new DEF parser object.
   *
   */
  DEF();

  /**
   * @brief Destroys the DEF parser object.
   *
   */
  virtual ~DEF();

public:
  /** =============================== PUBLIC METHODS ==================================== */

  /**
   * @brief Parses a DEF file from the given file path.
   *
   * @param file_path The path to the DEF file to be parsed.
   */
  Data
  parse(const std::filesystem::path& file_path) const;

private:
  /** =============================== PRIVATE METHODS =================================== */

  void
  die_area_callback(defiBox* param, Data& data);

  void
  row_callback(defiRow* param, Data& data);

  void
  track_callback(defiTrack* param, Data& data);

  void
  gcell_callback(defiGcellGrid* param, Data& data);

  void
  component_start_callback(int32_t param, Data& data);

  void
  component_callback(defiComponent* param, Data& data);

  void
  pin_callback(defiPin* param, Data& data);

  void
  net_callback(defiNet* param, Data& data);

private:
  /** =============================== PRIVATE STATIC METHODS =================================== */

  /**
   * @brief Prints internal defrRead function error.
   *
   * @param msg Error message.
   */
  static void
  error_callback(const char* msg);

  static int32_t
  d_die_area_callback(defrCallbackType_e type, defiBox* param, void* instance);

  static int32_t
  d_row_callback(defrCallbackType_e type, defiRow* param, void* instance);

  static int32_t
  d_track_callback(defrCallbackType_e type, defiTrack* param, void* instance);

  static int32_t
  d_gcell_callback(defrCallbackType_e type, defiGcellGrid* param, void* instance);

  static int32_t
  d_component_start_callback(defrCallbackType_e type, int32_t param, void* instance);

  static int32_t
  d_component_callback(defrCallbackType_e type, defiComponent* param, void* instance);

  static int32_t
  d_pin_callback(defrCallbackType_e type, defiPin* param, void* instance);

  static int32_t
  d_net_callback(defrCallbackType_e type, defiNet* param, void* instance);

  static int32_t
  d_special_net_callback(defrCallbackType_e type, defiNet* param, void* instance);

private:
  /** Temporary containers for data that used only while parsing and never after */
  std::vector<RowTemplate>       m_rows;
  std::vector<TrackTemplate>     m_tracks_x;
  std::vector<TrackTemplate>     m_tracks_y;
  std::vector<GCellGridTemplate> m_gcell_grid_x;
  std::vector<GCellGridTemplate> m_gcell_grid_y;

  /** All essential extracted data */
  Data                           m_data;
};

} // namespace def

#endif
