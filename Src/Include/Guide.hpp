#ifndef __GUIDE_HPP__
#define __GUIDE_HPP__

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "Include/Geometry.hpp"

namespace guide
{

struct Node
{
  std::size_t m_x;
  std::size_t m_y;
  std::size_t m_connections = 0;

  struct Hash
  {
    std::size_t
    operator()(const Node* node) const
    {
      static std::size_t prime = 0x9e3779b9;

      const std::size_t  a     = std::hash<std::size_t>{}(node->m_x);
      const std::size_t  b     = std::hash<std::size_t>{}(node->m_y);
      return a ^ (b + prime + (a << 6) + (a >> 2));
    }
  };

  struct Equal
  {
    bool
    operator()(const Node* lhs, const Node* rhs) const
    {
      return lhs->m_x == rhs->m_x && lhs->m_y == rhs->m_y;
    }
  };
};

struct Edge
{
  Node*        m_source;
  Node*        m_destination;
  types::Metal m_metal_layer;

  struct Hash
  {
    std::size_t
    operator()(const Edge& edge) const
    {
      static std::size_t prime            = 0x9e3779b9;

      const std::size_t  source_hash      = std::hash<Node*>{}(edge.m_source);
      const std::size_t  destination_hash = std::hash<Node*>{}(edge.m_destination);
      const std::size_t  metal_hash       = std::hash<types::Metal>{}(edge.m_metal_layer);
      return source_hash ^ (destination_hash + prime + (source_hash << 6) + (source_hash >> 2)) ^ (metal_hash + prime + (destination_hash << 6) + (destination_hash >> 2));
    }
  };

  struct Equal
  {
    bool
    operator()(const Edge& lhs, const Edge& rhs) const
    {
      return lhs.m_source == rhs.m_source && lhs.m_destination == rhs.m_destination && lhs.m_metal_layer == rhs.m_metal_layer;
    }
  };
};

struct Tree
{
  std::string                                        m_name;
  std::unordered_set<Node*, Node::Hash, Node::Equal> m_nodes;
  std::unordered_set<Edge, Edge::Hash, Edge::Equal> m_edges;
};

std::vector<Tree>
read(const std::filesystem::path& path);

void
cleanup(const std::vector<Tree>& trees);

} // namespace

#endif