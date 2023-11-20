#include "data.hpp"

Data::~Data()
{
    correspondingToPinCell.clear();

    for (auto& row : cells) {
        for (auto& col : row) {
            col->pins.clear();
        }

        row.clear();
    }
}