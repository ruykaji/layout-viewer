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

void Encoder::readDef(const std::string_view& t_fileName, const std::shared_ptr<Data>& t_data, const std::shared_ptr<PDK>& t_pdk, const std::shared_ptr<Config>& t_config)
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

    for (int32_t j = 0; j < t_data->numCellY; ++j) {
        for (int32_t i = 0; i < t_data->numCellX; ++i) {
            std::shared_ptr<WorkingCell> cell = t_data->cells[j][i];
            Point moveBy(i * t_data->cellSize + t_data->cellOffsetX, j * t_data->cellSize + t_data->cellOffsetY);

            s_threadPool.enqueue([](std::shared_ptr<Config>& config, std::shared_ptr<WorkingCell>& cell, Point& moveBy, std::string& name, int32_t& number) {
                torch::Tensor source = torch::zeros({ 5, config->getCellSize(), config->getCellSize() });

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
                        // case RectangleType::TRACK: {
                        //     source[layerIndex / 2 - 1].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 0.5;
                        //     break;
                        // }
                        default: {
                            source[layerIndex / 2 - 1].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = -0.5;
                            break;
                        }
                        }
                    }
                }

                torch::save(source, "./cache/" + name + "/source_" + std::to_string(number) + ".pt");
            },
                t_config, cell, moveBy, t_data->design, (i + 1) * (j + 1));

            s_threadPool.enqueue([](std::shared_ptr<Config>& config, std::shared_ptr<WorkingCell>& cell, Point& moveBy, std::string& name, int32_t& number) {
                torch::Tensor totalConnections = torch::zeros({ 0, 7 });

                for (auto& [_, net] : cell->nets) {
                    torch::Tensor connections = torch::zeros({ static_cast<int32_t>(net.size()), 7 });

                    for (std::size_t i = 0; i < net.size(); ++i) {
                        connections[i] = torch::tensor({ net[i].start.x, net[i].start.y, net[i].startLayer, net[i].end.x, net[i].end.y, net[i].endLayer, net[i].isOutOfCell });
                    }

                    totalConnections = torch::cat({ totalConnections, connections }, 0);
                }

                torch::save(totalConnections, "./cache/" + name + "/connections_" + std::to_string(number) + ".pt");
            },
                t_config, cell, moveBy, t_data->design, (i + 1) * (j + 1));
        }
    }

    s_threadPool.wait();

    delete container;
}

void Encoder::addGeometryToWorkingCells(const std::shared_ptr<Rectangle>& t_target, Container* t_container)
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

    if (t_target->layer != MetalLayer::L1) {
        for (auto& point : t_target->vertex) {
            point = point / t_container->pdk->scale;
        }

        Point lt = (t_target->vertex[0] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
        Point rt = (t_target->vertex[1] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
        Point lb = (t_target->vertex[3] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;

        if (lt.x >= t_container->data->numCellX || lt.y >= t_container->data->numCellY || rt.x >= t_container->data->numCellX || lb.y >= t_container->data->numCellY) {
            throw std::runtime_error("Working cells is out of bounds!");
        }

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
    }
}

void Encoder::addPinToWorkingCells(const std::vector<std::shared_ptr<Rectangle>>& t_target, const std::string& t_pinName, Container* t_container)
{
    static auto intersectionArea = [](const Rectangle& t_fl, const std::shared_ptr<Rectangle>& t_rl) {
        return std::array<int32_t, 4> {
            std::max(t_fl.vertex[0].x, t_rl->vertex[0].x),
            std::max(t_fl.vertex[0].y, t_rl->vertex[0].y),
            std::min(t_fl.vertex[2].x, t_rl->vertex[2].x),
            std::min(t_fl.vertex[2].y, t_rl->vertex[2].y),
        };
    };

    int32_t largestArea = 0;
    Point cellCoords {};
    std::shared_ptr<Rectangle> largestRect {};

    for (auto& targetRect : t_target) {
        if (!targetRect) {
            throw std::runtime_error("Can't add to working cell, target is not exists!");
        }

        for (auto& point : targetRect->vertex) {
            point = point / t_container->pdk->scale;
        }

        Point lt = (targetRect->vertex[0] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
        Point rt = (targetRect->vertex[1] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;
        Point lb = (targetRect->vertex[3] - Point(t_container->data->cellOffsetX, t_container->data->cellOffsetY)) / t_container->data->cellSize;

        if (lt.x >= t_container->data->numCellX || lt.y >= t_container->data->numCellY || rt.x >= t_container->data->numCellX || lb.y >= t_container->data->numCellY) {
            throw std::runtime_error("Working cells is out of bounds!");
        }

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle cellRect = t_container->data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(cellRect, targetRect);
                std::shared_ptr<Rectangle> rect {};

                if (inter[0] != inter[2] && inter[1] != inter[3]) {
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], targetRect->layer, targetRect->type);
                } else {
                    inter[2] = std::min(cellRect.vertex[2].x, inter[2]);
                    inter[3] = std::min(cellRect.vertex[2].y, inter[3]);
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], targetRect->layer, targetRect->type);
                }

                int32_t area = (inter[2] - inter[0]) * (inter[3] - inter[1]);

                if (area > largestArea) {
                    largestArea = area;
                    largestRect = rect;
                    cellCoords.x = i;
                    cellCoords.y = j;
                }

                if (targetRect->layer != MetalLayer::L1) {
                    t_container->data->cells[j][i]->geometries.emplace_back(targetRect);
                }
            }
        }
    }

    t_container->data->correspondingToPinCell[t_pinName] = t_container->data->cells[cellCoords.y][cellCoords.x];
    t_container->data->pins[t_pinName] = largestRect;
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

    double width = (rightBottom.x - leftTop.x) / container->pdk->scale;
    double height = (rightBottom.y - leftTop.y) / container->pdk->scale;

    container->data->cellSize = container->config->getCellSize();
    container->data->numCellX = std::ceil((width + container->config->getBorderSize()) / container->data->cellSize);
    container->data->numCellY = std::ceil((height + container->config->getBorderSize()) / container->data->cellSize);
    container->data->cellOffsetX = leftTop.x - container->config->getBorderSize();
    container->data->cellOffsetY = leftTop.y - container->config->getBorderSize();
    container->data->cells = std::vector<std::vector<std::shared_ptr<WorkingCell>>>(container->data->numCellY, std::vector<std::shared_ptr<WorkingCell>>(container->data->numCellX, nullptr));

    for (std::size_t j = 0; j < container->data->numCellY; ++j) {
        for (std::size_t i = 0; i < container->data->numCellX; ++i) {
            int32_t left = container->data->cellOffsetX + i * container->data->cellSize;
            int32_t top = container->data->cellOffsetY + j * container->data->cellSize;
            int32_t right = container->data->cellOffsetX + (i + 1) * container->data->cellSize;
            int32_t bottom = container->data->cellOffsetY + (j + 1) * container->data->cellSize;

            std::shared_ptr<WorkingCell> cell = std::make_shared<WorkingCell>();

            cell->name = std::to_string(j) + std::to_string(i);
            cell->originalPlace = Rectangle(left, top, right, bottom, MetalLayer::NONE);
            container->data->cells[j][i] = cell;
        }
    }

    for (std::size_t i = 0; i < container->data->dieArea.size() - 1; ++i) {
        int32_t left = container->data->dieArea[i].x;
        int32_t top = container->data->dieArea[i].y;
        int32_t right = container->data->dieArea[i + 1].x;
        int32_t bottom = container->data->dieArea[i + 1].y;

        addGeometryToWorkingCells(std::make_shared<Rectangle>(left, top, right, bottom, MetalLayer::NONE, RectangleType::DIEAREA), container);
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

    if (container->pdk->macros.count(t_component->name()) == 0) {
        throw std::runtime_error("Can't find macro: " + std::string(t_component->name()) + " in provided PDK!");
    };

    PDK::Macro macro = container->pdk->macros.at(t_component->name());
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
        std::vector<std::shared_ptr<Rectangle>> pinsRects {};

        for (auto& port : pin.ports) {
            for (auto& vertex : port.vertex) {
                vertex.x += (leftBottom.x - newLeftBottom.x) + placed.x;
                vertex.y += (leftBottom.y - newLeftBottom.y) + placed.y;
            }

            // TODO: fix this port -> rect
            Rectangle rect(port);

            if (isSignal) {
                pinsRects.emplace_back(std::make_shared<Rectangle>(rect.vertex[0].x, rect.vertex[0].y, rect.vertex[2].x, rect.vertex[2].y, rect.layer, RectangleType::PIN));
            } else {
                addGeometryToWorkingCells(std::make_shared<Rectangle>(rect.vertex[0].x, rect.vertex[0].y, rect.vertex[2].x, rect.vertex[2].y, rect.layer, RectangleType::CLOCK), container);
            }
        }

        if (!pinsRects.empty()) {
            addPinToWorkingCells(pinsRects, std::string(t_component->id()) + pin.name, container);
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

    RectangleType rType {};
    std::string netName { t_net->name() };

    if (t_net->hasUse()) {
        if (strcmp(t_net->use(), "SIGNAL") == 0) {
            rType = RectangleType::SIGNAL;
            ++container->data->totalNets;
        } else if (strcmp(t_net->use(), "CLOCK") == 0) {
            rType = RectangleType::CLOCK;
        }
    }

    if (t_net->isRouted() == 0 && rType != RectangleType::SIGNAL) {
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
                        if (container->pdk->layers.count(wirePath->getLayer()) == 0) {
                            throw std::runtime_error("Can't find layer: " + std::string(wirePath->getLayer()) + " in provided PDK!");
                        }

                        layer = container->pdk->layers[wirePath->getLayer()];
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

                        addGeometryToWorkingCells(std::make_shared<Rectangle>(left, top, right, bottom, layer.metal, rType), container);
                    }
                }
            }
        }
    }

    if (rType == RectangleType::SIGNAL) {
        std::vector<std::string> pinsNames(t_net->numConnections(), "");

        for (std::size_t i = 0; i < t_net->numConnections(); ++i) {
            pinsNames[i] = std::string(t_net->instance(i)) + t_net->pin(i);
        }

        for (std::size_t i = 0; i < pinsNames.size(); ++i) {
            std::shared_ptr<WorkingCell> cell = container->data->correspondingToPinCell[pinsNames[i]];

            if (cell->nets.count(netName) == 0) {
                std::vector<WorkingCell::Connection> connections {};
                std::shared_ptr<Rectangle> pinRect = container->data->pins[pinsNames[i]];
                Point start((pinRect->vertex[2].x + pinRect->vertex[0].x) / 2, (pinRect->vertex[3].y + pinRect->vertex[1].y) / 2);
                MetalLayer startLayer = pinRect->layer;

                for (std::size_t j = 0; j < pinsNames.size(); ++j) {
                    if (j != i) {
                        pinRect = container->data->pins[pinsNames[j]];

                        bool isOutOfCell = std::find(cell->pins.begin(), cell->pins.end(), pinsNames[j]) == cell->pins.end();
                        Point end((pinRect->vertex[2].x + pinRect->vertex[0].x) / 2, (pinRect->vertex[3].y + pinRect->vertex[1].y) / 2);

                        connections.emplace_back(WorkingCell::Connection { static_cast<int8_t>(isOutOfCell), start, static_cast<int8_t>(startLayer) / 2 - 1, end, static_cast<int8_t>(pinRect->layer) / 2 - 1 });
                    }
                }

                cell->nets[netName] = connections;
            }
        }

        container->data->signalNets[netName] = pinsNames;
    } else {
        for (std::size_t i = 0; i < t_net->numConnections(); ++i) {
            std::string pinName = std::string(t_net->instance(i)) + t_net->pin(i);
            auto range = container->data->pins.equal_range(pinName);

            for (auto it = range.first; it != range.second; ++it) {
                addGeometryToWorkingCells(it->second, container);
            }

            container->data->correspondingToPinCell.erase(pinName);
            container->data->pins.erase(pinName);
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
                    if (container->pdk->layers.count(wirePath->getLayer()) == 0) {
                        throw std::runtime_error("Can't find layer: " + std::string(wirePath->getLayer()) + " in provided PDK!");
                    }

                    layer = container->pdk->layers[wirePath->getLayer()];
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

                addGeometryToWorkingCells(rect, container);
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
    std::vector<std::shared_ptr<Rectangle>> pinsRects {};

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

            if (container->pdk->layers.count(pinPort->layer(j)) == 0) {
                throw std::runtime_error("Can't find layer: " + std::string(pinPort->layer(j)) + " in provided PDK!");
            }

            PDK::Layer layer = container->pdk->layers[pinPort->layer(j)];

            if (isSignal) {
                pinsRects.emplace_back(std::make_shared<Rectangle>(xl, yl, xh, yh, layer.metal, RectangleType::PIN));
            } else {
                addGeometryToWorkingCells(std::make_shared<Rectangle>(xl, yl, xh, yh, layer.metal, RectangleType::CLOCK), container);
            }
        }
    }

    if (!pinsRects.empty()) {
        addPinToWorkingCells(pinsRects, "PIN" + std::string(t_pin->pinName()), container);
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

    if (container->pdk->layers.count(t_track->layer(0)) == 0) {
        throw std::runtime_error("Can't find layer: " + std::string(t_track->layer(0)) + " in provided PDK!");
    }

    PDK::Layer layer = container->pdk->layers[t_track->layer(0)];

    if (layer.metal != MetalLayer::L1) {
        double offset = t_track->x();
        double number = t_track->xNum();
        double step = t_track->xStep();

        if (strcmp(t_track->macro(), "X") == 0) {
            for (double x = offset; x < number * step; x += step) {
                addGeometryToWorkingCells(std::make_shared<Rectangle>(x - layer.width / 2.0, 0, x + layer.width / 2.0, container->data->dieArea[2].y, layer.metal, RectangleType::TRACK), container);
            }
        } else if (strcmp(t_track->macro(), "Y") == 0) {
            for (double y = offset; y < number * step; y += step) {
                addGeometryToWorkingCells(std::make_shared<Rectangle>(0, y - layer.width / 2.0, container->data->dieArea[2].x, y + layer.width / 2.0, layer.metal, RectangleType::TRACK), container);
            }
        }
    }

    return 0;
};
