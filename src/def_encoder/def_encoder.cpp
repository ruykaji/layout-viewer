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

    return m_def;
}

int DEFEncoder::dieAreaCallback(defrCallbackType_e t_type, defiBox* t_box, void* t_userData) noexcept
{
    if (t_type == defrCallbackType_e::defrDieAreaCbkType) {
        auto points = t_box->getPoint();
        auto def = static_cast<Def*>(t_userData);

        if (points.numPoints == 6) {
            def->dieArea = Rect(points.x[0], points.y[0], std::abs(points.x[1] - points.x[0]), std::abs(points.y[1] - points.y[0]));

            return 0;
        }
    }

    return 2;
}