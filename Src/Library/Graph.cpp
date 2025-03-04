#include <algorithm>
#include <numeric>
#include <queue>
#include <unordered_map>

#include "Include/Graph.hpp"

namespace graph
{

bool
operator>(const Edge& lhs, const Edge& rhs)
{
  return lhs.m_weight > rhs.m_weight;
}

bool
operator<(const Edge& lhs, const Edge& rhs)
{
  return !(lhs > rhs);
}

bool
operator==(const Edge& lhs, const Edge& rhs)
{
  return lhs.m_source == rhs.m_source && rhs.m_destination == lhs.m_destination;
}

void
Graph::place_node()
{
  m_adj.push_back({});
}

void
Graph::add_edge(uint32_t weight, uint32_t source, uint32_t destination)
{
  auto&      source_v = m_adj.at(source);
  const Edge source_edge{ weight, source, destination };

  if(std::find(source_v.begin(), source_v.end(), source_edge) == source_v.end())
    {
      m_adj[source].emplace_back(std::move(source_edge));
    }

  auto&      destination_v = m_adj.at(destination);
  const Edge destination_edge{ weight, destination, source };

  if(std::find(destination_v.begin(), destination_v.end(), destination_edge) == destination_v.end())
    {
      m_adj[destination].emplace_back(std::move(destination_edge));
    }
}

void
Graph::add_cost(uint32_t cost, uint32_t source, uint32_t destination)
{
  std::vector<graph::Edge>& source_v   = m_adj.at(source);
  auto                      source_itr = std::find_if(source_v.begin(), source_v.end(), [destination](const graph::Edge& edge) { return destination == edge.m_destination; });
  source_itr.base()->m_weight += cost;

  std::vector<graph::Edge>& destination_v   = m_adj.at(destination);
  auto                      destination_itr = std::find_if(destination_v.begin(), destination_v.end(), [source](const graph::Edge& edge) { return source == edge.m_destination; });
  destination_itr.base()->m_weight += cost;
}

void
Graph::zero_cost(uint32_t source, uint32_t destination)
{
  std::vector<graph::Edge>& source_v        = m_adj.at(source);
  auto                      source_itr      = std::find_if(source_v.begin(), source_v.end(), [destination](const graph::Edge& edge) { return destination == edge.m_destination; });
  source_itr.base()->m_weight               = 0;

  std::vector<graph::Edge>& destination_v   = m_adj.at(destination);
  auto                      destination_itr = std::find_if(destination_v.begin(), destination_v.end(), [source](const graph::Edge& edge) { return source == edge.m_destination; });
  destination_itr.base()->m_weight          = 0;
}

void
Graph::restore_cost(uint32_t source, uint32_t destination)
{
  std::vector<graph::Edge>& source_v        = m_adj.at(source);
  auto                      source_itr      = std::find_if(source_v.begin(), source_v.end(), [destination](const graph::Edge& edge) { return destination == edge.m_destination; });
  source_itr.base()->m_weight               = source_itr.base()->m_base_cost;

  std::vector<graph::Edge>& destination_v   = m_adj.at(destination);
  auto                      destination_itr = std::find_if(destination_v.begin(), destination_v.end(), [source](const graph::Edge& edge) { return source == edge.m_destination; });
  destination_itr.base()->m_weight          = destination_itr.base()->m_base_cost;
}

void
Graph::update_usage(int32_t delta, uint32_t source, uint32_t destination)
{
  std::vector<graph::Edge>& source_v   = m_adj.at(source);
  auto                      source_itr = std::find_if(source_v.begin(), source_v.end(), [destination](const graph::Edge& edge) { return destination == edge.m_destination; });
  source_itr.base()->m_usage += delta;

  std::vector<graph::Edge>& destination_v   = m_adj.at(destination);
  auto                      destination_itr = std::find_if(destination_v.begin(), destination_v.end(), [source](const graph::Edge& edge) { return source == edge.m_destination; });
  destination_itr.base()->m_usage += delta;
}

void
Graph::remove_cost(uint32_t cost, uint32_t source, uint32_t destination)
{
  std::vector<graph::Edge>& source_v   = m_adj.at(source);
  auto                      source_itr = std::find_if(source_v.begin(), source_v.end(), [destination](const graph::Edge& edge) { return destination == edge.m_destination; });
  source_itr.base()->m_weight -= cost;

  std::vector<graph::Edge>& destination_v   = m_adj.at(destination);
  auto                      destination_itr = std::find_if(destination_v.begin(), destination_v.end(), [source](const graph::Edge& edge) { return source == edge.m_destination; });
  destination_itr.base()->m_weight -= cost;
}

std::vector<std::vector<Edge>>&
Graph::get_adj()
{
  return m_adj;
}

const std::vector<std::vector<Edge>>&
Graph::get_adj() const
{
  return m_adj;
}

} // namespace graph
