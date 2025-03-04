#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace py = pybind11;

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------

// A simple point structure (with a layer field).
struct Point
{
  int x, y, layer;

  Point(int x, int y, int z)
      : x(x), y(y), layer(z)
  {
  }

  Point(std::tuple<int, int, int> tuple)
  {
    x     = std::get<0>(tuple);
    y     = std::get<1>(tuple);
    layer = std::get<2>(tuple);
  }

  bool
  operator==(const Point& other) const
  {
    return x == other.x && y == other.y && layer == other.layer;
  }
};

struct PointHash
{
  std::size_t
  operator()(const Point& p) const
  {
    std::size_t hx = std::hash<int>()(p.x);
    std::size_t hy = std::hash<int>()(p.y);
    std::size_t hl = std::hash<int>()(p.layer);
    return hx ^ (hy << 1) ^ (hl << 2);
  }
};

// Here we use double because the Python arrays are of type double.
// (If you prefer integers, you can change the function signature to
//  py::array_t<int> and adjust accordingly.)
using PathMatrix    = std::vector<std::vector<double>>;
using Netlist       = std::vector<std::tuple<int, int, int>>;

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

// Returns true if (x,y) is within the grid of dimensions width x height.
bool
is_within_bounds(int x, int y, int width, int height)
{
  return (x >= 0 && x < width && y >= 0 && y < height);
}

// Given a point, returns its valid neighbors according to the path and via matrices.
//
// IMPORTANT: We assume that the C++ path matrix is stored in row‐major order,
// that is, as a vector of rows (with number of rows equal to “height”)
// and each row is a vector of “width” elements. In particular, an element is accessed
// as path_matrix[y][x] (with y in [0,height) and x in [0,width)).
std::vector<Point>
get_neighbors(const Point& p, const PathMatrix& path_matrix)
{
  std::vector<Point> neighbors;
  // Here the outer vector size is the number of rows (i.e. height)
  // and the inner vector size is the number of columns (i.e. width).
  int                height = static_cast<int>(path_matrix.size());
  int                width  = static_cast<int>(path_matrix[0].size());

  if(p.layer == 0)
    {
      if(is_within_bounds(p.x - 1, p.y, width, height) && path_matrix[p.y][p.x - 1] != 0.0)
        {
          neighbors.push_back({ p.x - 1, p.y, 0 });
        }

      if(is_within_bounds(p.x + 1, p.y, width, height) && path_matrix[p.y][p.x + 1] != 0.0)
        {
          neighbors.push_back({ p.x + 1, p.y, 0 });
        }
    }

  if(p.layer == 1)
    {
      if(is_within_bounds(p.x, p.y - 1, width, height) && path_matrix[p.y - 1][p.x] != 0.0)
        {
          neighbors.push_back({ p.x, p.y - 1, 1 });
        }

      if(is_within_bounds(p.x, p.y + 1, width, height) && path_matrix[p.y + 1][p.x] != 0.0)
        {
          neighbors.push_back({ p.x, p.y + 1, 1 });
        }
    }

  if(path_matrix[p.y][p.x] == 2)
    {
      neighbors.push_back({ p.x, p.y, 1 - p.layer });
    }

  return neighbors;
}

// For each net, perform a breadth-first search (BFS) starting at the first terminal,
// check that all terminals are reached, and also detect if a cycle exists in the net's
// routing. (If a cycle is detected, then the net's score is not increased.)
double
traverse_and_check(const Netlist& terminals, const PathMatrix& path_matrix)
{
  double score = 0.0;

  if(terminals.empty())
    {
      return 0.0;
    }

  // Start at the first terminal.
  Point                                       start(std::get<0>(terminals[0]),
                                                    std::get<1>(terminals[0]),
                                                    std::get<2>(terminals[0]));

  // Instead of a simple visited set, we record the parent of each visited node.
  std::unordered_map<Point, Point, PointHash> parent;
  std::queue<Point>                           q;

  // Use a sentinel value for the parent of the start.
  Point                                       sentinel(-1, -1, -1);
  parent.emplace(start, sentinel);
  q.push(start);

  bool cycle_found = false;

  // Modified BFS that tracks parents to detect cycles.
  while(!q.empty() && !cycle_found)
    {
      Point current = q.front();
      q.pop();

      for(const auto& neighbor : get_neighbors(current, path_matrix))
        {
          // If neighbor has not been visited, record its parent and enqueue it.
          if(parent.find(neighbor) == parent.end())
            {
              parent.emplace(neighbor, current);
              q.push(neighbor);
            }
          // If neighbor was visited and is not the parent of the current node,
          // then a cycle exists.
          else if(!(neighbor == parent.at(current)))
            {
              cycle_found = true;
              break;
            }
        }
    }

  // Check that every terminal is reached.
  bool connected = true;

  for(const auto& term_tuple : terminals)
    {
      Point term(std::get<0>(term_tuple),
                 std::get<1>(term_tuple),
                 std::get<2>(term_tuple));

      if(parent.find(term) == parent.end())
        {
          connected = false;
          break;
        }
    }

  // Only increase the score if the net is fully connected and no cycle was found.
  if(connected && !cycle_found)
    {
      score += 1.0;
    }

  return score;
}

// -----------------------------------------------------------------------------
// Main function: check_connectivity
// -----------------------------------------------------------------------------

std::tuple<double, double>
check_connectivity(const std::vector<std::vector<Netlist>>& netlist_batch,
                   const py::array_t<double>&               path_matrix_array)
{
  // Get a read-only view of the path matrix.
  // Expected shape: (batch, width, height)
  auto      path_buf = path_matrix_array.unchecked<4>();

  // Extract dimensions.
  const int batch    = static_cast<int>(path_buf.shape(0));
  const int nets     = static_cast<int>(path_buf.shape(1));
  const int height   = static_cast<int>(path_buf.shape(2));
  const int width    = static_cast<int>(path_buf.shape(3));

  // Check that the netlist batch size matches the batch dimension.
  if(netlist_batch.size() != static_cast<size_t>(batch))
    throw std::runtime_error("netlist batch size must match the number of batches in matrices.");

  double overall_result = 0.0;
  double general_result = 0.0;

  {
    py::gil_scoped_release release;

// Parallelize over the batch dimension.
#pragma omp parallel for schedule(dynamic) reduction(+ : overall_result, general_result)
    for(int b = 0; b < batch; b++)
      {
        // --- Build the PathMatrix for batch 'b' ---
        // We now build a row-major matrix with dimensions: height x width.
        PathMatrix cpp_path_matrix = std::vector<std::vector<double>>(height, std::vector<double>(width));

        for(int y = 0; y < height; y++)
          {
            for(int x = 0; x < width; x++)
              {
                for(int z = 0; z < nets; ++z)
                  {
                    // Note: path_buf(b, x, y) uses x as the second dimension and y as the third.
                    cpp_path_matrix[y][x] = path_buf(b, 0 y, x);
                  }
              }
          }

        const auto nets_batch = netlist_batch.at(b);
        double     tmp_result = 0.0;

        for(int z = 0; z < nets_batch.size(); ++z)
          {
            // --- Get the netlist for batch 'b' ---
            const Netlist& netlist_for_batch = nets_batch.at(z);

            // --- Perform connectivity check with cycle detection ---
            tmp_result += traverse_and_check(netlist_for_batch, cpp_path_matrix);
          }

        double result = (tmp_result / double(nets_batch.size()));

        overall_result += result;
        general_result += result == 1.0 ? 1.0 : 0.0;
      }
  } // GIL automatically reacquired here.

  return std::make_tuple(overall_result / netlist_batch.size(), general_result / netlist_batch.size());
}

PYBIND11_MODULE(net_connectivity, m)
{
  m.doc() = "Module for checking netlist connectivity using routing matrices, with cycle detection.";
  m.def("check_connectivity", &check_connectivity,
        "Check whether each net in the netlist is fully connected given the routing (path) and via matrices. "
        "A net that contains a cycle is not counted towards the score.",
        py::arg("netlist"),
        py::arg("path_matrix"));
}
