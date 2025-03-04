#ifndef __GRAPH_HPP__
#define __GRAPH_HPP__

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace graph
{

struct Edge
{
  uint32_t m_weight;
  uint32_t m_source;
  uint32_t m_destination;

  uint32_t m_base_cost;
  uint32_t m_usage    = 0;
  uint32_t m_capacity = 1;

  double   m_history  = 0.0;

  friend bool
  operator>(const Edge& lhs, const Edge& rhs);

  friend bool
  operator<(const Edge& lhs, const Edge& rhs);

  friend bool
  operator==(const Edge& lhs, const Edge& rhs);

  Edge(uint32_t weight, uint32_t source, uint32_t destination)
      : m_weight(weight), m_source(source), m_destination(destination), m_base_cost(weight)
  {
  }
};

class Graph
{
public:
  void
  place_node();

  void
  add_edge(uint32_t weight, uint32_t source, uint32_t destination);

  void
  add_cost(uint32_t cost, uint32_t source, uint32_t destination);

  void
  zero_cost(uint32_t source, uint32_t destination);

  void
  restore_cost(uint32_t source, uint32_t destination);

  void
  update_usage(int32_t delta, uint32_t source, uint32_t destination);

  void
  remove_cost(uint32_t cost, uint32_t source, uint32_t destination);

  std::vector<std::vector<Edge>>&
  get_adj();

  const std::vector<std::vector<Edge>>&
  get_adj() const;

private:
  std::vector<std::vector<Edge>> m_adj;
};

} // namespace graph

#endif