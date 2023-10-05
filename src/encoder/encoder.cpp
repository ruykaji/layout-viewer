#include <algorithm>
#include <execution>
#include <filesystem>
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

    // Settings
    //=================================================================
    defrSetAddPathToNet();

    // Set callbacks
    //=================================================================

    defrSetDieAreaCbk(&defDieAreaCallback);
    defrSetGcellGridCbk(&defGcellGridCallback);
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

    m_def = std::make_shared<Def>();

    // Read file
    //=================================================================

    int readStatus = defrRead(file, t_fileName.cbegin(), static_cast<void*>(m_def.get()), 0);

    if (readStatus != 0) {
        throw std::runtime_error("Error: Can't read a file!");
    }

    // Post processing
    //=================================================================

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
            if (i->second == (i + 1)->second) {
                if ((x >= i->first && x <= (i + 1)->first) || (x <= i->first && x >= (i + 1)->first)) {
                    if (!yStart.second) {
                        yStart.first = i->second;
                        yStart.second = true;
                    } else if (!yEnd.second) {
                        yEnd.first = i->second;
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
            if (i->first == (i + 1)->first) {
                if ((y >= i->second && y <= (i + 1)->second) || (y <= i->second && y >= (i + 1)->second)) {
                    if (!xStart.second) {
                        xStart.first = i->first;
                        xStart.second = true;
                    } else if (!xEnd.second) {
                        xEnd.first = i->first;
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
            if ((col + 1)->points.at(1).second >= (row + 1)->points.at(0).second && col->points.at(0).first >= row->points.at(0).first) {
                Polygon poly {};

                poly.append(col->points.at(0).first, row->points.at(0).second);
                poly.append((col + 1)->points.at(0).first, row->points.at(0).second);
                poly.append((col + 1)->points.at(0).first, (row + 1)->points.at(0).second);
                poly.append(col->points.at(0).first, (row + 1)->points.at(0).second);

                m_def->gCellGrid.cells.emplace_back(poly);
            }
        }
    }

    return m_def;
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
        cellName += *itr;
    }

    std::string pathToLef = t_folder + "/libraries/" + libName + "/" + cellName + "/" + t_fileName + ".lef";
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

int DEFEncoder::defComponentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData) {};

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

    return 0;
};

int DEFEncoder::defSpecialNetCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData)
{
    if (t_type == defrSNetCbkType) {

        Def* def = static_cast<Def*>(t_userData);

        for (std::size_t i = 0; i < t_net->numWires(); ++i) {
            defiWire* wire = t_net->wire(i);

            for (std::size_t j = 0; j < wire->numPaths(); ++j) {
                defiPath* wirePath = wire->path(j);
                Path path {};
                int tokenType {};

                wirePath->initTraverse();

                while ((tokenType = (int)wirePath->next()) != DEFIPATH_DONE) {
                    switch (tokenType) {
                    case DEFIPATH_VIA:
                        path.viaName = std::string(wirePath->getVia());
                        break;
                    case DEFIPATH_WIDTH:
                        path.width = wirePath->getWidth();
                        break;
                    case DEFIPATH_POINT:
                        if (!path.isStartSet) {
                            wirePath->getPoint(&path.start.first, &path.start.second);
                            wirePath->getPoint(&path.end.first, &path.end.second);
                            path.isStartSet = true;
                        } else {
                            wirePath->getPoint(&path.end.first, &path.end.second);
                        }
                        break;
                    default:
                        break;
                    }
                }

                def->paths.emplace_back(path);
            }
        }

        return 0;
    }

    return 0;
};

int DEFEncoder::defNonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData) {};

int DEFEncoder::defPathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData) {};

int DEFEncoder::defPinCallback(defrCallbackType_e t_type, defiPin* t_pin, void* t_userData)
{
    if (t_type == defrPinCbkType) {
        Pin pin {};

        for (std::size_t i = 0; i < t_pin->numPorts(); ++i) {
            defiPinPort* pinPort = t_pin->pinPort(i);
            Port port {};
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

                port.polygons.emplace_back(Polygon(xl, yl, xh, yh));
            }

            pin.ports.emplace_back(port);
        }

        Def* def = static_cast<Def*>(t_userData);

        def->pins.emplace_back(pin);

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

                via.polygons.emplace_back(Polygon(xLeft, yTop, xRight, yBottom));
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