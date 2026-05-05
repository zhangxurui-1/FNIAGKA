#pragma once

// Keep MIRACL pairing configuration consistent across the project.
#define MR_PAIRING_BN
#include "pairing_3.h"

#include <sstream>
#include <string>

inline std::string
ToString(const G1& elem)
{
    std::stringstream ss;
    ss << elem.g;
    return ss.str();
}

inline std::string
ToString(const G2& elem)
{
    std::stringstream ss;
    ss << elem.g;
    return ss.str();
}

inline std::string
ToString(const GT& elem)
{
    std::stringstream ss;
    ss << elem.g;
    return ss.str();
}
