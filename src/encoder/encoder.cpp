#include <algorithm>
#include <cmath>
#include <execution>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "encoder.hpp"

// General functions
// ======================================================================================

inline static ML convertNameToML(const char* t_name)
{
    static const std::unordered_map<std::string, ML> nameToMLMap = {
        { "li1", ML::L1 },
        { "met1", ML::M1 },
        { "met2", ML::M2 },
        { "met3", ML::M3 },
        { "met4", ML::M4 },
        { "met5", ML::M5 },
        { "met6", ML::M6 },
        { "met7", ML::M7 },
        { "met8", ML::M8 },
        { "met9", ML::M9 }
    };

    auto it = nameToMLMap.find(t_name);

    if (it != nameToMLMap.end()) {
        return it->second;
    }

    return ML::NONE;
}

inline static ML viaRuleToML(const char* t_name)
{
    static const std::unordered_map<std::string, ML> layerToMLMap = {
        { "L1M1", ML::L1M1_V },
        { "M1M2", ML::M1M2_V },
        { "M2M3", ML::M2M3_V },
        { "M3M4", ML::M3M4_V },
        { "M4M5", ML::M4M5_V },
        { "M5M6", ML::M5M6_V },
        { "M6M7", ML::M6M7_V },
        { "M7M8", ML::M7M8_V },
        { "M8M9", ML::M8M9_V }
    };

    std::string combinedLayer = { t_name[0], t_name[1], t_name[2], t_name[3] };

    auto it = layerToMLMap.find(combinedLayer);

    if (it != layerToMLMap.end()) {
        return it->second;
    }

    return ML::NONE;
}

inline static void setGeomOrientation(const int8_t t_orientation, int32_t& t_x, int32_t& t_y)
{
    int32_t temp_x {}, temp_y {};

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

inline static void addToMatrices(const std::shared_ptr<Geometry>& t_target, Def* t_def)
{
    auto rect = std::static_pointer_cast<Rectangle>(t_target);

    Point lt = Point((rect->vertex[0].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[0].y - t_def->matrixOffsetY) / t_def->matrixStepY);
    Point rt = Point((rect->vertex[1].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[1].y - t_def->matrixOffsetY) / t_def->matrixStepY);
    Point lb = Point((rect->vertex[3].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[3].y - t_def->matrixOffsetY) / t_def->matrixStepY);

    if (rect->rType == RType::PIN) {
        auto pin = std::static_pointer_cast<Pin>(rect);
        int32_t largestArea {};
        Point pinCoors {};

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_def->matrixes[j][i].originalPlace;

                int32_t ltX = std::max(matrixRect.vertex[0].x, rect->vertex[0].x);
                int32_t rbX = std::min(matrixRect.vertex[2].x, rect->vertex[2].x);
                int32_t ltY = std::max(matrixRect.vertex[0].y, rect->vertex[0].y);
                int32_t rbY = std::min(matrixRect.vertex[2].y, rect->vertex[2].y);
                int32_t area = (rbX - ltX) * (rbY - ltY);

                if (area > largestArea) {
                    largestArea = area;
                    pinCoors.x = i;
                    pinCoors.y = j;
                }
            }
        }

        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_def->matrixes[j][i].originalPlace;

                int32_t ltX = std::max(matrixRect.vertex[0].x, rect->vertex[0].x);
                int32_t rbX = std::min(matrixRect.vertex[2].x, rect->vertex[2].x);
                int32_t ltY = std::max(matrixRect.vertex[0].y, rect->vertex[0].y);
                int32_t rbY = std::min(matrixRect.vertex[2].y, rect->vertex[2].y);

                if (i == pinCoors.x && j == pinCoors.y) {
                    t_def->matrixes[j][i].geometries.emplace_back(std::make_shared<Pin>(pin->name, ltX, ltY, rbX, rbY, rect->layer));
                } else {
                    t_def->matrixes[j][i].geometries.emplace_back(std::make_shared<Rectangle>(ltX, ltY, rbX, rbY, RType::NONE, rect->layer));
                }
            }
        }
    } else {
        for (std::size_t i = lt.x; i < rt.x + 1; ++i) {
            for (std::size_t j = lt.y; j < lb.y + 1; ++j) {
                Rectangle matrixRect = t_def->matrixes[j][i].originalPlace;

                int32_t ltX = std::max(matrixRect.vertex[0].x, rect->vertex[0].x);
                int32_t rbX = std::min(matrixRect.vertex[2].x, rect->vertex[2].x);
                int32_t ltY = std::max(matrixRect.vertex[0].y, rect->vertex[0].y);
                int32_t rbY = std::min(matrixRect.vertex[2].y, rect->vertex[2].y);

                t_def->matrixes[j][i].geometries.emplace_back(std::make_shared<Rectangle>(ltX, ltY, rbX, rbY, RType::NONE, rect->layer));
            }
        }
    }
}

// Class methods
// ======================================================================================

void Encoder::readDef(const std::string_view t_fileName, const std::shared_ptr<Def> t_def)
{
    // Init session
    //=================================================================
    int initStatusDef = defrInitSession();

    if (initStatusDef != 0) {
        throw std::runtime_error("Error: cant't initialize def parser!");
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
    defrSetComponentStartCbk(&defComponentStartCallback);
    defrSetComponentCbk(&defComponentCallback);
    defrSetComponentEndCbk(&defComponentEndCallback);
    defrSetPinCbk(&defPinCallback);
    defrSetNetCbk(&defNetCallback);
    defrSetSNetCbk(&defSpecialNetCallback);
    defrSetViaCbk(&defViaCallback);

    // Read file
    //=================================================================

    int readStatus = defrRead(file, t_fileName.cbegin(), static_cast<void*>(t_def.get()), 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't parse a file: " + std::string(t_fileName));
    }

    fclose(file);
    defrClear();
}

// Lef callbacks
// ==================================================================================================================================================

int Encoder::lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type != lefrPinCbkType) {
        return 2;
    }

    Def* def = static_cast<Def*>(t_userData);
    std::string pinName = def->componentName + t_pin->name();
    const char* layer {};

    for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
        lefiGeometries* portGeom = t_pin->port(i);

        for (std::size_t j = 0; j < portGeom->numItems(); ++j) {
            switch (portGeom->itemType(j)) {
            case lefiGeomEnum::lefiGeomLayerE: {
                layer = portGeom->getLayer(j);
                break;
            }
            case lefiGeomEnum::lefiGeomRectE: {
                lefiGeomRect* portRect = portGeom->getRect(j);
                int32_t xLeft = portRect->xl * 1000.0;
                int32_t yTop = portRect->yl * 1000.0;
                int32_t xRight = portRect->xh * 1000.0;
                int32_t yBottom = portRect->yh * 1000.0;

                std::shared_ptr<Pin> pinRect = std::make_shared<Pin>(pinName, xLeft, yTop, xRight, yBottom, convertNameToML(layer));

                def->component.emplace_back(pinRect);
                def->geometries.emplace_back(pinRect);
                break;
            }
            default:
                break;
            }
        }
    }

    return 0;
}

int Encoder::lefObstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData)
{
    if (t_type != lefrObstructionCbkType) {
        return 2;
    }

    Def* def = static_cast<Def*>(t_userData);
    lefiGeometries* osbrGeom = t_obstruction->geometries();
    const char* layer {};

    for (std::size_t i = 0; i < osbrGeom->numItems(); ++i) {
        switch (osbrGeom->itemType(i)) {
        case lefiGeomEnum::lefiGeomLayerE: {
            layer = osbrGeom->getLayer(i);
            break;
        }
        case lefiGeomEnum::lefiGeomRectE: {
            lefiGeomRect* portRect = osbrGeom->getRect(i);
            int32_t xLeft = portRect->xl * 1000.0;
            int32_t yTop = portRect->yl * 1000.0;
            int32_t xRight = portRect->xh * 1000.0;
            int32_t yBottom = portRect->yh * 1000.0;

            std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xLeft, yTop, xRight, yBottom, RType::NONE, convertNameToML(layer));

            def->component.emplace_back(rect);
            def->geometries.emplace_back(rect);
            break;
        }
        default:
            break;
        }
    }

    return 0;
}

// Def callbacks
// ==================================================================================================================================================

int Encoder::defBlockageCallback(defrCallbackType_e t_type, defiBlockage* t_blockage, void* t_userData) {};

int Encoder::defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
{
    if (t_type != defrCallbackType_e::defrDieAreaCbkType) {
        return 2;
    }

    auto points = t_box->getPoint();
    auto def = static_cast<Def*>(t_userData);

    if (points.numPoints != 2) {
        for (std::size_t i = 0; i < points.numPoints; ++i) {
            def->dieArea.emplace_back(Point(points.x[i], points.y[i]));
        }
    } else {
        def->dieArea.emplace_back(Point(points.x[0], points.y[0]));
        def->dieArea.emplace_back(Point(points.x[1], points.y[0]));
        def->dieArea.emplace_back(Point(points.x[1], points.y[1]));
        def->dieArea.emplace_back(Point(points.x[0], points.y[1]));
        def->dieArea.emplace_back(Point(points.x[0], points.y[0]));
    }

    Point leftTop { INT32_MAX, INT32_MAX };
    Point rightBottom { 0, 0 };

    for (auto& point : def->dieArea) {
        leftTop.x = std::min(point.x, leftTop.x);
        leftTop.y = std::min(point.y, leftTop.y);
        rightBottom.x = std::max(point.x, rightBottom.x);
        rightBottom.y = std::max(point.y, rightBottom.y);
    }

    uint32_t width = rightBottom.x - leftTop.x;
    uint32_t height = rightBottom.y - leftTop.y;
    uint32_t scaledWidth = width / 20;
    uint32_t scaledHeight = height / 20;

    def->matrixSize = 480;
    def->numMatrixX = scaledWidth / def->matrixSize;
    def->numMatrixY = scaledHeight / def->matrixSize;
    def->matrixStepX = (width + 1000) / def->numMatrixX;
    def->matrixStepY = (height + 1000) / def->numMatrixY;
    def->matrixOffsetX = leftTop.x - 500;
    def->matrixOffsetY = leftTop.y - 500;
    def->matrixes = std::vector<std::vector<Matrix>>(def->numMatrixY, std::vector<Matrix>(def->numMatrixX, Matrix()));

    for (std::size_t j = 0; j < def->numMatrixY; ++j) {
        for (std::size_t i = 0; i < def->numMatrixX; ++i) {
            Matrix matrix {};

            matrix.originalPlace = Rectangle(
                def->matrixOffsetX + i * def->matrixStepX,
                def->matrixOffsetY + j * def->matrixStepY,
                def->matrixOffsetX + (i + 1) * def->matrixStepX,
                def->matrixOffsetY + (j + 1) * def->matrixStepY,
                RType::NONE, ML::NONE);

            def->matrixes[j][i] = matrix;
        }
    }

    return 0;
}

int Encoder::defComponentStartCallback(defrCallbackType_e t_type, int t_number, void* t_userData)
{
    if (t_type != defrComponentStartCbkType) {
        return 2;
    }

    // Init lef parser
    // =====================================================================

    int initStatusLef = lefrInitSession();

    if (initStatusLef != 0) {
        throw std::runtime_error("Error: cant't initialize lef parser!");
    }

    // Set lef callbacks
    // =====================================================================

    lefrSetPinCbk(&lefPinCallback);
    lefrSetObstructionCbk(&lefObstructionCallback);

    return 0;
}

int Encoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type != defrComponentCbkType) {
        return 2;
    }

    Def* def = static_cast<Def*>(t_userData);

    def->componentName = t_component->id();

    // Find path to cell
    // =====================================================================

    std::string fileName { t_component->name() };
    std::string libName {};
    std::string cellName {};

    auto itr = fileName.begin();

    for (; itr != fileName.end(); ++itr) {
        if (*itr == '_' && *(itr + 1) == '_') {
            itr += 2;
            break;
        }

        libName += *itr;
    }

    for (; itr != fileName.end(); ++itr) {
        if (*itr != '_') {
            cellName += *itr;
        } else {
            break;
        }
    }

    std::string pathToCell { "/home/alaie/stuff/skywater-pdk/libraries/" + libName + "/latest/cells/" + cellName + "/" + fileName + ".lef" };

    // Open file and parse
    // =====================================================================

    auto file = fopen(pathToCell.c_str(), "r");

    if (file == nullptr) {
        throw std::runtime_error("Error: Can't open a file: " + pathToCell);
    }

    int readStatus = lefrRead(file, pathToCell.c_str(), def);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't read a file: " + pathToCell);
    }

    fclose(file);

    // Component orientation and placement
    // =====================================================================

    Point leftBottom = { INT32_MAX, INT32_MAX };
    Point newLeftBottom { INT32_MAX, INT32_MAX };
    Point placed {};

    if (t_component->isFixed() || t_component->isPlaced()) {
        placed = Point(t_component->placementX(), t_component->placementY());
    }

    for (const auto& rect : def->component) {
        for (auto& vertex : rect->vertex) {
            leftBottom.x = std::min(vertex.x, leftBottom.x);
            leftBottom.y = std::min(vertex.y, leftBottom.y);

            setGeomOrientation(t_component->placementOrient(), vertex.x, vertex.y);

            newLeftBottom.x = std::min(vertex.x, newLeftBottom.x);
            newLeftBottom.y = std::min(vertex.y, newLeftBottom.y);
        }

        rect->fixVertex();
    }

    for (const auto& rect : def->component) {
        for (auto& vertex : rect->vertex) {
            vertex.x += (leftBottom.x - newLeftBottom.x) + placed.x;
            vertex.y += (leftBottom.y - newLeftBottom.y) + placed.y;
        }

        addToMatrices(rect, def);
    }

    def->component.clear();

    return 0;    
};

int Encoder::defComponentEndCallback(defrCallbackType_e t_type, void* t, void* t_userData)
{
    if (t_type != defrComponentEndCbkType) {
        return 2;
    }

    lefrClear();

    return 0;
}

int Encoder::defComponentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData) {};

int Encoder::defDoubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData) {};

int Encoder::defFillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData) {};

int Encoder::defGcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData) {};

int Encoder::defGroupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData) {};

int Encoder::defIntegerCallback(defrCallbackType_e t_type, int t_number, void* t_userData) {};

int Encoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type != defrNetCbkType) {
        return 2;
    }

    Def* def = static_cast<Def*>(t_userData);

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

                        rect = Rectangle(dx1 + start.x, dy1 + start.y, dx2 + start.x, dy2 + start.y, RType::NONE, convertNameToML(layerName));
                        isViaRect = true;
                        break;
                    }
                    default:
                        break;
                    }
                }

                if (viaName == nullptr) {
                    if (isStartSet && isEndSet) {
                        std::shared_ptr<Rectangle> line {};

                        if (start.x != end.x) {
                            line = std::make_shared<Rectangle>(start.x - extStart, start.y, end.x + extEnd + 1, end.y + 1, RType::NONE, convertNameToML(layerName));
                        } else {
                            line = std::make_shared<Rectangle>(start.x, start.y - extStart, end.x + 1, end.y + extEnd + 1, RType::NONE, convertNameToML(layerName));
                        }

                        def->geometries.emplace_back(line);

                        addToMatrices(line, def);
                    } else if (isViaRect) {
                        std::shared_ptr<Rectangle> viaRect = std::make_shared<Rectangle>(rect);

                        def->geometries.emplace_back(viaRect);

                        addToMatrices(viaRect, def);
                    }
                } else {
                    std::shared_ptr<Rectangle> via = std::make_shared<Rectangle>(start.x, start.y, start.x + 1, start.y + 1, RType::NONE, viaRuleToML(viaName));

                    def->geometries.emplace_back(via);

                    addToMatrices(via, def);
                }
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

    Def* def = static_cast<Def*>(t_userData);

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

            if (viaName == nullptr) {
                std::shared_ptr<Rectangle> rect {};

                if (start.x != end.x) {
                    rect = std::make_shared<Rectangle>(start.x, start.y - width / 2.0, end.x, end.y + width / 2.0, RType::NONE, convertNameToML(layerName));
                } else {
                    rect = std::make_shared<Rectangle>(start.x - width / 2.0, start.y, end.x + width / 2.0, end.y, RType::NONE, convertNameToML(layerName));
                }

                def->geometries.emplace_back(rect);

                addToMatrices(rect, def);
            } else if (def->vias.count(viaName) != 0) {
                std::vector<Rectangle> via = def->vias[viaName];

                for (auto& instance : via) {
                    for (auto& vertex : instance.vertex) {
                        vertex.x += start.x;
                        vertex.y += start.y;
                    }

                    std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(instance);

                    def->geometries.emplace_back(rect);

                    addToMatrices(rect, def);
                }

            } else {
                throw std::runtime_error("Via used before it's declaration!");
            }
        }
    }

    return 0;
};

int Encoder::defNonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData) {};

int Encoder::defPathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData) {};

int Encoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type != defrPinCbkType) {
        return 2;
    }

    Def* def = static_cast<Def*>(t_userData);

    for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
        defiPinPort* pinPort = t_pin->pinPort(i);
        int32_t xl {}, yl {}, xh {}, yh {};

        for (std::size_t j = 0; j < pinPort->numLayer(); ++j) {
            pinPort->bounds(j, &xl, &yl, &xh, &yh);

            if (pinPort->isPlaced() || pinPort->isFixed() || pinPort->isCover()) {
                int32_t xPlacement = pinPort->placementX();
                int32_t yPlacement = pinPort->placementY();

                xl += xPlacement;
                yl += yPlacement;
                xh += xPlacement;
                yh += yPlacement;
            }

            std::shared_ptr<Pin> pinRect = std::make_shared<Pin>("PIN" + std::string(t_pin->pinName()), xl, yl, xh, yh, convertNameToML(pinPort->layer(j)));

            def->geometries.emplace_back(pinRect);

            addToMatrices(pinRect, def);
        }
    }

    return 0;
};

int Encoder::defPropCallback(defrCallbackType_e t_type, defiProp* t_prop, void* t_userData) {};

int Encoder::defRegionCallback(defrCallbackType_e t_type, defiRegion* t_region, void* t_userData) {};

int Encoder::defRowCallback(defrCallbackType_e t_type, defiRow* t_row, void* t_userData) {};

int Encoder::defScanchainCallback(defrCallbackType_e t_type, defiScanchain* t_scanchain, void* t_userData) {};

int Encoder::defSlotCallback(defrCallbackType_e t_type, defiSlot* t_slot, void* t_userData) {};

int Encoder::defStringCallback(defrCallbackType_e t_type, const char* t_string, void* t_userData) {};

int Encoder::defStylesCallback(defrCallbackType_e t_type, defiStyles* t_style, void* t_userData) {};

int Encoder::defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData) {};

int Encoder::defViaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData)
{
    if (t_type != defrViaCbkType) {
        return 2;
    }

    char *viaRuleName {}, *botLayer {}, *cutLayer {}, *topLayer {};
    int32_t xSize {}, ySize {}, xCutSpacing {}, yCutSpacing {}, xBotEnc {}, yBotEnc {}, xTopEnc {}, yTopEnc {}, numRow {}, numCol {};

    t_via->viaRule(&viaRuleName, &xSize, &ySize, &botLayer, &cutLayer, &topLayer, &xCutSpacing, &yCutSpacing, &xBotEnc, &yBotEnc, &xTopEnc, &yTopEnc);
    t_via->rowCol(&numRow, &numCol);

    numRow = std::max(1, numRow);
    numCol = std::max(1, numCol);

    ML layer = viaRuleToML(viaRuleName);
    std::vector<Rectangle> via {};

    for (int32_t i = 0; i < numRow; ++i) {
        for (int32_t j = 0; j < numCol; ++j) {
            int32_t xLeft = xCutSpacing * j + xSize * j - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
            int32_t yTop = yCutSpacing * i + ySize * i - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;
            int32_t xRight = xCutSpacing * j + xSize * (j + 1) - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
            int32_t yBottom = yCutSpacing * i + ySize * (i + 1) - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;

            via.emplace_back(Rectangle(xLeft, yTop, xRight, yBottom, RType::NONE, layer));
        }
    }

    Def* def = static_cast<Def*>(t_userData);

    def->vias[t_via->name()] = via;

    return 0;
};

int Encoder::defVoidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};