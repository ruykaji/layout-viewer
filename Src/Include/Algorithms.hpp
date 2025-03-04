#ifndef __ALGORITHMS_HPP__
#define __ALGORITHMS_HPP__

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <vector>

#include "Include/Geometry.hpp"
#include "Include/Graph.hpp"
#include "Include/Matrix.hpp"

namespace algorithms
{

class AStar
{
  struct Node
  {
    uint32_t m_ref;
    double   m_g      = 0.0;
    double   m_h      = 0.0;
    double   m_f      = 0.0;
    Node*    m_parent = nullptr;

    Node(const uint32_t ref, const double g, const double h, Node* parent = nullptr)
        : m_ref(ref), m_g(g), m_h(h), m_f(g + h), m_parent(parent) {};

    struct Compare
    {
      bool
      operator()(const Node* lhs, const Node* rhs) const
      {
        return lhs->m_f > rhs->m_f;
      };
    };
  };

public:
  AStar(const graph::Graph& graph, const matrix::SetOfNodes& obs, const std::vector<matrix::Node>& nodes)
      : m_graph(graph), m_obs(obs), m_nodes(nodes)
  {
    m_obstacle_cost = std::numeric_limits<uint32_t>::max() / (m_graph.get_adj().size() / 2);
  };

public:
  std::vector<graph::Edge>
  multi_terminal_path(const std::unordered_set<uint32_t>& terminals)
  {
    std::vector<graph::Edge>     result_path;

    std::unordered_set<uint32_t> visited_terminals;
    std::unordered_set<uint32_t> extended_terminals;

    while(true)
      {
        uint32_t goal           = 0;
        uint32_t start          = 0;
        double   best_heuristic = std::numeric_limits<double>::max();
        bool     goal_is_set    = false;

        for(const auto& current_goal : terminals)
          {
            if(visited_terminals.count(current_goal) != 0)
              {
                continue;
              }

            for(const auto& current_start : (extended_terminals.empty() ? terminals : extended_terminals))
              {
                if(current_start == current_goal)
                  {
                    continue;
                  }

                double h = heuristic(current_start, current_goal);

                if(h < best_heuristic)
                  {
                    start          = current_start;
                    goal           = current_goal;
                    best_heuristic = h;
                  }
              }

            goal_is_set = true;
          }

        if(!goal_is_set)
          {
            break;
          }

        const std::vector<graph::Edge> path = find_path(start, goal, terminals);

        if(path.empty())
          {
            return {};
          }

        for(const auto& edge : path)
          {
            extended_terminals.emplace(edge.m_source);
            visited_terminals.emplace(edge.m_source);

            extended_terminals.emplace(edge.m_destination);
            visited_terminals.emplace(edge.m_destination);
          }

        result_path.insert(result_path.end(), std::move_iterator(path.begin()), std::move_iterator(path.end()));
      }

    for(const auto& t : terminals)
      {
        if(extended_terminals.count(t) == 0)
          {
            return {};
          }
      }

    for(const auto& edge : result_path)
      {
        m_obs.emplace(m_nodes[edge.m_source]);
        m_obs.emplace(m_nodes[edge.m_destination]);
      }

    return result_path;
  }

private:
  std::vector<graph::Edge>
  find_path(const uint32_t start, const uint32_t goal, const std::unordered_set<uint32_t>& terminals)
  {
    const auto&                                                   adj = m_graph.get_adj();

    std::vector<uint8_t>                                          closed(m_nodes.size(), 0);
    std::vector<double>                                           g_score(m_nodes.size(), std::numeric_limits<double>::max());
    std::vector<Node*>                                            nodes_collection;

    std::priority_queue<Node*, std::vector<Node*>, Node::Compare> open_list;

    {
      Node* new_node = new Node(start, 0, heuristic(start, goal));
      open_list.emplace(new_node);
      nodes_collection.emplace_back(new_node);
    }

    g_score[start]  = 0;

    Node* goal_node = nullptr;

    while(!open_list.empty())
      {
        Node* current = open_list.top();
        open_list.pop();

        if(current->m_ref == goal)
          {
            goal_node = current;
            break;
          }

        if(closed.at(current->m_ref) == 1 || (m_obs.count(m_nodes.at(current->m_ref)) != 0 && terminals.count(current->m_ref) == 0))
          {
            continue;
          }

        closed[current->m_ref] = 1;

        for(const auto& edge : adj.at(current->m_ref))
          {
            if(closed.at(edge.m_destination) == 1 || (m_obs.count(m_nodes.at(edge.m_destination)) != 0 && terminals.count(edge.m_destination) == 0))
              {
                continue;
              }

            double tentative_g = current->m_g + 1;

            if(tentative_g < g_score.at(edge.m_destination))
              {
                g_score[edge.m_destination] = tentative_g;

                Node* new_node              = new Node(edge.m_destination, tentative_g, heuristic(edge.m_destination, goal), current);
                open_list.emplace(new_node);
                nodes_collection.emplace_back(new_node);
              }
          }
      }

    if(goal_node == nullptr)
      {
        for(const auto& node : nodes_collection)
          {
            delete node;
          }

        return {};
      }

    std::vector<graph::Edge> path;
    Node*                    current = goal_node;

    while(current->m_parent != nullptr)
      {
        path.emplace_back(0, current->m_ref, current->m_parent->m_ref);
        current = current->m_parent;
      }

    for(const auto& node : nodes_collection)
      {
        delete node;
      }

    return path;
  }

private:
  double
  heuristic(const uint32_t lhs, const uint32_t rhs) const
  {
    const auto&  lhs_node = m_nodes.at(lhs);
    const auto&  rhs_node = m_nodes.at(rhs);

    const double dx       = std::abs(double(lhs_node.m_x) - double(rhs_node.m_x));
    const double dy       = std::abs(double(lhs_node.m_y) - double(rhs_node.m_y));
    const double dz       = std::abs(double(lhs_node.m_z) - double(rhs_node.m_z));

    return dx + dy + dz;
  }

private:
  const graph::Graph&              m_graph;
  const std::vector<matrix::Node>& m_nodes;

  matrix::SetOfNodes               m_obs;
  double                           m_obstacle_cost;
};

} // namespace algorithms

#endif
