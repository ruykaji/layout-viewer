#include <algorithm>
#include <cmath>
#include <execution>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "encoder.hpp"

std::shared_ptr<Def> DEFEncoder::read(const std::string_view t_fileName)
{
    // Init session
    //=================================================================
    int initStatus = defrInitSession();

    if (initStatus != 0) {
        throw std::runtime_error("Error: cant't initialize parser!");
    }

    m_def = std::make_shared<Def>();

    // Settings
    //=================================================================
    defrSetAddPathToNet();

    // Set callbacks
    //=================================================================

    lefrSetPinCbk(&lefPinCallback);
    lefrSetObstructionCbk(&lefObstructionCallback);

    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetComponentCbk(&defComponentCallback);
    defrSetPinCbk(&defPinCallback);
    defrSetNetCbk(&defNetCallback);
    defrSetSNetCbk(&defSpecialNetCallback);
    defrSetViaCbk(&defViaCallback);

    // Open file
    //=================================================================

    auto file = fopen(t_fileName.cbegin(), "r");

    if (file == nullptr) {
        throw std::runtime_error("Error: Can't open a file!");
    }

    // Read file
    //=================================================================

    int readStatus = defrRead(file, t_fileName.cbegin(), static_cast<void*>(m_def.get()), 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't read a file!");
    }

    std::sort(m_def->geometries.begin(), m_def->geometries.end(), [](auto& t_left, auto& t_right) { return static_cast<int>(t_left->layer) < static_cast<int>(t_right->layer); });

    return m_def;
}

Geometry::ML DEFEncoder::convertNameToML(const char* t_name)
{
    if (strcmp(t_name, "li1") == 0) {
        return Geometry::ML::L1;
    } else if (strcmp(t_name, "met1") == 0) {
        return Geometry::ML::M1;
    } else if (strcmp(t_name, "met2") == 0) {
        return Geometry::ML::M2;
    } else if (strcmp(t_name, "met3") == 0) {
        return Geometry::ML::M3;
    } else if (strcmp(t_name, "met4") == 0) {
        return Geometry::ML::M4;
    } else if (strcmp(t_name, "met5") == 0) {
        return Geometry::ML::M5;
    } else if (strcmp(t_name, "met6") == 0) {
        return Geometry::ML::M6;
    } else if (strcmp(t_name, "met7") == 0) {
        return Geometry::ML::M7;
    } else if (strcmp(t_name, "met8") == 0) {
        return Geometry::ML::M8;
    } else if (strcmp(t_name, "met9") == 0) {
        return Geometry::ML::M9;
    }

    return Geometry::ML::NONE;
}

std::string DEFEncoder::findLef(const std::string& t_folder, const std::string& t_fileName)
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

    return t_folder + "/libraries/" + libName + "/latest/cells/" + cellName + "/" + t_fileName + ".lef";
}

void DEFEncoder::setGeomOrientation(const int8_t t_orientation, int32_t& t_x, int32_t& t_y)
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

int DEFEncoder::lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type == lefrPinCbkType) {
        Def* def = static_cast<Def*>(t_userData);
        std::shared_ptr<Pin> pin = std::make_shared<Pin>();
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
                    std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xLeft, yTop, xRight, yBottom, Rectangle::RType::PIN, convertNameToML(layer));

                    pin->rects.emplace_back(rect);
                    def->geometries.emplace_back(std::static_pointer_cast<Geometry>(rect));
                    break;
                }
                default:
                    break;
                }
            }
        }

        def->component.emplace_back(pin);
        def->pins[def->componentName + t_pin->name()] = pin;

        return 0;
    }

    return 2;
}

int DEFEncoder::lefObstructionCallback(lefrCallbackType_e t_type, lefiObstruction* t_obstruction, void* t_userData)
{
    if (t_type == lefrObstructionCbkType) {
        Def* def = static_cast<Def*>(t_userData);
        std::shared_ptr<Pin> obs = std::make_shared<Pin>(); // Not actual pin
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
                std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xLeft, yTop, xRight, yBottom, Rectangle::RType::PIN, convertNameToML(layer));

                obs->rects.emplace_back(rect);
                def->geometries.emplace_back(std::static_pointer_cast<Geometry>(rect));
                break;
            }
            default:
                break;
            }
        }

        def->component.emplace_back(obs);

        return 0;
    }

    return 2;
}

int DEFEncoder::defBlockageCallback(defrCallbackType_e t_type, defiBlockage* t_blockage, void* t_userData) {};

int DEFEncoder::defDieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
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
        def->matrixStepX = width / def->numMatrixX;
        def->matrixStepY = height / def->numMatrixY;
        def->matrixOffsetX = leftTop.x;
        def->matrixOffsetY = leftTop.y;

        for (std::size_t j = 0; j < def->numMatrixY; ++j) {
            for (std::size_t i = 0; i < def->numMatrixX; ++i) {
                Matrix matrix {};

                matrix.originalPlace = Rectangle(
                    def->matrixOffsetX + i * def->matrixStepX,
                    def->matrixOffsetY + j * def->matrixStepY,
                    def->matrixOffsetX + (i + 1) * def->matrixStepX,
                    def->matrixOffsetY + (j + 1) * def->matrixStepY,
                    Rectangle::RType::NONE,
                    Geometry::ML::NONE);

                def->matrixes.emplace_back(matrix);
            }
        }

        return 0;
    }

    return 2;
}

int DEFEncoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type == defrComponentCbkType) {
        Def* def = static_cast<Def*>(t_userData);

        def->componentName = t_component->id();

        std::string pathToCell = findLef("/home/alaie/stuff/skywater-pdk", t_component->name());

        int initStatus = lefrInit();

        if (initStatus != 0) {
            throw std::runtime_error("Error: cant't initialize parser!");
        }

        auto file = fopen(pathToCell.c_str(), "r");

        if (file == nullptr) {
            throw std::runtime_error("Error: Can't open a file: " + pathToCell);
        }

        int readStatus = lefrRead(file, pathToCell.c_str(), t_userData);

        if (readStatus != 0) {
            throw std::runtime_error("Error: Can't read a file: " + pathToCell);
        }

        // Component orientation and placement
        // =====================================================================

        Point leftBottom = { INT32_MAX, INT32_MAX };
        Point newLeftBottom { INT32_MAX, INT32_MAX };
        Point placed {};

        if (t_component->isFixed() || t_component->isPlaced()) {
            placed = Point(t_component->placementX(), t_component->placementY());
        }

        for (auto& pin : def->component) {
            for (auto& rect : pin->rects) {
                for (auto& vertex : rect->vertex) {
                    leftBottom.x = std::min(vertex.x, leftBottom.x);
                    leftBottom.y = std::min(vertex.y, leftBottom.y);

                    setGeomOrientation(t_component->placementOrient(), vertex.x, vertex.y);

                    newLeftBottom.x = std::min(vertex.x, newLeftBottom.x);
                    newLeftBottom.y = std::min(vertex.y, newLeftBottom.y);
                }
            }
        }

        for (auto& pin : def->component) {
            for (auto& rect : pin->rects) {
                for (auto& vertex : rect->vertex) {
                    vertex.x += (leftBottom.x - newLeftBottom.x) + placed.x;
                    vertex.y += (leftBottom.y - newLeftBottom.y) + placed.y;
                }
            }
        }

        def->component.clear();

        return 0;
    }

    return 2;
};

int DEFEncoder::defComponentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defDoubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData) {};

int DEFEncoder::defFillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defGcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData) {};

int DEFEncoder::defGroupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defIntegerCallback(defrCallbackType_e t_type, int t_number, void* t_userData) {};

int DEFEncoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
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
                    const char* layerName {};
                    const char* viaName {};
                    int32_t tokenType {};
                    int32_t width {};
                    int32_t extStart {};
                    int32_t extEnd {};
                    Point start {};
                    Point end {};

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
                        case DEFIPATH_RECT:
                            break;
                        default:
                            break;
                        }
                    }

                    if (viaName == nullptr) {
                        if (isStartSet && isEndSet) {
                            std::shared_ptr<Line> line {};

                            if (start.x != end.x) {
                                line = std::make_shared<Line>(start.x - extStart, start.y, end.x + extEnd, end.y, Line::LType::COMPONENT_ROUTE, convertNameToML(layerName));
                            } else {

                                line = std::make_shared<Line>(start.x, start.y - extStart, end.x, end.y + extEnd, Line::LType::COMPONENT_ROUTE, convertNameToML(layerName));
                            }

                            def->geometries.emplace_back(std::static_pointer_cast<Geometry>(line));
                        }
                    }
                }
            }
        }

        return 0;
    }

    return 2;
};

int DEFEncoder::defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
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
                        rect = std::make_shared<Rectangle>(start.x, start.y - width / 2.0, end.x, end.y + width / 2.0, Rectangle::RType::NONE, convertNameToML(layerName));
                    } else {
                        rect = std::make_shared<Rectangle>(start.x - width / 2.0, start.y, end.x + width / 2.0, end.y, Rectangle::RType::NONE, convertNameToML(layerName));
                    }

                    def->geometries.emplace_back(std::static_pointer_cast<Geometry>(rect));
                } else {
                    if (def->vias.count(viaName) != 0) {
                        Via via = def->vias[viaName];

                        for (auto& rect : via.rects) {
                            for (auto& vertex : rect.vertex) {
                                vertex.x += start.x;
                                vertex.y += start.y;
                            }

                            def->geometries.emplace_back(std::static_pointer_cast<Geometry>(std::make_shared<Rectangle>(rect)));
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

int DEFEncoder::defNonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData) {};

int DEFEncoder::defPathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData) {};

int DEFEncoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type == defrPinCbkType) {
        Def* def = static_cast<Def*>(t_userData);
        std::shared_ptr<Pin> pin = std::make_shared<Pin>();

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

                std::shared_ptr<Rectangle> rect = std::make_shared<Rectangle>(xl, yl, xh, yh, Rectangle::RType::PIN, convertNameToML(pinPort->layer(j)));

                pin->rects.emplace_back(rect);
                def->geometries.emplace_back(std::static_pointer_cast<Geometry>(rect));
            }
        }

        def->pins["PIN" + std::string(t_pin->pinName())] = pin;

        return 0;
    }

    return 2;
};

int DEFEncoder::defPropCallback(defrCallbackType_e t_type, defiProp* t_prop, void* t_userData) {};

int DEFEncoder::defRegionCallback(defrCallbackType_e t_type, defiRegion* t_region, void* t_userData) {};

int DEFEncoder::defRowCallback(defrCallbackType_e t_type, defiRow* t_row, void* t_userData) {};

int DEFEncoder::defScanchainCallback(defrCallbackType_e t_type, defiScanchain* t_scanchain, void* t_userData) {};

int DEFEncoder::defSlotCallback(defrCallbackType_e t_type, defiSlot* t_slot, void* t_userData) {};

int DEFEncoder::defStringCallback(defrCallbackType_e t_type, const char* t_string, void* t_userData) {};

int DEFEncoder::defStylesCallback(defrCallbackType_e t_type, defiStyles* t_style, void* t_userData) {};

int DEFEncoder::defTrackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData) {};

int DEFEncoder::defViaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData)
{
    if (t_type == defrViaCbkType) {
        Via via {};

        char *viaRuleName {}, *botLayer {}, *cutLayer {}, *topLayer {};
        int32_t xSize {}, ySize {}, xCutSpacing {}, yCutSpacing {}, xBotEnc {}, yBotEnc {}, xTopEnc {}, yTopEnc {}, numRow {}, numCol {};

        t_via->viaRule(&viaRuleName, &xSize, &ySize, &botLayer, &cutLayer, &topLayer, &xCutSpacing, &yCutSpacing, &xBotEnc, &yBotEnc, &xTopEnc, &yTopEnc);
        t_via->rowCol(&numRow, &numCol);

        if (numRow == 0 && numCol == 0) {
            numRow = 1;
            numCol = 1;
        }

        for (int32_t i = 0; i < numRow; ++i) {
            for (int32_t j = 0; j < numCol; ++j) {
                int32_t xLeft = xCutSpacing * j + xSize * j - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
                int32_t yTop = yCutSpacing * i + ySize * i - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;
                int32_t xRight = xCutSpacing * j + xSize * (j + 1) - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
                int32_t yBottom = yCutSpacing * i + ySize * (i + 1) - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;

                via.rects.emplace_back(Rectangle(xLeft, yTop, xRight, yBottom, Rectangle::RType::VIA, Geometry::ML::NONE));
            }
        }

        Def* def = static_cast<Def*>(t_userData);

        def->vias[t_via->name()] = via;

        return 0;
    }

    return 2;
};

int DEFEncoder::defVoidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};