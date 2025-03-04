#ifndef __NET_HPP__
#define __NET_HPP__

#include <string>
#include <vector>

namespace def
{

struct Net
{
  std::size_t                     m_idx;
  std::string                     m_name;
  std::unordered_set<std::string> m_pins;

  struct HashPtr
  {
    std::size_t
    operator()(const Net* net) const
    {
      return std::hash<std::string>{}(net->m_name);
    }
  };
};

} // namespace def

#endif