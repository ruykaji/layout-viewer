// #include <algorithm>
// #include <functional>
// #include <iostream>
// #include <numeric>
// #include <queue>
// #include <set>
// #include <thread>
// #include <unordered_map>

// #include "Include/Algorithms.hpp"

// template <>
// struct std::hash<graph::Edge>
// {
//   std::size_t
//   operator()(const graph::Edge& edge) const noexcept
//   {
//     return std::hash<std::string>{}(std::to_string(edge.m_source) + "_" + std::to_string(edge.m_destination) + "_" + std::to_string(edge.m_weight));
//   }
// };

// namespace algorithms
// {

// namespace details
// {

// class UnionFind
// {
// private:
//   std::vector<uint32_t> parent;
//   std::vector<uint32_t> rank;

// public:
//   UnionFind(std::size_t n)
//   {
//     parent.resize(n);
//     rank.resize(n, 0);
//     std::iota(parent.begin(), parent.end(), 0);
//   }

//   uint32_t
//   find(uint32_t u)
//   {
//     while(u != parent[u])
//       {
//         parent[u] = parent[parent[u]];
//         u         = parent[u];
//       }

//     return u;
//   }

//   bool
//   union_sets(uint32_t u, uint32_t v)
//   {
//     uint32_t root_u = find(u);
//     uint32_t root_v = find(v);

//     if(root_u != root_v)
//       {
//         if(rank[root_u] < rank[root_v])
//           {
//             parent[root_u] = root_v;
//           }
//         else if(rank[root_u] > rank[root_v])
//           {
//             parent[root_v] = root_u;
//           }
//         else
//           {
//             parent[root_v] = root_u;
//             ++rank[root_u];
//           }

//         return true;
//       }

//     return false;
//   }

//   bool
//   connected(uint32_t u, uint32_t v)
//   {
//     return find(u) == find(v);
//   }

//   bool
//   connected(const std::vector<uint32_t>& n)
//   {
//     const uint32_t s = find(n[0]);

//     for(std::size_t i = 1, end = n.size(); i < end; ++i)
//       {
//         if(s != find(n[i]))
//           {
//             return false;
//           }
//       }

//     return true;
//   }

//   void
//   reset()
//   {
//     std::fill(rank.begin(), rank.end(), 0);
//     std::iota(parent.begin(), parent.end(), 0);
//   }
// };

// /**
//  * @brief Handles information about single source to single destination path.
//  *
//  */
// struct PathInfo
// {
//   uint32_t                 m_weight;
//   uint32_t                 m_source;
//   uint32_t                 m_destination;
//   std::vector<graph::Edge> m_path;
// };

// // std::unordered_map<graph::Edge, std::vector<uint32_t>>
// // all_paths_dijkstra(const std::unordered_set<uint32_t>& terminals, const std::vector<std::vector<graph::Edge>>& adj, const std::size_t paths_count)
// // {
// //   const std::vector<uint32_t>                            terminals_v(terminals.begin(), terminals.end());
// //   const std::size_t                                      num_vertices  = adj.size();
// //   const std::size_t                                      num_terminals = terminals.size();

// //   std::unordered_map<graph::Edge, std::vector<uint32_t>> merge_collection;
// //   uint32_t                                               path_counter = 0;

// //   for(uint32_t i = 0; i < num_terminals; ++i)
// //     {
// //       const uint32_t                     src = terminals_v[i];

// //       std::vector<uint32_t>              dist(num_vertices, std::numeric_limits<uint32_t>::max());
// //       std::vector<std::vector<uint32_t>> prev(num_vertices);

// //       dist[src - 1] = 0;

// //       std::priority_queue<graph::Edge, std::vector<graph::Edge>, std::greater<graph::Edge>> queue;
// //       queue.emplace(0, src);

// //       while(!queue.empty())
// //         {
// //           const uint32_t u      = queue.top().m_source;
// //           const uint32_t dist_u = queue.top().m_destination;
// //           queue.pop();

// //           if(dist_u > dist[u - 1])
// //             {
// //               continue;
// //             }

// //           for(const auto& edge : adj[u - 1])
// //             {
// //               const uint32_t v   = edge.m_destination;
// //               const uint32_t alt = dist[u - 1] + edge.m_weight;

// //               if(alt < dist[v - 1])
// //                 {
// //                   dist[v - 1] = alt;
// //                   prev[v - 1].clear();
// //                   prev[v - 1].push_back(u);
// //                   queue.emplace(alt, v);
// //                 }
// //               else if(alt == dist[v - 1])
// //                 {
// //                   prev[v - 1].push_back(u);
// //                 }
// //             }
// //         }

// //       for(uint32_t j = i + 1; j < num_terminals; ++j)
// //         {
// //           const uint32_t dst = terminals_v[j];

// //           if(prev[dst - 1].empty())
// //             {
// //               continue;
// //             }

// //           std::queue<std::vector<uint32_t>> prev_queue;
// //           prev_queue.push({ dst });

// //           ++path_counter;

// //           while(!prev_queue.empty())
// //             {
// //               auto current_path = std::move(prev_queue.front());
// //               prev_queue.pop();

// //               const uint32_t current_node = current_path.back();

// //               if(current_node == src)
// //                 {
// //                   for(std::size_t k = 0, end = current_path.size() - 1; k < end; ++k)
// //                     {
// //                       graph::Edge new_edge{ 0, current_path[k + 1], current_path[k] };

// //                       const auto& connections = adj.at(new_edge.m_source - 1);
// //                       auto        it          = std::find_if(connections.begin(), connections.end(), [destination = new_edge.m_destination](const graph::Edge& edge) { return edge.m_destination == destination; });

// //                       new_edge.m_weight       = it.base()->m_weight;

// //                       if(merge_collection.count(new_edge) == 0)
// //                         {
// //                           merge_collection[new_edge].resize(paths_count, 0);
// //                         }

// //                       merge_collection[new_edge][path_counter - 1] = 1;
// //                     }
// //                 }
// //               else
// //                 {
// //                   for(const uint32_t prev_node : prev[current_node - 1])
// //                     {
// //                       std::vector<uint32_t> new_path = current_path;
// //                       new_path.push_back(prev_node);
// //                       prev_queue.push(std::move(new_path));
// //                     }
// //                 }
// //             }
// //         }
// //     }

// //   return merge_collection;
// // }

// } // namespace details

// std::vector<graph::Edge>
// dijkstra_kruskal(const graph::Graph& graph)
// {
//   // struct PathInfo
//   // {
//   //   uint32_t                 total_weight;
//   //   uint32_t                 source;
//   //   uint32_t                 destination;
//   //   std::vector<graph::Edge> full_path;
//   // };

//   // const auto&                           adj       = graph.get_adj();
//   // const auto&                           terminals = graph.get_terminals();
//   // const std::vector<uint32_t>           terminals_v(terminals.begin(), terminals.end());
//   // const std::size_t                     num_vertices  = adj.size();
//   // const std::size_t                     num_terminals = terminals.size();

//   // /** Number of sort and full paths have to be equal */
//   // std::vector<PathInfo>                 paths_info;
//   // std::vector<graph::Edge>              short_paths;
//   // std::vector<std::vector<graph::Edge>> full_paths;

//   // for(uint32_t i = 0; i < num_terminals; ++i)
//   //   {
//   //     const uint32_t                                                                        src     = terminals_v[i];

//   //     uint32_t                                                                              visited = 0;
//   //     std::vector<uint32_t>                                                                 dist(num_vertices, std::numeric_limits<uint32_t>::max());
//   //     std::vector<uint32_t>                                                                 weights(num_vertices, 0);
//   //     std::vector<uint32_t>                                                                 prev(num_vertices, 0);
//   //     std::priority_queue<graph::Edge, std::vector<graph::Edge>, std::greater<graph::Edge>> queue;

//   //     dist[src - 1] = 0;
//   //     queue.emplace(0, src);

//   //     while(!queue.empty())
//   //       {
//   //         const uint32_t u = queue.top().m_source;
//   //         queue.pop();

//   //         for(const auto& edge : adj[u - 1])
//   //           {
//   //             uint32_t v   = edge.m_destination;
//   //             uint32_t alt = dist[u - 1] + edge.m_weight;

//   //             if(alt < dist[v - 1])
//   //               {
//   //                 dist[v - 1]    = alt;
//   //                 prev[v - 1]    = u;
//   //                 weights[v - 1] = edge.m_weight;
//   //                 queue.emplace(alt, v);

//   //                 if(terminals.find(v) != terminals.end())
//   //                   {
//   //                     ++visited;
//   //                   }

//   //                 if(visited == num_terminals)
//   //                   {
//   //                     break;
//   //                   }
//   //               }
//   //           }

//   //         if(visited == num_terminals)
//   //           {
//   //             break;
//   //           }
//   //       }

//   //     /** Get short and full paths */
//   //     for(uint32_t j = i + 1; j < num_terminals; ++j)
//   //       {
//   //         uint32_t                 dst = terminals_v[j];
//   //         std::vector<graph::Edge> full_path;

//   //         for(uint32_t at = dst; at != src; at = prev[at - 1])
//   //           {
//   //             const uint32_t from   = prev[at - 1];
//   //             const uint32_t weight = weights[at - 1];

//   //             full_path.emplace_back(weight, from, at);
//   //           }

//   //         std::reverse(full_path.begin(), full_path.end());
//   //         paths_info.push_back({ dist[dst - 1], src, dst, std::move(full_path) });
//   //       }
//   //   }

//   // /** Using Kruskal's algorithm find MST in short paths */
//   // std::sort(paths_info.begin(), paths_info.end(), [](const PathInfo& a, const PathInfo& b) { return a.total_weight < b.total_weight; });

//   // std::vector<graph::Edge> mst_edges;
//   // details::UnionFind       uf(adj.size());

//   // for(const auto& path_info : paths_info)
//   //   {
//   //     if(uf.union_sets(path_info.source, path_info.destination))
//   //       {
//   //         mst_edges.insert(mst_edges.end(), path_info.full_path.begin(), path_info.full_path.end());
//   //       }
//   //   }

//   // std::sort(mst_edges.begin(), mst_edges.end(), [](const graph::Edge& a, const graph::Edge& b) { return a.m_weight < b.m_weight; });
//   // mst_edges.erase(std::unique(mst_edges.begin(), mst_edges.end(), [](const graph::Edge& a, const graph::Edge& b) { return a.m_source == b.m_source && a.m_destination == b.m_destination; }), mst_edges.end());

//   // /** Using Kruskal's algorithm find MST in full paths from short path mst edges*/
//   // std::sort(mst_edges.begin(), mst_edges.end(), [](const graph::Edge& a, const graph::Edge& b) { return a.m_weight < b.m_weight; });
//   // uf.reset();

//   // std::vector<graph::Edge> final_mst;

//   // for(const auto& edge : mst_edges)
//   //   {
//   //     if(uf.union_sets(edge.m_source, edge.m_destination))
//   //       {
//   //         final_mst.emplace_back(edge.m_weight, edge.m_source, edge.m_destination);
//   //       }
//   //   }

//   // return final_mst;
// }

// } // namespace algorithms
