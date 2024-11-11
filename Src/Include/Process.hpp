#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include "Include/DEF.hpp"
#include "Include/Guide.hpp"
#include "Include/LEF.hpp"

namespace process
{

void
fill_gcells(def::Data& def_data, const lef::Data& lef_data, const std::vector<guide::Net>& nets);

} // namespace process

#endif