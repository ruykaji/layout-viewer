#include <algorithm>
#include <cmath>
#include <execution>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "encoder.hpp"

// Static functions
// ======================================================================================

static ML convertNameToML(const char* t_name)
{
    if (strcmp(t_name, "li1") == 0) {
        return ML::L1;
    } else if (strcmp(t_name, "met1") == 0) {
        return ML::M1;
    } else if (strcmp(t_name, "met2") == 0) {
        return ML::M2;
    } else if (strcmp(t_name, "met3") == 0) {
        return ML::M3;
    } else if (strcmp(t_name, "met4") == 0) {
        return ML::M4;
    } else if (strcmp(t_name, "met5") == 0) {
        return ML::M5;
    } else if (strcmp(t_name, "met6") == 0) {
        return ML::M6;
    } else if (strcmp(t_name, "met7") == 0) {
        return ML::M7;
    } else if (strcmp(t_name, "met8") == 0) {
        return ML::M8;
    } else if (strcmp(t_name, "met9") == 0) {
        return ML::M9;
    }

    return ML::NONE;
}

static ML viaRuleToML(const char* t_name)
{
    std::string botLayer = { t_name[0], t_name[1] };
    std::string topLayer = { t_name[2], t_name[3] };

    if (botLayer == "L1" && topLayer == "M1") {
        return ML::L1M1_V;
    } else if (botLayer == "M1" && topLayer == "M2") {
        return ML::M1M2_V;
    } else if (botLayer == "M2" && topLayer == "M3") {
        return ML::M2M3_V;
    } else if (botLayer == "M3" && topLayer == "M4") {
        return ML::M3M4_V;
    } else if (botLayer == "M4" && topLayer == "M5") {
        return ML::M4M5_V;
    } else if (botLayer == "M5" && topLayer == "M6") {
        return ML::M5M6_V;
    } else if (botLayer == "M6" && topLayer == "M7") {
        return ML::M6M7_V;
    } else if (botLayer == "M7" && topLayer == "M8") {
        return ML::M7M8_V;
    } else if (botLayer == "M8" && topLayer == "M9") {
        return ML::M8M9_V;
    }

    return ML::NONE;
}

static void setGeomOrientation(const int8_t t_orientation, int32_t& t_x, int32_t& t_y)
{
    switch (t_orientation) {
    case 0:
        break;
    case 1:
        t_x = -t_y * sin(-M_PI_2);
        t_y = t_x * sin(-M_PI_2);
        break;
    case 2:
        t_x = t_x * cos(M_PI);
        t_y = t_y * cos(M_PI);
        break;
    case 3:
        t_x = -t_y * sin(M_PI_2);
        t_y = t_x * sin(M_PI_2);
        break;
    case 4:
        t_x = -t_x;
        break;
    case 5:
        t_x = t_y * sin(M_PI_2);
        t_y = t_x * sin(M_PI_2);
        break;
    case 6:
        t_y = -t_y;
        break;
    case 7:
        t_x = -t_y * sin(-M_PI_2);
        t_y = -t_x * sin(-M_PI_2);
        break;
    default:
        break;
    }
}

static void addToMatrices(const std::shared_ptr<Geometry>& t_target, Def* t_def)
{
    auto rect = std::static_pointer_cast<Rectangle>(t_target);

    Point lt = Point((rect->vertex[0].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[0].y - t_def->matrixOffsetY) / t_def->matrixStepY);
    Point rt = Point((rect->vertex[1].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[1].y - t_def->matrixOffsetY) / t_def->matrixStepY);
    Point rb = Point((rect->vertex[2].x - t_def->matrixOffsetX) / t_def->matrixStepX, (rect->vertex[2].y - t_def->matrixOffsetY) / t_def->matrixStepY);
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

static void readLef(const std::string& t_folder, const std::string& t_fileName)
{
    std::string libName {};
    std::string cellName {};
    auto itr = t_fileName.begin();

    for (; itr != t_fileName.end(); ++itr) {
        if (*itr == '_' && *(itr + 1) == '_') {
            itr += 2;
            break;
        }

        libName += *itr;
    }

    for (; itr != t_fileName.end(); ++itr) {
        // Hardcode for now
        if (*itr != '_') {
            cellName += *itr;
        } else {
            break;
        }
    }

    std::string pathToCell = t_folder + "/libraries/" + libName + "/latest/cells/" + cellName + "/" + t_fileName + ".lef";

    int initStatus = lefrInit();

    if (initStatus != 0) {
        throw std::runtime_error("Error: cant't initialize parser!");
    }

    auto file = fopen(pathToCell.c_str(), "r");

    if (file == nullptr) {
        throw std::runtime_error("Error: Can't open a file: " + pathToCell);
    }

    void* userData = lefrGetUserData();
    int readStatus = lefrRead(file, pathToCell.c_str(), userData);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't read a file: " + pathToCell);
    }
}

// Class methods
// ======================================================================================

void Encoder::readDef(const std::string_view t_fileName, const std::shared_ptr<Def> t_def)
{
    // Init session
    //=================================================================
    int initStatus = defrInitSession();

    if (initStatus != 0) {
        throw std::runtime_error("Error: cant't initialize parser!");
    }

    // Open file
    //=================================================================

    auto file = fopen(t_fileName.cbegin(), "r");

    if (file == nullptr) {
        throw std::runtime_error("Error: Can't open a file!");
    }

    // Settings
    //=================================================================

    // Def settings
    defrSetAddPathToNet();
    defrSetUserData(static_cast<void*>(t_def.get()));

    // Lef settings
    lefrSetUserData(static_cast<void*>(t_def.get()));

    // Set callbacks
    //=================================================================

    // Lef callbacks
    lefrSetPinCbk(&lefPinCallback);
    lefrSetObstructionCbk(&lefObstructionCallback);

    // Def callbacks
    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetComponentCbk(&defComponentCallback);
    defrSetPinCbk(&defPinCallback);
    defrSetNetCbk(&defNetCallback);
    defrSetSNetCbk(&defSpecialNetCallback);
    defrSetViaCbk(&defViaCallback);

    // Read file
    //=================================================================

    void* userData = defrGetUserData();
    int readStatus = defrRead(file, t_fileName.cbegin(), userData, 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't read a file!");
    }

    std::sort(t_def->geometries.begin(), t_def->geometries.end(), [](auto& t_left, auto& t_right) { return static_cast<int>(t_left->layer) < static_cast<int>(t_right->layer); });
}

int Encoder::lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type == lefrPinCbkType) {
        Def* def = static_cast<Def*>(t_userData);
        std::string pinName = def->componentName + t_pin->name();
        lefiGeometries* portGeom {};
        const char* layer {};

        for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
            portGeom = t_pin->port(i);

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

    return 2;
}

int Encoder::lefObstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData)
{
    if (t_type == lefrObstructionCbkType) {
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

    return 2;
}

int Encoder::defBlockageCallback(defrCallbackType_e t_type, defiBlockage* t_blockage, void* t_userData) {};

int Encoder::defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
{
    if (t_type == defrCallbackType_e::defrDieAreaCbkType) {
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

    return 2;
}

int Encoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type == defrComponentCbkType) {
        Def* def = static_cast<Def*>(t_userData);

        def->componentName = t_component->id();

        readLef("/home/alaie/stuff/skywater-pdk", t_component->name());

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
    }

    return 2;
};

int Encoder::defComponentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData) {};

int Encoder::defDoubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData) {};

int Encoder::defFillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData) {};

int Encoder::defGcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData) {};

int Encoder::defGroupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData) {};

int Encoder::defIntegerCallback(defrCallbackType_e t_type, int t_number, void* t_userData) {};

int Encoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type == defrNetCbkType) {
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

        std::string source = std::string(t_net->instance(0)) + std::string(t_net->pin(0));

        for (std::size_t i = 1; i < t_net->numConnections(); ++i) {
            def->pinConnections[source].emplace_back(std::string(t_net->instance(i)) + std::string(t_net->pin(i)));
            def->pinConnections[std::string(t_net->instance(i)) + std::string(t_net->pin(i))].emplace_back(source);
        }

        return 0;
    }

    return 2;
};

int Encoder::defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type == defrSNetCbkType) {

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
                } else {
                    if (def->vias.count(viaName) != 0) {
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
        }

        return 0;
    }

    return 2;
};

int Encoder::defNonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData) {};

int Encoder::defPathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData) {};

int Encoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type == defrPinCbkType) {
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
    }

    return 2;
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
    if (t_type == defrViaCbkType) {
        std::vector<Rectangle> via {};

        char *viaRuleName {}, *botLayer {}, *cutLayer {}, *topLayer {};
        int32_t xSize {}, ySize {}, xCutSpacing {}, yCutSpacing {}, xBotEnc {}, yBotEnc {}, xTopEnc {}, yTopEnc {}, numRow {}, numCol {};

        t_via->viaRule(&viaRuleName, &xSize, &ySize, &botLayer, &cutLayer, &topLayer, &xCutSpacing, &yCutSpacing, &xBotEnc, &yBotEnc, &xTopEnc, &yTopEnc);
        t_via->rowCol(&numRow, &numCol);

        if (numRow == 0 && numCol == 0) {
            numRow = 1;
            numCol = 1;
        }

        ML layer = viaRuleToML(viaRuleName);

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
    }

    return 2;
};

int Encoder::defVoidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};