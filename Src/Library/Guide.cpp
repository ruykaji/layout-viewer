#include <fstream>
#include <iostream>
#include <unordered_set>

#include "Include/Guide.hpp"
#include "Include/GlobalUtils.hpp"

namespace guide
{

std::vector<Tree>
read(const std::filesystem::path& path)
{
  if(!std::filesystem::exists(path) || path.extension() != ".guide")
    {
      throw std::runtime_error("Guide Error: Can't locate guide file by the path - \"" + path.string() + "\".");
    }

  std::ifstream in_file(path);

  if(!in_file.is_open() || !in_file.good())
    {
      throw std::runtime_error("Guide Error: Can't open the file - \"" + path.string() + "\".");
    }

  std::string       line;
  std::vector<Tree> trees;

  Tree*             current_tree = nullptr;

  while(std::getline(in_file, line))
    {
      if(trees.empty())
        {
          current_tree = &trees.emplace_back(line);
          continue;
        }

      if(line == "(")
        {
          continue;
        }

      if(line == ")" && std::getline(in_file, line))
        {
          current_tree = &trees.emplace_back(line);
          continue;
        }

      if(line.empty())
        {
          break;
        }

      std::size_t  pos          = 0;
      std::size_t  next_pos     = line.find(' ');
      const double x1           = std::stod(line.substr(pos, next_pos - pos));

      pos                       = next_pos + 1;
      next_pos                  = line.find(' ', pos);
      const double y1           = std::stod(line.substr(pos, next_pos - pos));

      pos                       = next_pos + 1;
      next_pos                  = line.find(' ', pos);
      const double x2           = std::stod(line.substr(pos, next_pos - pos));

      pos                       = next_pos + 1;
      next_pos                  = line.find(' ', pos);
      const double       y2     = std::stod(line.substr(pos, next_pos - pos));

      const std::size_t  left   = std::min(x1, x2) / 6900;
      const std::size_t  top    = std::min(y1, y2) / 6900;
      const std::size_t  right  = std::max(x1, x2) / 6900;
      const std::size_t  bottom = std::max(y1, y2) / 6900;
      const types::Metal metal  = utils::get_skywater130_metal(line.substr(next_pos + 1));

      if(bottom - top == 1 && right - left == 1)
        {
          Node* new_node = new Node(left, top);

          if(current_tree->m_nodes.count(new_node) == 0)
            {
              current_tree->m_nodes.emplace(new_node);
            }
          else
            {
              delete new_node;
            }
        }

      if(bottom - top > 1)
        {
          Node* prev_node = nullptr;

          for(std::size_t y = top; y < bottom; ++y)
            {
              Node* new_node = new Node(left, y);

              if(current_tree->m_nodes.count(new_node) == 0)
                {
                  current_tree->m_nodes.emplace(new_node);
                }
              else
                {
                  Node* tmp = *current_tree->m_nodes.find(new_node);
                  delete new_node;
                  new_node = tmp;
                }

              if(prev_node != nullptr)
                {
                  auto [_, is_inserted] = current_tree->m_edges.emplace(prev_node, new_node, metal);

                  if(is_inserted)
                    {
                      ++prev_node->m_connections;
                      ++new_node->m_connections;
                    }
                }

              prev_node = new_node;
            }
        }

      if(right - left > 1)
        {
          Node* prev_node = nullptr;

          for(std::size_t x = left; x < right; ++x)
            {
              Node* new_node = new Node(x, top);

              if(current_tree->m_nodes.count(new_node) == 0)
                {
                  current_tree->m_nodes.emplace(new_node);
                }
              else
                {
                  Node* tmp = *current_tree->m_nodes.find(new_node);
                  delete new_node;
                  new_node = tmp;
                }

              if(prev_node != nullptr)
                {
                  auto [_, is_inserted] = current_tree->m_edges.emplace(prev_node, new_node, metal);

                  if(is_inserted)
                    {
                      ++prev_node->m_connections;
                      ++new_node->m_connections;
                    }
                }

              prev_node = new_node;
            }
        }
    }

  return trees;
}

void
cleanup(const std::vector<Tree>& trees)
{
  for(auto tree : trees)
    {
      for(auto node : tree.m_nodes)
        {
          delete node;
        }
    }
}

} // namespace guide
