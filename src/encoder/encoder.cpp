#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "encoder/encoder.hpp"

PDK Encoder::s_pdk = PDK();
ThreadPool Encoder::s_threadPool = ThreadPool();

// General functions
// ======================================================================================

inline static MetalLayer convertNameToML(const char* t_name)
{
    static const std::unordered_map<std::string, MetalLayer> nameToMLMap = {
        { "li1", MetalLayer::L1 },
        { "met1", MetalLayer::M1 },
        { "met2", MetalLayer::M2 },
        { "met3", MetalLayer::M3 },
        { "met4", MetalLayer::M4 },
        { "met5", MetalLayer::M5 },
        { "met6", MetalLayer::M6 },
        { "met7", MetalLayer::M7 },
        { "met8", MetalLayer::M8 },
        { "met9", MetalLayer::M9 }
    };

    auto it = nameToMLMap.find(t_name);

    if (it != nameToMLMap.end()) {
        return it->second;
    }

    return MetalLayer::NONE;
}

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

// Class methods
// ======================================================================================

void Encoder::readDef(const std::string_view& t_fileName, const std::shared_ptr<Data>& t_data)
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

    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetComponentCbk(&defComponentCallback);
    defrSetPinCbk(&defPinCallback);
    defrSetNetCbk(&defNetCallback);
    defrSetSNetCbk(&defSpecialNetCallback);
    // defrSetViaCbk(&defViaCallback);

    // Read file
    //=================================================================

    int readStatus = defrRead(file, t_fileName.cbegin(), static_cast<void*>(t_data.get()), 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't parse a file: " + std::string(t_fileName));
    }

    fclose(file);
    defrClear();

    for (int32_t j = 0; j < t_data->numCellY; ++j) {
        for (int32_t i = 0; i < t_data->numCellX; ++i) {
            std::shared_ptr<WorkingCell> cell = t_data->cells[j][i];
            Point moveBy(i * t_data->cellSize + t_data->cellOffsetX - 1, j * t_data->cellSize + t_data->cellOffsetY - 1);

            s_threadPool.enqueue([](std::shared_ptr<WorkingCell>& cell, Point& moveBy) {
                for (const auto& pin : cell->pins) {
                    auto vertexes = pin.second->vertex;

                    for (auto& vertex : vertexes) {
                        vertex -= moveBy;
                    }

                    uint8_t layerIndex = static_cast<uint8_t>(pin.second->layer);

                    if (layerIndex != 0) {
                        cell->source[layerIndex].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = pin.second->netIndex;
                    }

                    cell->source[0].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = pin.second->netIndex;
                }
            },
                cell, moveBy);

            s_threadPool.enqueue([](std::shared_ptr<WorkingCell>& cell, Point& moveBy) {
                for (const auto& rout : cell->routes) {
                    auto vertexes = rout->vertex;

                    for (auto& vertex : vertexes) {
                        vertex -= moveBy;
                    }

                    uint8_t layerIndex = static_cast<uint8_t>(rout->layer);

                    cell->source[layerIndex].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 1.0;
                }
            },
                cell, moveBy);

            s_threadPool.enqueue([](std::shared_ptr<WorkingCell>& cell, Point& moveBy) {
                for (const auto& geom : cell->geometries) {
                    auto vertexes = geom->vertex;

                    for (auto& vertex : vertexes) {
                        vertex -= moveBy;
                    }

                    if (geom->type != RectangleType::DIEAREA) {
                        uint8_t layerIndex = static_cast<uint8_t>(geom->layer);

                        if (layerIndex <= 6) {
                            cell->source[layerIndex].slice(0, vertexes[0].y, vertexes[2].y).slice(1, vertexes[0].x, vertexes[2].x) = 1.0;
                        }
                    } else {
                        cell->source.slice(1, vertexes[0].y, vertexes[2].y).slice(2, vertexes[0].x, vertexes[2].x) = -1.0;
                    }
                }
            },
                cell, moveBy);
        }
    }

    s_threadPool.wait();
}

void Encoder::addToWorkingCells(const std::shared_ptr<Rectangle>& t_target, Data* t_data)
{
    static auto intersectionArea = [](const Rectangle& t_fl, const std::shared_ptr<Rectangle>& t_rl) {
        return std::array<int32_t, 4> {
            std::max(t_fl.vertex[0].x, t_rl->vertex[0].x),
            std::max(t_fl.vertex[0].y, t_rl->vertex[0].y),
            std::min(t_fl.vertex[2].x, t_rl->vertex[2].x),
            std::min(t_fl.vertex[2].y, t_rl->vertex[2].y),
        };
    };

    for (auto& point : t_target->vertex) {
        point = point / s_pdk.scale;
    }

    Point lt = (t_target->vertex[0] - Point(t_data->cellOffsetX, t_data->cellOffsetY)) / t_data->cellSize;
    Point rt = (t_target->vertex[1] - Point(t_data->cellOffsetX, t_data->cellOffsetY)) / t_data->cellSize;
    Point lb = (t_target->vertex[3] - Point(t_data->cellOffsetX, t_data->cellOffsetY)) / t_data->cellSize;

    switch (t_target->type) {
    case RectangleType::PIN: {
        std::shared_ptr<Pin> pin = std::static_pointer_cast<Pin>(t_target);

        int32_t largestArea {};
        Point pinCoords {};

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(matrixRect, t_target);
                int32_t area = (inter[0] - inter[2]) * (inter[1] - inter[3]);

                if (area > largestArea) {
                    largestArea = area;
                    pinCoords.x = i;
                    pinCoords.y = j;
                }
            }
        }

        if (t_data->pins.count(pin->name) == 0 || (t_data->pins.count(pin->name) != 0 && t_data->pins[pin->name]->parentCell == t_data->cells[pinCoords.y][pinCoords.x])) {
            for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
                for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                    Rectangle matrixRect = t_data->cells[j][i]->originalPlace;
                    std::array<int32_t, 4> inter = intersectionArea(matrixRect, t_target);

                    if (i == pinCoords.x && j == pinCoords.y) {
                        std::shared_ptr<Pin> pinCut = std::make_shared<Pin>(pin->name, inter[0], inter[1], inter[2], inter[3], t_target->layer);

                        pinCut->parentCell = t_data->cells[j][i];

                        t_data->pins[pin->name] = pinCut;
                        t_data->cells[j][i]->pins.insert(std::pair(pin->name, pinCut));

                        return;
                    }
                }
            }
        }
        break;
    }
    case RectangleType::SIGNAL: {
        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(matrixRect, t_target);
                std::shared_ptr<Rectangle> rect {};

                if (inter[0] != inter[2] && inter[1] != inter[3]) {
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                } else {
                    inter[2] = std::min(matrixRect.vertex[2].x, inter[2] + 1);
                    inter[3] = std::min(matrixRect.vertex[2].y, inter[3] + 1);
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                }

                t_data->cells[j][i]->routes.emplace_back(rect);
            }
        }
        break;
    }
    case RectangleType::CLOCK:
    case RectangleType::POWER:
    case RectangleType::GROUND:
    case RectangleType::DIEAREA: {
        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_data->cells[j][i]->originalPlace;
                std::array<int32_t, 4> inter = intersectionArea(matrixRect, t_target);
                std::shared_ptr<Rectangle> rect {};

                if (inter[0] != inter[2] && inter[1] != inter[3]) {
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                } else {
                    inter[2] = std::min(matrixRect.vertex[2].x, inter[2] + 1);
                    inter[3] = std::min(matrixRect.vertex[2].y, inter[3] + 1);
                    rect = std::make_shared<Rectangle>(inter[0], inter[1], inter[2], inter[3], t_target->layer, t_target->type);
                }

                t_data->cells[j][i]->geometries.emplace_back(rect);
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

int Encoder::defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
{
    if (t_type != defrCallbackType_e::defrDieAreaCbkType) {
        return 2;
    }

    defiPoints points = t_box->getPoint();
    Data* data = static_cast<Data*>(t_userData);

    s_pdk.scale = static_cast<int32_t>(s_pdk.scale * 1000.0);

    if (points.numPoints != 2) {
        for (std::size_t i = 0; i < points.numPoints; ++i) {
            data->dieArea.emplace_back(Point(points.x[i], points.y[i]) / s_pdk.scale);
        }
    } else {
        data->dieArea.emplace_back(Point(points.x[0], points.y[0]) / s_pdk.scale);
        data->dieArea.emplace_back(Point(points.x[1], points.y[0]) / s_pdk.scale);
        data->dieArea.emplace_back(Point(points.x[1], points.y[1]) / s_pdk.scale);
        data->dieArea.emplace_back(Point(points.x[0], points.y[1]) / s_pdk.scale);
        data->dieArea.emplace_back(Point(points.x[0], points.y[0]) / s_pdk.scale);
    }

    Point leftTop { INT32_MAX, INT32_MAX };
    Point rightBottom { 0, 0 };

    for (auto& point : data->dieArea) {
        leftTop.x = std::min(point.x, leftTop.x);
        leftTop.y = std::min(point.y, leftTop.y);
        rightBottom.x = std::max(point.x, rightBottom.x);
        rightBottom.y = std::max(point.y, rightBottom.y);
    }

    int32_t width = rightBottom.x - leftTop.x + 1;
    int32_t height = rightBottom.y - leftTop.y + 1;

    data->cellSize = 478;
    data->numCellX = std::ceil((width + 50.0) / data->cellSize);
    data->numCellY = std::ceil((height + 50.0) / data->cellSize);
    data->cellOffsetX = leftTop.x - 50;
    data->cellOffsetY = leftTop.y - 50;
    data->cells = std::vector<std::vector<std::shared_ptr<WorkingCell>>>(data->numCellY, std::vector<std::shared_ptr<WorkingCell>>(data->numCellX, nullptr));

    for (std::size_t j = 0; j < data->numCellY; ++j) {
        for (std::size_t i = 0; i < data->numCellX; ++i) {
            int32_t left = data->cellOffsetX + i * data->cellSize;
            int32_t top = data->cellOffsetY + j * data->cellSize;
            int32_t right = data->cellOffsetX + (i + 1) * data->cellSize;
            int32_t bottom = data->cellOffsetY + (j + 1) * data->cellSize;

            std::shared_ptr<WorkingCell> cell = std::make_shared<WorkingCell>();

            cell->originalPlace = Rectangle(left, top, right, bottom, MetalLayer::NONE);
            cell->source = torch::zeros({ 8, data->cellSize + 2, data->cellSize + 2 });
            cell->source.slice(1, 0, data->cellSize + 2).slice(2, 0, 1) = -1.0;
            cell->source.slice(1, 0, 1).slice(2, 0, data->cellSize + 2) = -1.0;

            if (i == data->numCellX - 1) {
                cell->source.slice(1, 0, data->cellSize + 2).slice(2, data->cellSize + 1, data->cellSize + 2) = -1.0;
            }

            if (j == data->numCellY - 1) {
                cell->source.slice(1, data->cellSize + 1, data->cellSize + 2).slice(2, 0, data->cellSize + 2) = -1.0;
            }

            data->cells[j][i] = cell;
        }
    }

    for (std::size_t i = 0; i < data->dieArea.size() - 1; ++i) {
        int32_t left = data->dieArea[i].x * s_pdk.scale;
        int32_t top = data->dieArea[i].y * s_pdk.scale;
        int32_t right = data->dieArea[i + 1].x * s_pdk.scale;
        int32_t bottom = data->dieArea[i + 1].y * s_pdk.scale;

        addToWorkingCells(std::make_shared<Rectangle>(left, top, right, bottom, MetalLayer::NONE, RectangleType::DIEAREA), data);
    }

    return 0;
}

int Encoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type != defrComponentCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);
    Point placed {};

    if (t_component->isFixed() || t_component->isPlaced()) {
        placed = Point(t_component->placementX(), t_component->placementY());
    }

    PDK::Macro macro = s_pdk.macros.at(t_component->name());
    PointF leftBottom = { INT32_MAX, INT32_MAX };
    PointF newLeftBottom { INT32_MAX, INT32_MAX };

    for (auto& pin : macro.pins) {
        for (auto& port : pin.ports) {
            for (auto& vertex : port.vertex) {
                vertex.x *= 1000.0;
                vertex.y *= 1000.0;

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
        bool isSignal = pin.use == "SIGNAL" && pin.name.find("clk", 0) == std::string::npos && std::string(t_component->id()).find("clk", 0) == std::string::npos;

        for (auto& port : pin.ports) {
            for (auto& vertex : port.vertex) {
                vertex.x += (leftBottom.x - newLeftBottom.x) + placed.x;
                vertex.y += (leftBottom.y - newLeftBottom.y) + placed.y;
            }

            Rectangle rect { port };

            if (isSignal) {
                addToWorkingCells(std::make_shared<Pin>(t_component->id() + pin.name, rect.vertex[0].x, rect.vertex[0].y, rect.vertex[2].x, rect.vertex[2].y, rect.layer), data);
            }
            // else {
            //     addToWorkingCells(std::make_shared<Rectangle>(rect), data);
            // }
        }
    }

    return 0;
};

int Encoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type != defrNetCbkType) {
        return 2;
    }

    RectangleType rType {};

    if (t_net->hasUse()) {
        if (strcmp(t_net->use(), "SIGNAL") == 0) {
            rType = RectangleType::SIGNAL;
        } else if (strcmp(t_net->use(), "CLOCK") == 0) {
            rType = RectangleType::CLOCK;
        }
    }

    Data* data = static_cast<Data*>(t_userData);

    if (rType == RectangleType::SIGNAL) {
        ++data->totalNets;

        std::vector<std::string> pinsNames(t_net->numConnections(), "");

        for (std::size_t i = 0; i < t_net->numConnections(); ++i) {
            pinsNames[i] = std::string(t_net->instance(i)) + t_net->pin(i);
        }

        Net net { data->totalNets, std::vector<int8_t>(pinsNames.size(), 0) };

        for (const auto& name : pinsNames) {
            std::shared_ptr<WorkingCell> cell = data->pins[name]->parentCell;

            if (cell->nets.count(net.index) == 0) {
                for (std::size_t i = 0; i < pinsNames.size(); ++i) {
                    auto range = cell->pins.equal_range(pinsNames[i]);

                    if (range.first != range.second) {
                        net.pins[i] = 1;

                        for (auto& it = range.first; it != range.second; ++it) {
                            it->second->netIndex = data->totalNets;
                        }
                    } else {
                        net.pins[i] = 0;
                    }
                }

                cell->nets[net.index] = net;
            }
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
                const char* layerName {};
                const char* viaName {};
                int32_t tokenType {};
                int32_t width {};
                int32_t extStart {};
                int32_t extEnd {};
                Point start {};
                Point end {};
                Rectangle rect {};

                while ((tokenType = wirePath->next()) != DEFIPATH_DONE) {
                    switch (tokenType) {
                    case DEFIPATH_LAYER:
                        layerName = wirePath->getLayer();
                        break;
                    case DEFIPATH_WIDTH:
                        width = wirePath->getWidth();
                        break;
                    case DEFIPATH_VIA:
                        viaName = wirePath->getVia();
                        break;
                    case DEFIPATH_POINT:
                        if (!isStartSet) {
                            isStartSet = true;
                            wirePath->getPoint(&start.x, &start.y);
                        } else {
                            isEndSet = true;
                            wirePath->getPoint(&end.x, &end.y);
                        }
                        break;
                    case DEFIPATH_FLUSHPOINT:
                        if (!isStartSet) {
                            isStartSet = true;
                            wirePath->getFlushPoint(&start.x, &start.y, &extStart);
                        } else {
                            isEndSet = true;
                            wirePath->getFlushPoint(&end.x, &end.y, &extEnd);
                        }
                        break;
                    case DEFIPATH_RECT: {
                        int32_t dx1 {};
                        int32_t dx2 {};
                        int32_t dy1 {};
                        int32_t dy2 {};

                        wirePath->getViaRect(&dx1, &dy1, &dx2, &dy2);

                        rect = Rectangle(dx1 + start.x, dy1 + start.y, dx2 + start.x, dy2 + start.y, convertNameToML(layerName), RectangleType::SIGNAL);
                        isViaRect = true;
                        break;
                    }
                    default:
                        break;
                    }
                }

                std::shared_ptr<Rectangle> routRect {};

                if (viaName == nullptr) {
                    if (isStartSet && isEndSet) {
                        if (start.x != end.x) {
                            routRect = std::make_shared<Rectangle>(start.x - extStart, start.y, end.x + extEnd, end.y, convertNameToML(layerName), rType);
                        } else {
                            routRect = std::make_shared<Rectangle>(start.x, start.y - extStart, end.x, end.y + extEnd, convertNameToML(layerName), rType);
                        }
                    } else if (isViaRect) {
                        routRect = std::make_shared<Rectangle>(rect);
                    }
                } else {
                    routRect = std::make_shared<Rectangle>(start.x, start.y, start.x, start.y, viaRuleToML(viaName), rType);
                }

                addToWorkingCells(routRect, data);
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

    Data* data = static_cast<Data*>(t_userData);

    for (std::size_t i = 0; i < t_net->numWires(); ++i) {
        defiWire* wire = t_net->wire(i);

        for (std::size_t j = 0; j < wire->numPaths(); ++j) {
            defiPath* wirePath = wire->path(j);
            wirePath->initTraverse();

            int tokenType {};
            int width {};
            bool isStartSet = false;
            const char* layerName {};
            const char* viaName {};
            Point start {};
            Point end {};

            while ((tokenType = wirePath->next()) != DEFIPATH_DONE) {
                switch (tokenType) {
                case DEFIPATH_LAYER:
                    layerName = wirePath->getLayer();
                case DEFIPATH_WIDTH:
                    width = wirePath->getWidth();
                    break;
                case DEFIPATH_VIA:
                    viaName = wirePath->getVia();
                    break;
                case DEFIPATH_POINT:
                    if (!isStartSet) {
                        isStartSet = true;
                        wirePath->getPoint(&start.x, &start.y);
                    } else {
                        wirePath->getPoint(&end.x, &end.y);
                    }
                    break;
                default:
                    break;
                }
            }

            if (!viaName) {
                std::shared_ptr<Rectangle> rect {};

                if (start.x != end.x) {
                    rect = std::make_shared<Rectangle>(start.x, start.y - width / 2.0, end.x, end.y + width / 2.0, convertNameToML(layerName), rType);
                } else {
                    rect = std::make_shared<Rectangle>(start.x - width / 2.0, start.y, end.x + width / 2.0, end.y, convertNameToML(layerName), rType);
                }

                addToWorkingCells(rect, data);
            }
            // else {
            //     auto it = data->vias.find(viaName);

            //     if (it != data->vias.end()) {
            //         for (auto instance : it->second) {
            //             for (auto& vertex : instance.vertex) {
            //                 vertex.x += start.x;
            //                 vertex.y += start.y;
            //             }

            //             std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(instance);

            //             addToWorkingCells(rect, data);
            //         }
            //     } else {
            //         throw std::runtime_error("Via used before it's declaration!");
            //     }
            // }
        }
    }

    return 0;
};

int Encoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type != defrPinCbkType) {
        return 2;
    }

    Data* data = static_cast<Data*>(t_userData);
    bool isSignal = strcmp(t_pin->use(), "SIGNAL") == 0 && std::string(t_pin->pinName()).find("clk", 0) == std::string::npos;

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

            MetalLayer layerName = convertNameToML(pinPort->layer(j));

            if (isSignal) {
                std::shared_ptr<Pin> pinRect = std::make_shared<Pin>("PIN" + std::string(t_pin->pinName()), xl, yl, xh, yh, layerName);

                addToWorkingCells(pinRect, data);
            }
            // else {
            //     std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xl, yl, xh, yh, layerName);

            //     addToWorkingCells(rect, data);
            // }
        }
    }

    return 0;
};

// int Encoder::defViaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData)
// {
//     if (t_type != defrViaCbkType) {
//         return 2;
//     }

//     char *viaRuleName {}, *botLayer {}, *cutLayer {}, *topLayer {};
//     int32_t xSize {}, ySize {}, xCutSpacing {}, yCutSpacing {}, xBotEnc {}, yBotEnc {}, xTopEnc {}, yTopEnc {}, numRow {}, numCol {};

//     t_via->viaRule(&viaRuleName, &xSize, &ySize, &botLayer, &cutLayer, &topLayer, &xCutSpacing, &yCutSpacing, &xBotEnc, &yBotEnc, &xTopEnc, &yTopEnc);
//     t_via->rowCol(&numRow, &numCol);

//     numRow = std::max(1, numRow);
//     numCol = std::max(1, numCol);

//     MetalLayer layer = viaRuleToML(viaRuleName);
//     std::vector<Rectangle> via {};

//     via.reserve(numRow * numCol);

//     for (int32_t i = 0; i < numRow; ++i) {
//         for (int32_t j = 0; j < numCol; ++j) {
//             int32_t xLeft = xCutSpacing * j + xSize * j - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
//             int32_t yTop = yCutSpacing * i + ySize * i - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;
//             int32_t xRight = xCutSpacing * j + xSize * (j + 1) - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
//             int32_t yBottom = yCutSpacing * i + ySize * (i + 1) - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;

//             via.emplace_back(Rectangle(xLeft, yTop, xRight, yBottom, layer));
//         }
//     }

//     Data* data = static_cast<Data*>(t_userData);

//     data->vias[t_via->name()] = via;

//     return 0;
// };