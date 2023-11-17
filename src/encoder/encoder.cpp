#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "encoder/encoder.hpp"

ThreadPool Encoder::s_threadPool = ThreadPool();

// General functions
// ======================================================================================

inline static MetalLayer viaRuleToML(const char* t_name)
{
    static const std::unordered_map<std::string, MetalLayer> layerToMLMap = {
        { "L1M1", MetalLayer::L1M1_V },
        { "M1M2", MetalLayer::M1M2_V },
        { "M2M3", MetalLayer::M2M3_V },
        { "M3M4", MetalLayer::M3M4_V },
        { "M4M5", MetalLayer::M4M5_V },
        { "M5M6", MetalLayer::M5M6_V },
        { "M6M7", MetalLayer::M6M7_V },
        { "M7M8", MetalLayer::M7M8_V },
        { "M8M9", MetalLayer::M8M9_V }
    };

    std::string combinedLayer = { t_name[0], t_name[1], t_name[2], t_name[3] };

    auto it = layerToMLMap.find(combinedLayer);

    if (it != layerToMLMap.end()) {
        return it->second;
    }

    return MetalLayer::NONE;
}

inline static void setGeomOrientation(const int8_t t_orientation, double& t_x, double& t_y)
{
    double temp_x {}, temp_y {};

    switch (t_orientation) {
    case 0:
        break;
    case 1:
        temp_x = -t_y;
        t_y = t_x;
        t_x = temp_x;
        break;
    case 2:
        t_x = -t_x;
        t_y = -t_y;
        break;
    case 3:
        temp_x = t_y;
        t_y = -t_x;
        t_x = temp_x;
        break;
    case 4:
        t_x = -t_x;
        break;
    case 5:
        temp_x = t_y;
        t_y = t_x;
        t_x = temp_x;
        break;
    case 6:
        t_y = -t_y;
        break;
    case 7:
        temp_x = -t_y;
        t_y = -t_x;
        t_x = temp_x;
        break;
    default:
        break;
    }
}

inline static void clearDirectory(const std::string& path)
{
    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_directory(entry)) {
                clearDirectory(entry.path().string());
            } else {
                std::filesystem::remove(entry);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error while clearing directory: " << e.what() << std::endl;
    }
}

// Class methods
// ======================================================================================

void Encoder::readDef(const std::string_view& t_fileName, const std::shared_ptr<Data>& t_data, const PDK& t_pdk, const Config& t_config)
{
    // Init session
    //=================================================================
    int initStatusDef = defrInitSession();

    if (initStatusDef != 0) {
        throw std::runtime_error("Error: cant't initialize data parser!");
    }

    // Open file
    //=================================================================

    auto file = fopen(t_fileName.cbegin(), "r");

    if (file == nullptr) {
        throw std::runtime_error("Error: Can't open a file: " + std::string(t_fileName));
    }

    // Settings
    //=================================================================

    defrSetAddPathToNet();

    // Set callbacks
    //=================================================================

    defrSetDesignCbk(&defDesignCallback);
    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetComponentCbk(&defComponentCallback);
    defrSetPinCbk(&defPinCallback);
    defrSetNetCbk(&defNetCallback);
    defrSetSNetCbk(&defSpecialNetCallback);
    defrSetTrackCbk(&defTrackCallback);

    // Read file
    //=================================================================

    Container* container = new Container { t_config, t_pdk, t_data };

    int readStatus = defrRead(file, t_fileName.cbegin(), static_cast<void*>(container), 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't parse a file: " + std::string(t_fileName));
    }

    fclose(file);
    defrClear();

    for (auto pair = t_data->correspondingToPinCells.begin(); pair != t_data->correspondingToPinCells.end(); ++pair) {
        for (auto& cell : pair->second) {
            auto pin = cell->pins.find(pair->first);

            if (pin != cell->pins.end() && pin->second->netIndex == 0) {
                auto range = cell->pins.equal_range(pair->first);

                for (auto& it = range.first; it != range.second; ++it) {
                    if (it->second->layer != MetalLayer::L1) {
                        cell->geometries.emplace_back(it->second);
                    }
                }

                cell->pins.erase(pair->first);
            }
        }

        // pair = t_data->correspondingToPinCells.erase(pair);
    }

    for (int32_t j = 0; j < t_data->numCellY; ++j) {
        for (int32_t i = 0; i < t_data->numCellX; ++i) {
            std::shared_ptr<WorkingCell> cell = t_data->cells[j][i];

            Point moveByBase(i * t_data->cellSize + t_data->cellOffsetX, j * t_data->cellSize + t_data->cellOffsetY);
            Point moveBy = moveByBase - t_config.getBorderRoutesSize();

            s_threadPool.enqueue([](Config& config, std::shared_ptr<WorkingCell>& cell, Point& moveBy, Point& moveByBase, std::string& name, int32_t& number) {
                torch::Tensor source = torch::zeros({ 7, config.getCellSize(), config.getCellSize() });

                // Fill tracks and obstacles
                // =======================================================================================

                for (const auto& geom : cell->geometries) {
                    uint8_t layerIndex = static_cast<uint8_t>(geom->layer);

                    if (layerIndex % 2 == 0) {
                        std::array<Point, 4> vertexes = geom->vertex;

                        for (auto& vertex : vertexes) {
                            vertex -= moveBy;
                        }

                        switch (geom->type) {
                        case RectangleType::DIEAREA: {
                            source.slice(1, vertexes[0].y, vertexes[2].y).slice(2, vertexes[0].x, vertexes[2].x) = -0.5;
                            break;
                        }
                        case RectangleType::TRACK: {
                            source[1 + layerIndex / 2].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 0.5;
                            break;
                        }
                        default: {
                            source[1 + layerIndex / 2].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                            break;
                        }
                        }
                    }
                }

                // Collect and normalize net information
                // =======================================================================================

                for (auto& route : cell->maskedRoutes) {
                    if (cell->nets.count(route->netIndex) != 0) {
                        cell->nets[route->netIndex].pins.erase(--cell->nets[route->netIndex].pins.end());
                        cell->nets[route->netIndex].pins.insert(1);
                    } else {
                        cell->nets[route->netIndex] = Net();
                        cell->nets[route->netIndex].pins.insert(0);
                    };

                    if (cell->localNetsHash.count(route->netIndex) == 0) {
                        cell->localNetsHash[route->netIndex] = cell->localNetsHash.size();
                    }
                }

                for (auto& [_, pin] : cell->pins) {
                    if (pin->netIndex == 0) {
                        std::cout << pin->name << std::endl;
                    }

                    if (cell->localNetsHash.count(pin->netIndex) == 0) {
                        cell->localNetsHash[pin->netIndex] = cell->localNetsHash.size();
                    }
                }

                // Fill pins
                // =======================================================================================

                for (const auto& [_, pin] : cell->pins) {
                    std::array<Point, 4> vertexes = pin->vertex;

                    for (auto& vertex : vertexes) {
                        vertex -= moveBy;
                    }

                    uint8_t layerIndex = static_cast<uint8_t>(pin->layer);

                    if (layerIndex != 0) {
                        source[1 + layerIndex / 2].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                    }

                    source[1].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = static_cast<double>(cell->localNetsHash[pin->netIndex] / cell->localNetsHash.size()) - 0.5;

                    if ((*cell->nets[pin->netIndex].pins.rbegin()) != 0) {
                        source[0].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                    } else {
                        source[0].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 0.5;
                    }
                }

                // Fill masked routes
                // =======================================================================================

                for (const auto& mask : cell->maskedRoutes) {
                    std::array<Point, 4> vertexes = mask->vertex;

                    for (auto& vertex : vertexes) {
                        vertex -= moveByBase;
                    }

                    uint8_t layerIndex = static_cast<uint8_t>(mask->layer);

                    source[1 + layerIndex / 2].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                    source[1].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = static_cast<double>(cell->localNetsHash[mask->netIndex] / cell->localNetsHash.size()) - 0.5;

                    if ((*cell->nets[mask->netIndex].pins.rbegin()) != 0) {
                        source[0].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                    } else {
                        source[0].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 0.5;
                    }
                }

                // Save source tensor
                // =======================================================================================

                auto pickledSource = torch::pickle_save(source);
                std::ofstream foutSource("./cache/" + name + "/source_" + std::to_string(number) + ".pt", std::ios::out | std::ios::binary);
                foutSource.write(pickledSource.data(), pickledSource.size());
                foutSource.close();
            },
                t_config, cell, moveBy, moveByBase, t_data->design, (i + 1) * (j + 1));

            s_threadPool.enqueue([](Config& config, std::shared_ptr<WorkingCell>& cell, Point& moveBy, std::string& name, int32_t& number) {
                torch::Tensor target = torch::zeros({ 5, config.getCellSize(), config.getCellSize() });

                // Fill target routes
                // =======================================================================================

                for (const auto& rout : cell->routes) {
                    auto vertexes = rout->vertex;

                    uint8_t layerIndex = static_cast<uint8_t>(rout->layer);

                    if (layerIndex % 2 == 0) {
                        for (auto& vertex : vertexes) {
                            vertex -= moveBy;
                        }

                        target[layerIndex / 2 - 1].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 1.0;
                    }
                }

                // Save target tensor
                // =======================================================================================

                auto pickledTarget = torch::pickle_save(target);
                std::ofstream foutTarget("./cache/" + name + "/target_" + std::to_string(number) + ".pt", std::ios::out | std::ios::binary);
                foutTarget.write(pickledTarget.data(), pickledTarget.size());
                foutTarget.close();
            },
                t_config, cell, moveBy, t_data->design, (i + 1) * (j + 1));
        }
    }

    s_threadPool.wait();

    delete container;
}

void Encoder::addToWorkingCells(const std::shared_ptr<Rectangle>& t_target, Container* t_container)
{
    static auto intersectionArea = [](const Rectangle& t_fl, const std::shared_ptr<Rectangle>& t_rl) {
        return std::array<int32_t, 4> {
            std::max(t_fl.vertex[0].x, t_rl->vertex[0].x),
            std::max(t_fl.vertex[0].y, t_rl->vertex[0].y),
            std::min(t_fl.vertex[2].x, t_rl->vertex[2].x),
            std::min(t_fl.vertex[2].y, t_rl->vertex[2].y),
        };
    };

    if (!t_target) {
        throw std::runtime_error("Can't add to working cell, target is not exists!");
    }

    for (auto& point : t_target->vertex) {
        point = point / t_container->pdk.scale;
    }

    Point lt = (t_target->vertex[0] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
    Point rt = (t_target->vertex[1] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
    Point lb = (t_target->vertex[3] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;

    if (lt.x >= t_container->data->numCellX || lt.y >= t_container->data->numCellY || rt.x >= t_container->data->numCellX || lb.y >= t_container->data->numCellY) {
        throw std::runtime_error("Indexes out of bounds in working cells!");
    }

    switch (t_target->type) {
    case RectangleType::PIN: {
        std::shared_ptr<Pin> pin = std::static_pointer_cast<Pin>(t_target);

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle cellRect = t_container->data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(cellRect, t_target);

                std::shared_ptr<Pin> pinCut = std::make_shared<Pin>(pin->name, inter[0], inter[1], inter[2], inter[3], pin->layer);

                pinCut->parentCell = t_container->data->cells[j][i];

                t_container->data->correspondingToPinCells[pin->name].insert(t_container->data->cells[j][i]);
                t_container->data->cells[j][i]->pins.insert(std::pair(pin->name, pinCut));
            }
        }
        break;
    }
    case RectangleType::SIGNAL: {
        std::shared_ptr<Route> rout = std::static_pointer_cast<Route>(t_target);

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle cellRect = t_container->data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(cellRect, t_target);
                std::shared_ptr<Route> routCut {};

                if (i != lt.x || j != lt.y && (static_cast<int8_t>(rout->layer) % 2 == 0)) {
                    std::shared_ptr<Route> maskedRoutCut {};

                    if (inter[2] - inter[0] <= inter[3] - inter[1]) {
                        maskedRoutCut = std::make_shared<Route>(inter[0], inter[1], inter[2], inter[1] + t_container->config.getBorderRoutesSize(), rout->layer, rout->netIndex);
                    } else {
                        maskedRoutCut = std::make_shared<Route>(inter[0], inter[1], inter[0] + t_container->config.getBorderRoutesSize(), inter[3], rout->layer, rout->netIndex);
                    }

                    t_container->data->cells[j][i]->maskedRoutes.emplace_back(maskedRoutCut);
                }

                if (inter[0] != inter[2] && inter[1] != inter[3]) {
                    routCut = std::make_shared<Route>(inter[0], inter[1], inter[2], inter[3], rout->layer);
                } else {
                    inter[2] = std::min(cellRect.vertex[2].x, inter[2]);
                    inter[3] = std::min(cellRect.vertex[2].y, inter[3]);
                    routCut = std::make_shared<Route>(inter[0], inter[1], inter[2], inter[3], rout->layer);
                }

                t_container->data->cells[j][i]->routes.emplace_back(routCut);
            }
        }
        break;
    }
    case RectangleType::TRACK:
    case RectangleType::CLOCK:
    case RectangleType::POWER:
    case RectangleType::GROUND:
    case RectangleType::DIEAREA: {
        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle cellRect = t_container->data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(cellRect, t_target);
                std::shared_ptr<Rectangle> rect {};

                if (inter[0] != inter[2] && inter[1] != inter[3]) {
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                } else {
                    inter[2] = std::min(cellRect.vertex[2].x, inter[2]);
                    inter[3] = std::min(cellRect.vertex[2].y, inter[3]);
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                }

                t_container->data->cells[j][i]->geometries.emplace_back(rect);
            }
        }
        break;
    }
    default:
        break;
    }
}

// Def callbacks
// ==================================================================================================================================================

int Encoder::defDesignCallback(defrCallbackType_e t_type, const char* t_design, void* t_userData)
{
    if (t_type != defrDesignStartCbkType) {
        return 2;
    }

    Container* container = static_cast<Container*>(t_userData);
    container->data->design = t_design;

    std::string path = "./cache/" + container->data->design;

    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    } else {
        clearDirectory(path);
    }

    return 0;
}

int Encoder::defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
{
    if (t_type != defrCallbackType_e::defrDieAreaCbkType) {
        return 2;
    }

    defiPoints points = t_box->getPoint();
    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    if (points.numPoints != 2) {
        for (std::size_t i = 0; i < points.numPoints; ++i) {
            container->data->dieArea.emplace_back(Point(points.x[i], points.y[i]));
        }
    } else {
        container->data->dieArea.emplace_back(Point(points.x[0], points.y[0]));
        container->data->dieArea.emplace_back(Point(points.x[1], points.y[0]));
        container->data->dieArea.emplace_back(Point(points.x[1], points.y[1]));
        container->data->dieArea.emplace_back(Point(points.x[0], points.y[1]));
        container->data->dieArea.emplace_back(Point(points.x[0], points.y[0]));
    }

    Point leftTop { INT32_MAX, INT32_MAX };
    Point rightBottom { 0, 0 };

    for (auto& point : container->data->dieArea) {
        leftTop.x = std::min(point.x, leftTop.x);
        leftTop.y = std::min(point.y, leftTop.y);
        rightBottom.x = std::max(point.x, rightBottom.x);
        rightBottom.y = std::max(point.y, rightBottom.y);
    }

    double width = (rightBottom.x - leftTop.x) / container->pdk.scale;
    double height = (rightBottom.y - leftTop.y) / container->pdk.scale;

    container->data->cellSize = container->config.getCellSize() - container->config.getBorderRoutesSize();
    container->data->numCellX = std::ceil((width + container->config.getBorderSize()) / container->data->cellSize);
    container->data->numCellY = std::ceil((height + container->config.getBorderSize()) / container->data->cellSize);
    container->data->cellOffsetX = leftTop.x - container->config.getBorderSize();
    container->data->cellOffsetY = leftTop.y - container->config.getBorderSize();
    container->data->cells = std::vector<std::vector<std::shared_ptr<WorkingCell>>>(container->data->numCellY, std::vector<std::shared_ptr<WorkingCell>>(container->data->numCellX, nullptr));

    for (std::size_t j = 0; j < container->data->numCellY; ++j) {
        for (std::size_t i = 0; i < container->data->numCellX; ++i) {
            int32_t left = container->data->cellOffsetX + i * container->data->cellSize;
            int32_t top = container->data->cellOffsetY + j * container->data->cellSize;
            int32_t right = container->data->cellOffsetX + (i + 1) * container->data->cellSize;
            int32_t bottom = container->data->cellOffsetY + (j + 1) * container->data->cellSize;

            std::shared_ptr<WorkingCell> cell = std::make_shared<WorkingCell>();

            cell->originalPlace = Rectangle(left, top, right, bottom, MetalLayer::NONE);
            container->data->cells[j][i] = cell;
        }
    }

    for (std::size_t i = 0; i < container->data->dieArea.size() - 1; ++i) {
        int32_t left = container->data->dieArea[i].x;
        int32_t top = container->data->dieArea[i].y;
        int32_t right = container->data->dieArea[i + 1].x;
        int32_t bottom = container->data->dieArea[i + 1].y;

        addToWorkingCells(std::make_shared<Rectangle>(left, top, right, bottom, MetalLayer::NONE, RectangleType::DIEAREA), container);
    }

    return 0;
}

int Encoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type != defrComponentCbkType) {
        return 2;
    }

    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    Point placed {};

    if (t_component->isFixed() || t_component->isPlaced()) {
        placed = Point(t_component->placementX(), t_component->placementY());
    }

    if (container->pdk.macros.count(t_component->name()) == 0) {
        throw std::runtime_error("Can't find macro: " + std::string(t_component->name()) + " in provided PDK!");
    };

    PDK::Macro macro = container->pdk.macros.at(t_component->name());
    PointF leftBottom = { INT32_MAX, INT32_MAX };
    PointF newLeftBottom { INT32_MAX, INT32_MAX };

    for (auto& pin : macro.pins) {
        for (auto& port : pin.ports) {
            for (auto& vertex : port.vertex) {
                leftBottom.x = std::min(vertex.x, leftBottom.x);
                leftBottom.y = std::min(vertex.y, leftBottom.y);

                setGeomOrientation(t_component->placementOrient(), vertex.x, vertex.y);

                newLeftBottom.x = std::min(vertex.x, newLeftBottom.x);
                newLeftBottom.y = std::min(vertex.y, newLeftBottom.y);
            }

            port.fixVertex();
        }
    }

    for (auto& pin : macro.pins) {
        bool isSignal = pin.use == "SIGNAL";

        for (auto& port : pin.ports) {
            for (auto& vertex : port.vertex) {
                vertex.x += (leftBottom.x - newLeftBottom.x) + placed.x;
                vertex.y += (leftBottom.y - newLeftBottom.y) + placed.y;
            }

            Rectangle rect { port };

            if (isSignal) {
                addToWorkingCells(std::make_shared<Pin>(t_component->id() + pin.name, rect.vertex[0].x, rect.vertex[0].y, rect.vertex[2].x, rect.vertex[2].y, rect.layer), container);
            } else {
                if (rect.layer != MetalLayer::L1) {
                    addToWorkingCells(std::make_shared<Rectangle>(rect.vertex[0].x, rect.vertex[0].y, rect.vertex[2].x, rect.vertex[2].y, rect.layer, RectangleType::CLOCK), container);
                }
            }
        }
    }

    return 0;
};

int Encoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type != defrNetCbkType) {
        return 2;
    }

    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    std::vector<std::shared_ptr<Route>> route {};
    RectangleType rType {};

    if (t_net->hasUse()) {
        if (strcmp(t_net->use(), "SIGNAL") == 0) {
            rType = RectangleType::SIGNAL;
            ++container->data->totalNets;
        } else if (strcmp(t_net->use(), "CLOCK") == 0) {
            rType = RectangleType::CLOCK;
        }
    }

    if (t_net->isRouted() == 0) {
        for (std::size_t i = 0; i < t_net->numWires(); ++i) {
            defiWire* wire = t_net->wire(i);

            for (std::size_t j = 0; j < wire->numPaths(); ++j) {
                defiPath* wirePath = wire->path(j);
                wirePath->initTraverse();

                bool isStartSet = false;
                bool isEndSet = false;
                bool isViaRect = false;
                const char* viaName {};
                int32_t tokenType {};
                int32_t width {};
                int32_t extStart {};
                int32_t extEnd {};
                PDK::Layer layer {};
                Point start {};
                Point end {};

                while ((tokenType = wirePath->next()) != DEFIPATH_DONE) {
                    switch (tokenType) {
                    case DEFIPATH_LAYER: {
                        if (container->pdk.layers.count(wirePath->getLayer()) == 0) {
                            throw std::runtime_error("Can't find layer: " + std::string(wirePath->getLayer()) + " in provided PDK!");
                        }

                        layer = container->pdk.layers[wirePath->getLayer()];
                        break;
                    }
                    case DEFIPATH_WIDTH:
                        width = wirePath->getWidth();
                        break;
                    case DEFIPATH_VIA:
                        viaName = wirePath->getVia();
                        break;
                    case DEFIPATH_POINT: {
                        if (!isStartSet) {
                            isStartSet = true;
                            wirePath->getPoint(&start.x, &start.y);
                        } else {
                            isEndSet = true;
                            wirePath->getPoint(&end.x, &end.y);
                        }
                        break;
                    }
                    case DEFIPATH_FLUSHPOINT: {
                        if (!isStartSet) {
                            isStartSet = true;
                            wirePath->getFlushPoint(&start.x, &start.y, &extStart);
                        } else {
                            isEndSet = true;
                            wirePath->getFlushPoint(&end.x, &end.y, &extEnd);
                        }
                        break;
                    }
                    case DEFIPATH_RECT:
                        isViaRect = true;
                        break;
                    default:
                        break;
                    }
                }

                if (viaName == nullptr) {
                    if (isStartSet && isEndSet) {
                        int32_t left = start.x - layer.width / 2.0;
                        int32_t top = start.y - layer.width / 2.0;
                        int32_t right = end.x + layer.width / 2.0;
                        int32_t bottom = end.y + layer.width / 2.0;

                        if (rType == RectangleType::SIGNAL) {
                            route.emplace_back(std::make_shared<Route>(left, top, right, bottom, layer.metal, container->data->totalNets));

                            addToWorkingCells(route.back(), container);
                        } else {
                            addToWorkingCells(std::make_shared<Rectangle>(left, top, right, bottom, layer.metal, rType), container);
                        }
                    }
                } else {
                    if (rType == RectangleType::SIGNAL) {
                        route.emplace_back(std::make_shared<Route>(start.x, start.y, start.x, start.y, viaRuleToML(viaName), 0));
                    }
                }
            }
        }
    }

    if (rType == RectangleType::SIGNAL && !route.empty()) {
        std::vector<std::string> pinsNames(t_net->numConnections(), "");

        for (std::size_t i = 0; i < t_net->numConnections(); ++i) {
            pinsNames[i] = std::string(t_net->instance(i)) + t_net->pin(i);
            std::shared_ptr<WorkingCell> cellThatStays {};

            for (auto& cell : container->data->correspondingToPinCells[pinsNames[i]]) {
                auto range = cell->pins.equal_range(pinsNames[i]);

                for (auto& it = range.first; it != range.second && !cellThatStays; ++it) {
                    for (const auto& part : route) {
                        if (static_cast<int8_t>(part->layer) - 1 == static_cast<int8_t>(it->second->layer) || static_cast<int8_t>(part->layer) == static_cast<int8_t>(it->second->layer)) {
                            if ((part->vertex[2].x >= it->second->vertex[0].x || it->second->vertex[2].x <= part->vertex[0].x) && (part->vertex[2].y >= it->second->vertex[0].y || it->second->vertex[2].y <= part->vertex[0].y)) {
                                cellThatStays = cell;
                                break;
                            }
                        }
                    }
                }

                if (cellThatStays) {
                    break;
                }
            }

            for (const auto& cell : container->data->correspondingToPinCells[pinsNames[i]]) {
                if (cell != cellThatStays) {
                    cell->pins.erase(pinsNames[i]);
                }
            }

            container->data->correspondingToPinCells[pinsNames[i]].clear();
            container->data->correspondingToPinCells[pinsNames[i]].insert(cellThatStays);
        }

        for (const auto& name : pinsNames) {
            if (container->data->correspondingToPinCells.count(name) == 0) {
                throw std::runtime_error("Can't find pin '" + name + "' in any of working cells!");
            };

            std::shared_ptr<WorkingCell> cell = *container->data->correspondingToPinCells.at(name).begin();
            Net net {};

            net.index = container->data->totalNets;

            if (cell->nets.count(net.index) == 0) {
                for (std::size_t i = 0; i < pinsNames.size(); ++i) {
                    auto range = cell->pins.equal_range(pinsNames[i]);

                    if (range.first != range.second) {
                        net.pins.insert(1);

                        for (auto& it = range.first; it != range.second; ++it) {
                            it->second->netIndex = container->data->totalNets;
                        }
                    } else {
                        net.pins.insert(0);
                    }
                }

                cell->nets[net.index] = net;
            }
        }
    }

    return 0;
};

int Encoder::defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type != defrSNetCbkType) {
        return 2;
    }

    RectangleType rType {};

    if (t_net->hasUse()) {
        if (strcmp(t_net->use(), "POWER") == 0) {
            rType = RectangleType::POWER;
        } else if (strcmp(t_net->use(), "GROUND") == 0) {
            rType = RectangleType::GROUND;
        }
    }

    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    for (std::size_t i = 0; i < t_net->numWires(); ++i) {
        defiWire* wire = t_net->wire(i);

        for (std::size_t j = 0; j < wire->numPaths(); ++j) {
            defiPath* wirePath = wire->path(j);
            wirePath->initTraverse();

            int tokenType {};
            int width {};
            bool isStartSet = false;
            const char* viaName {};
            PDK::Layer layer {};
            Point start {};
            Point end {};

            while ((tokenType = wirePath->next()) != DEFIPATH_DONE) {
                switch (tokenType) {
                case DEFIPATH_LAYER: {
                    if (container->pdk.layers.count(wirePath->getLayer()) == 0) {
                        throw std::runtime_error("Can't find layer: " + std::string(wirePath->getLayer()) + " in provided PDK!");
                    }

                    layer = container->pdk.layers[wirePath->getLayer()];
                    break;
                }
                case DEFIPATH_WIDTH:
                    width = wirePath->getWidth();
                    break;
                case DEFIPATH_VIA:
                    viaName = wirePath->getVia();
                    break;
                case DEFIPATH_POINT: {
                    if (!isStartSet) {
                        isStartSet = true;
                        wirePath->getPoint(&start.x, &start.y);
                    } else {
                        wirePath->getPoint(&end.x, &end.y);
                    }
                    break;
                }
                default:
                    break;
                }
            }

            if (!viaName) {
                std::shared_ptr<Rectangle> rect {};
                if (start.x != end.x) {
                    int32_t top = start.y - width / 2.0;
                    int32_t bottom = end.y + width / 2.0;

                    rect = std::make_shared<Rectangle>(start.x, top, end.x, bottom, layer.metal, rType);
                } else {
                    int32_t left = start.x - width / 2.0;
                    int32_t right = end.x + width / 2.0;

                    rect = std::make_shared<Rectangle>(left, start.y, right, end.y, layer.metal, rType);
                }

                addToWorkingCells(rect, container);
            }
        }
    }

    return 0;
};

int Encoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type != defrPinCbkType) {
        return 2;
    }

    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    bool isSignal = strcmp(t_pin->use(), "SIGNAL") == 0;

    for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
        defiPinPort* pinPort = t_pin->pinPort(i);
        int32_t xPlacement = pinPort->placementX();
        int32_t yPlacement = pinPort->placementY();
        int32_t xl {}, yl {}, xh {}, yh {};
        bool isPortPlaced = pinPort->isPlaced() || pinPort->isFixed() || pinPort->isCover();

        for (std::size_t j = 0; j < pinPort->numLayer(); ++j) {
            pinPort->bounds(j, &xl, &yl, &xh, &yh);

            if (isPortPlaced) {
                xl += xPlacement;
                yl += yPlacement;
                xh += xPlacement;
                yh += yPlacement;
            }

            if (container->pdk.layers.count(pinPort->layer(j)) == 0) {
                throw std::runtime_error("Can't find layer: " + std::string(pinPort->layer(j)) + " in provided PDK!");
            }

            PDK::Layer layer = container->pdk.layers[pinPort->layer(j)];

            if (isSignal) {
                std::shared_ptr<Pin> pinRect = std::make_shared<Pin>("PIN" + std::string(t_pin->pinName()), xl, yl, xh, yh, layer.metal);

                addToWorkingCells(pinRect, container);
            } else {
                std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xl, yl, xh, yh, layer.metal, RectangleType::CLOCK);

                addToWorkingCells(rect, container);
            }
        }
    }

    return 0;
};

int Encoder::defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData)
{
    if (t_type != defrTrackCbkType) {
        return 2;
    }

    Container* container = static_cast<Container*>(t_userData);

    if (!container) {
        throw std::runtime_error("Encoder's container is not defined!");
    }

    if (container->pdk.layers.count(t_track->layer(0)) == 0) {
        throw std::runtime_error("Can't find layer: " + std::string(t_track->layer(0)) + " in provided PDK!");
    }

    PDK::Layer layer = container->pdk.layers[t_track->layer(0)];

    if (layer.metal != MetalLayer::L1) {
        double offset = t_track->x();
        double number = t_track->xNum();
        double step = t_track->xStep();

        if (strcmp(t_track->macro(), "X") == 0) {
            for (double x = offset; x < number * step; x += step) {
                addToWorkingCells(std::make_shared<Rectangle>(x - layer.width / 2.0, 0, x + layer.width / 2.0, container->data->dieArea[2].y, layer.metal, RectangleType::TRACK), container);
            }
        } else if (strcmp(t_track->macro(), "Y") == 0) {
            for (double y = offset; y < number * step; y += step) {
                addToWorkingCells(std::make_shared<Rectangle>(0, y - layer.width / 2.0, container->data->dieArea[2].x, y + layer.width / 2.0, layer.metal, RectangleType::TRACK), container);
            }
        }
    }

    return 0;
};
