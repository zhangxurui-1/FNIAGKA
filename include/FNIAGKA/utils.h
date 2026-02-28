#pragma once

#define MR_PAIRING_SSP
#include "pairing_1.h"

#include <sstream>
#include <string>

inline std::string
ToString(const G1& elem)
{
    ZZn coordinate_a, coordinate_b, coordinate_c;
    extract(const_cast<ECn&>(elem.g), coordinate_a, coordinate_b, coordinate_c);

    std::stringstream ss;
    ss << "(" << coordinate_a << ", " << coordinate_b << ", " << coordinate_c << ")";
    return ss.str();
}

inline std::string
ToString(const GT& elem)
{
    ZZn x, y;
    elem.g.get(x, y);

    std::stringstream ss;
    ss << "(" << x << "," << y << ")";
    return ss.str();
}
