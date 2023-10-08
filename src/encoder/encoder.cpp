#include <algorithm>
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
    m_def->components["__THIS__"] = Component();

    // Settings
    //=================================================================
    defrSetAddPathToNet();

    // Set callbacks
    //=================================================================

    lefrSetPinCbk(&lefPinCallback);

    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetGcellGridCbk(&defGcellGridCallback);
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

    // Post processing
    //=================================================================

    if (!m_def->gCellGrid.cells.empty()) {
        // ---> DieArea

        int32_t minX {};
        int32_t minY {};

        for (auto& [x, y] : m_def->dieArea.points) {
            minX = std::min(minX, x);
            minY = std::min(minY, y);
        }

        // ---> gCellGrid

        std::vector<Polygon> xLines {};

        for (int32_t x = m_def->gCellGrid.offsetX + minX; x != m_def->gCellGrid.maxX + minX; x += m_def->gCellGrid.stepX) {
            std::pair<int32_t, bool> yStart { 0, false };
            std::pair<int32_t, bool> yEnd { 0, false };

            for (auto i = m_def->dieArea.points.begin(); i != m_def->dieArea.points.end() - 1; ++i) {
                if (i->y == (i + 1)->y) {
                    if ((x >= i->x && x <= (i + 1)->x) || (x <= i->x && x >= (i + 1)->x)) {
                        if (!yStart.second) {
                            yStart.first = i->y;
                            yStart.second = true;
                        } else if (!yEnd.second) {
                            yEnd.first = i->y;
                            yEnd.second = true;
                            break;
                        }
                    }
                }
            }

            Polygon path;

            if (yStart < yEnd) {
                path.append(x, yStart.first);
                path.append(x, yEnd.first);

            } else {
                path.append(x, yEnd.first);
                path.append(x, yStart.first);
            }

            xLines.emplace_back(path);
        }

        std::vector<Polygon> yLines {};

        for (int32_t y = m_def->gCellGrid.offsetY + minY; y != m_def->gCellGrid.maxY + minY; y += m_def->gCellGrid.stepY) {
            std::pair<int32_t, bool> xStart { 0, false };
            std::pair<int32_t, bool> xEnd { 0, false };

            for (auto i = m_def->dieArea.points.begin(); i != m_def->dieArea.points.end() - 1; ++i) {
                if (i->x == (i + 1)->x) {
                    if ((y >= i->y && y <= (i + 1)->y) || (y <= i->y && y >= (i + 1)->y)) {
                        if (!xStart.second) {
                            xStart.first = i->x;
                            xStart.second = true;
                        } else if (!xEnd.second) {
                            xEnd.first = i->x;
                            xEnd.second = true;
                            break;
                        }
                    }
                }
            }

            Polygon path;

            if (xStart < xEnd) {
                path.append(xStart.first, y);
                path.append(xEnd.first, y);
            } else {
                path.append(xEnd.first, y);
                path.append(xStart.first, y);
            }

            yLines.emplace_back(path);
        }

        for (auto col = xLines.begin(); col != xLines.end() - 1; ++col) {
            for (auto row = yLines.begin(); row != yLines.end() - 1; ++row) {
                if ((col + 1)->points.at(1).y >= (row + 1)->points.at(0).y && col->points.at(0).x >= row->points.at(0).x) {
                    Polygon poly {};

                    poly.append(col->points.at(0).x, row->points.at(0).y);
                    poly.append((col + 1)->points.at(0).x, row->points.at(0).y);
                    poly.append((col + 1)->points.at(0).x, (row + 1)->points.at(0).y);
                    poly.append(col->points.at(0).x, (row + 1)->points.at(0).y);

                    m_def->gCellGrid.cells.emplace_back(poly);
                }
            }
        }
    }

    std::sort(m_def->polygon.begin(), m_def->polygon.end(), [](auto& t_left, auto& t_right) { return static_cast<int>(t_left->layer) < static_cast<int>(t_right->layer); });

    return m_def;
}

Polygon::ML DEFEncoder::convertNameToML(const char* t_name)
{
    if (strcmp(t_name, "li1") == 0) {
        return Polygon::ML::L1;
    } else if (strcmp(t_name, "met1") == 0) {
        return Polygon::ML::M1;
    } else if (strcmp(t_name, "met2") == 0) {
        return Polygon::ML::M2;
    } else if (strcmp(t_name, "met3") == 0) {
        return Polygon::ML::M3;
    } else if (strcmp(t_name, "met4") == 0) {
        return Polygon::ML::M4;
    } else if (strcmp(t_name, "met5") == 0) {
        return Polygon::ML::M5;
    } else if (strcmp(t_name, "met6") == 0) {
        return Polygon::ML::M6;
    } else if (strcmp(t_name, "met7") == 0) {
        return Polygon::ML::M7;
    } else if (strcmp(t_name, "met8") == 0) {
        return Polygon::ML::M8;
    } else if (strcmp(t_name, "met9") == 0) {
        return Polygon::ML::M9;
    }

    return Polygon::ML::NONE;
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

int DEFEncoder::lefPinCallback(lefrCallbackType_e t_type, lefiPin* t_pin, void* t_userData)
{
    if (t_type == lefrPinCbkType) {
        Def* def = static_cast<Def*>(t_userData);
        Component& component = def->components[def->lastComponentId];
        Point placed = component.placed;
        Pin pin {};

        for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
            lefiGeometries* portGeom = t_pin->port(i);
            Polygon::ML layer = convertNameToML(portGeom->getLayer(0));

            for (std::size_t j = 0; j < portGeom->numItems(); ++j) {
                switch (portGeom->itemType(j)) {
                case lefiGeomEnum::lefiGeomRectE: {
                    lefiGeomRect* rect = portGeom->getRect(j);
                    int32_t xLeft = rect->xl * 1000.0 + placed.x;
                    int32_t yTop = rect->yl * 1000.0 + placed.y;
                    int32_t xRight = rect->xh * 1000.0 + placed.x;
                    int32_t yBottom = rect->yh * 1000.0 + placed.y;
                    std::shared_ptr<Polygon> poly = std::make_shared<Polygon>(xLeft, yTop, xRight, yBottom, layer);

                    def->polygon.emplace_back(poly);
                    pin.polygons.emplace_back(poly);
                    break;
                }
                default:
                    break;
                }
            }
        }

        component.pins[t_pin->name()] = pin;

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

        Polygon poly {};

        if (points.numPoints != 2) {
            for (std::size_t i = 0; i < points.numPoints; ++i) {
                def->dieArea.append(points.x[i], points.y[i]);
            }
        } else {
            def->dieArea.append(points.x[0], points.y[0]);
            def->dieArea.append(points.x[1], points.y[0]);
            def->dieArea.append(points.x[1], points.y[1]);
            def->dieArea.append(points.x[0], points.y[1]);
            def->dieArea.append(points.x[0], points.y[0]);
        }

        return 0;
    }

    return 2;
}

int DEFEncoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData)
{
    if (t_type == defrComponentCbkType) {
        Component component {};

        if (t_component->isFixed() || t_component->isPlaced()) {
            component.placed = Point(t_component->placementX(), t_component->placementY());
        }

        Def* def = static_cast<Def*>(t_userData);

        def->lastComponentId = t_component->id();
        def->components[t_component->id()] = component;

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

        return 0;
    }

    return 2;
};

int DEFEncoder::defComponentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defDoubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData) {};

int DEFEncoder::defFillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defGcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData)
{
    if (t_type == defrCallbackType_e::defrGcellGridCbkType) {
        auto def = static_cast<Def*>(t_userData);
        auto macro = t_grid->macro();

        if (strcmp(macro, "X") == 0) {
            def->gCellGrid.offsetX = t_grid->x();
            def->gCellGrid.numX = t_grid->xNum();
            def->gCellGrid.stepX = t_grid->xStep();
            def->gCellGrid.maxX = def->gCellGrid.offsetX + (def->gCellGrid.numX + 1) * def->gCellGrid.stepX;

            return 0;
        } else if (strcmp(macro, "Y") == 0) {
            def->gCellGrid.offsetY = t_grid->x();
            def->gCellGrid.numY = t_grid->xNum();
            def->gCellGrid.stepY = t_grid->xStep();
            def->gCellGrid.maxY = def->gCellGrid.offsetY + (def->gCellGrid.numY + 1) * def->gCellGrid.stepY;

            return 0;
        }
    }

    return 2;
};

int DEFEncoder::defGroupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData) {};

int DEFEncoder::defIntegerCallback(defrCallbackType_e t_type, int t_number, void* t_userData) {};

int DEFEncoder::defNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type == defrNetCbkType) {
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
                std::string viaName {};
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
                        viaName = std::string(wirePath->getVia());
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

                if (viaName.empty()) {
                    std::shared_ptr<Polygon> poly = std::make_shared<Polygon>();

                    poly->layer = convertNameToML(layerName);

                    if (start.x != end.x) {
                        poly->append(start.x, start.y - width / 2.0);
                        poly->append(end.x, end.y - width / 2.0);
                        poly->append(end.x, end.y + width / 2.0);
                        poly->append(start.x, start.y + width / 2.0);
                    } else {
                        poly->append(start.x - width / 2.0, start.y);
                        poly->append(end.x - width / 2.0, end.y);
                        poly->append(end.x + width / 2.0, end.y);
                        poly->append(start.x + width / 2.0, start.y);
                    }

                    def->polygon.emplace_back(poly);
                } else {
                    if (def->vias.count(viaName) != 0) {
                        Via via = def->vias[viaName];
                        std::shared_ptr<Polygon> poly = std::make_shared<Polygon>();

                        for (auto& polygon : via.polygons) {
                            for (auto& point : polygon.points) {
                                poly->append(point.x + end.x, point.y + end.y);
                            }
                        }

                        def->polygon.emplace_back(poly);
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
        Pin pin {};

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

                std::shared_ptr<Polygon> poly = std::make_shared<Polygon>(xl, yl, xh, yh, convertNameToML(pinPort->layer(j)));

                def->polygon.emplace_back(poly);
                pin.polygons.emplace_back(poly);
            }
        }

        def->components["__THIS__"].pins[t_pin->pinName()] = pin;

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

        for (int32_t i = 0; i < numRow; ++i) {
            for (int32_t j = 0; j < numCol; ++j) {
                int32_t xLeft = xCutSpacing * j + xSize * j - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
                int32_t yTop = yCutSpacing * i + ySize * i - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;
                int32_t xRight = xCutSpacing * j + xSize * (j + 1) - (xSize * numCol + xCutSpacing * (numCol - 1)) / 2;
                int32_t yBottom = yCutSpacing * i + ySize * (i + 1) - (ySize * numRow + yCutSpacing * (numRow - 1)) / 2;

                via.polygons.emplace_back(Polygon(xLeft, yTop, xRight, yBottom, Polygon::ML::NONE));
            }
        }

        const char* name = t_via->name();
        Def* def = static_cast<Def*>(t_userData);

        def->vias[name] = via;

        return 0;
    }

    return 2;
};

int DEFEncoder::defVoidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};