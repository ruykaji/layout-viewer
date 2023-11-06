#include "data.hpp"

bool Net::operator<(const Net& t_net) const
{
    return index < t_net.index;
}

Data::~Data()
{
    correspondingToPinCells.clear();

    for (auto& row : cells) {
        for (auto& col : row) {
            col->pins.clear();
        }

        row.clear();
    }
}