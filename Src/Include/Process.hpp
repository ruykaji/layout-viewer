#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include "Include/DEF.hpp"
#include "Include/Guide.hpp"
#include "Include/LEF.hpp"
#include "Include/Matrix.hpp"

namespace process
{

struct Task
{
  /** Matrix coordinates */
  std::size_t                                                                         m_idx_x;
  std::size_t                                                                         m_idx_y;

  /** Data */
  matrix::Matrix                                                                      m_matrix;
  std::unordered_map<std::string, std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>> m_nets;
};

void
apply_global_routing(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets);

std::vector<std::vector<Task>>
encode(def::Data& def_data);

} // namespace process

#endif