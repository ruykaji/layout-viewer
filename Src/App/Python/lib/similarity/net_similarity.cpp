#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_set>

namespace py = pybind11;

// A simple point structure
struct Point {
    double x;
    double y;
};

// Helper function to trim whitespace from a string.
std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        ++start;
    }
    auto end = s.end();
    while (end != start && std::isspace(*(end - 1))) {
        --end;
    }
    return std::string(start, end);
}

// Function to process the net file and extract coordinates.
// It mimics the Python function by reading all lines (except the last three)
// and parsing lines between "BEGIN" and "END".
std::vector<Point> process_net(const std::string &path_txt) {
    std::vector<Point> netlist;
    std::ifstream file(path_txt);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path_txt);
    }
    
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    std::string current_net;
    // Process all lines except the last three (if there are that many)
    size_t nlines = (lines.size() > 3 ? lines.size() - 3 : 0);
    for (size_t i = 0; i < nlines; i++) {
        std::string trimmed = trim(lines[i]);
        if (trimmed == "BEGIN") {
            current_net = "";
        } else if (current_net.empty()) {
            current_net = trimmed;
        } else if (trimmed == "END") {
            current_net = "";
        } else {
            // Expecting a line of the form "x,y"
            std::istringstream iss(trimmed);
            std::string token;
            if (std::getline(iss, token, ',')) {
                int x = std::stoi(token);
                if (std::getline(iss, token, ',')) {
                    int y = std::stoi(token);
                    netlist.push_back({static_cast<double>(x), static_cast<double>(y)});
                }
            }
        }
    }
    return netlist;
}

// Compute the directed Hausdorff distance from set A to set B.
double directedHausdorffDistance(const std::vector<Point>& A, const std::vector<Point>& B) {
    double max_min = 0.0;
    for (const auto &p : A) {
        double min_dist = std::numeric_limits<double>::max();
        for (const auto &q : B) {
            double dx = p.x - q.x;
            double dy = p.y - q.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        if (min_dist > max_min) {
            max_min = min_dist;
        }
    }
    return max_min;
}

// Compute the normalized Hausdorff distance between two sets of points.
double hausdorff_distance(const std::vector<Point>& set1, const std::vector<Point>& set2) {
    double d1 = directedHausdorffDistance(set1, set2);
    double d2 = directedHausdorffDistance(set2, set1);
    double max_dist = std::max(d1, d2);
    
    // Compute the bounding box of the union of points.
    double min_x = std::numeric_limits<double>::max(), min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest(), max_y = std::numeric_limits<double>::lowest();
    
    for (const auto &p : set1) {
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }
    for (const auto &p : set2) {
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }
    double bbox_diag = std::sqrt((max_x - min_x) * (max_x - min_x) +
                                 (max_y - min_y) * (max_y - min_y));
    
    return (bbox_diag > 0) ? max_dist / bbox_diag : 0.0;
}

// The main function that mimics the Python code.
// It expects a pandas DataFrame with a column named "net" (a file path),
// computes the Hausdorff similarity for each pair of nets,
// and returns a new DataFrame with rows dropped if similarity > threshold.
py::object remove_similar_data(py::object df, double threshold) {
    std::vector<std::vector<Point>> all_nets;
    std::vector<py::object> indices;
    
    // Use the DataFrame's iterrows() to get (index, row) tuples.
    py::object iterrows = df.attr("iterrows")();
    for (auto row : iterrows) {
        // Each row is a tuple: (index, row_data)
        py::tuple tup = row.cast<py::tuple>();
        py::object index = tup[0];
        py::object row_data = tup[1];
        indices.push_back(index);
        
        // Extract the file path from the "net" column.
        std::string net_file = py::str(row_data.attr("__getitem__")("net"));
        std::vector<Point> netlist = process_net(net_file);
        all_nets.push_back(netlist);
    }
    
    size_t n = all_nets.size();

    // Import tqdm and create a progress bar.
    py::module tqdm_module = py::module::import("tqdm");
    // Create an empty list as a dummy iterable for manual update.
    py::list dummy_list;
    py::object progress = tqdm_module.attr("tqdm")(
        dummy_list, 
        py::arg("total") = n, 
        py::arg("desc") = "Removing similar data", 
        py::arg("leave") = false
    );
    
    std::unordered_set<size_t> to_remove;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (to_remove.count(j)) {
                continue;
            }
            double dist = hausdorff_distance(all_nets[i], all_nets[j]);
            double similarity = 1.0 - dist;
            if (similarity > threshold) {
                to_remove.insert(j);
            }
        }
        // Update the progress bar after each outer loop iteration.
        progress.attr("update")(1);
    }
    // Close the progress bar.
    progress.attr("close")();
    
    // Build a Python list of indices to drop.
    py::list drop_list;
    for (size_t idx : to_remove) {
        drop_list.append(indices[idx]);
    }
    
    // Call the DataFrame's drop() method to remove the selected rows.
    return df.attr("drop")(drop_list);
}

PYBIND11_MODULE(net_similarity, m) {
    m.doc() = "Module to remove similar data from a DataFrame using Hausdorff distance with a progress bar";
    m.def("remove_similar_data", &remove_similar_data, 
          "Remove similar data from a DataFrame based on a similarity threshold",
          py::arg("df"), py::arg("threshold"));
}
