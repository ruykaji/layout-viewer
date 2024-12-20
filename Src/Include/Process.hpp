#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <array>

#include "Include/DEF.hpp"
#include "Include/Guide.hpp"
#include "Include/LEF.hpp"
#include "Include/Matrix.hpp"

namespace process
{

struct Task
{
  uint8_t                           m_wip_idx;

  std::vector<std::vector<uint8_t>> m_nets;     /** From 0 to N in this task. Points in net are (x = i, y = i + 1, z = i + 2) */
  std::vector<matrix::Matrix>       m_matrices; /** These matrices are not encoded */

  std::pair<matrix::Matrix, std::vector<std::vector<uint8_t>>>
  encode_wip(const uint8_t max_layers);

  void
  set_value(const std::string& name, const uint8_t value, const uint8_t x, const uint8_t y, const uint8_t z);
};

struct TaskBatch
{
  def::GCell*              m_gcell;

  std::vector<std::string> m_net_to_idx; /** From 0 to N in corresponding gcell */
  std::vector<Task>        m_task;
};

void
apply_global_routing(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets);

std::vector<Task>
encode(def::Data& def_data);

} // namespace process

#endif