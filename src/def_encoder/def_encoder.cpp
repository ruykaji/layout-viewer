#include <iostream>
#include <stdexcept>

#include "def_encoder.hpp"

std::shared_ptr<Def> DEFEncoder::read(const std::string_view t_fileName)
{
    // Init session
    //=================================================================
    int initStatus = defrInitSession();

    if (initStatus != 0) {
        throw std::runtime_error("Error: cant't initialize parser!");
    }

    // Set callbacks
    //=================================================================

    defrSetDieAreaCbk(&dieAreaCallback);
    defrSetGcellGridCbk(&gcellGridCallback);

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

    return m_def;
}

int DEFEncoder::blockageCallback(defrCallbackType_e t_type, defiBlockage* t_blockage, void* t_userData) {};

int DEFEncoder::dieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData)
{
    if (t_type == defrCallbackType_e::defrDieAreaCbkType) {
        auto points = t_box->getPoint();
        auto def = static_cast<Def*>(t_userData);

        Polygon poly {};

        for (std::size_t i = 0; i < points.numPoints; ++i) {
            def->dieArea.append(points.x[i], points.y[i]);
        }

        return 0;
    }

    return 2;
}

int DEFEncoder::componentCallback(defrCallbackType_e t_type, defiComponent* t_component, void* t_userData) {};

int DEFEncoder::componentMaskShiftLayerCallback(defrCallbackType_e t_type, defiComponentMaskShiftLayer* t_shiftLayers, void* t_userData) {};

int DEFEncoder::doubleCallback(defrCallbackType_e t_type, double* t_number, void* t_userData) {};

int DEFEncoder::fillCallback(defrCallbackType_e t_type, defiFill* t_shiftLayers, void* t_userData) {};

int DEFEncoder::gcellGridCallback(defrCallbackType_e t_type, defiGcellGrid* t_grid, void* t_userData)
{
    if (t_type == defrCallbackType_e::defrGcellGridCbkType) {
        auto def = static_cast<Def*>(t_userData);
        auto macro = t_grid->macro();
        auto step = t_grid->xStep();
        auto start = t_grid->x();

        if (strcmp(macro, "X") == 0) {
            for (std::size_t i = 0; i < t_grid->xNum(); ++i) {
                Polygon poly;

                poly.append(start + i * step, 0);
                def->gCellGridX.emplace_back(poly);
            }

            return 0;
        }

        if (strcmp(macro, "Y") == 0) {
            for (std::size_t i = 0; i < t_grid->xNum(); ++i) {
                Polygon poly;

                poly.append(0, start + i * step);
                def->gCellGridY.emplace_back(poly);
            }

            return 0;
        }
    }

    return 2;
};

int DEFEncoder::groupCallback(defrCallbackType_e t_type, defiGroup* t_shiftLayers, void* t_userData) {};

int DEFEncoder::integerCallback(defrCallbackType_e t_type, int t_number, void* t_userData) {};

int DEFEncoder::netCallback(defrCallbackType_e t_type, defiNet* t_net, void* t_userData) {};

int DEFEncoder::nonDefaultCallback(defrCallbackType_e t_type, defiNonDefault* t_rul, void* t_userData) {};

int DEFEncoder::pathCallback(defrCallbackType_e t_type, defiPath* t_path, void* t_userData) {};

int DEFEncoder::pinCallback(defrCallbackType_e t_type, defiPin* t_pinProp, void* t_userData) {};

int DEFEncoder::propCallback(defrCallbackType_e t_type, defiProp* t_prop, void* t_userData) {};

int DEFEncoder::regionCallback(defrCallbackType_e t_type, defiRegion* t_region, void* t_userData) {};

int DEFEncoder::rowCallback(defrCallbackType_e t_type, defiRow* t_row, void* t_userData) {};

int DEFEncoder::scanchainCallback(defrCallbackType_e t_type, defiScanchain* t_scanchain, void* t_userData) {};

int DEFEncoder::slotCallback(defrCallbackType_e t_type, defiSlot* t_slot, void* t_userData) {};

int DEFEncoder::stringCallback(defrCallbackType_e t_type, const char* t_string, void* t_userData) {};

int DEFEncoder::stylesCallback(defrCallbackType_e t_type, defiStyles* t_style, void* t_userData) {};

int DEFEncoder::trackCallback(defrCallbackType_e t_type, defiTrack* t_track, void* t_userData) {};

int DEFEncoder::viaCallback(defrCallbackType_e t_type, defiVia* t_via, void* t_userData) {};

int DEFEncoder::voidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};

int DEFEncoder::voidCallback(defrCallbackType_e t_type, void* t_variable, void* t_userData) {};