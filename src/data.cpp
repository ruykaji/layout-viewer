#include "data.hpp"

bool Net::operator<(const Net& t_net) const
{
    return index < t_net.index;
}