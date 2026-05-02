#pragma once

#define MR_PAIRING_SSP

#include "big.h"
#include "pairing_1.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

class ByteWriter
{
  public:
    explicit ByteWriter(std::vector<uint8_t>& out)
        : out_(out)
    {
    }

    static const int PREFIX_Big = 2;
    static const int PREFIX_GT = 4;

    // basic type
    template <typename T>
    std::enable_if_t<std::is_integral_v<T>> write(T v)
    {
        for (int i = sizeof(T) - 1; i >= 0; --i)
        {
            out_.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xff));
        }
    }

    // byte array
    void write(const uint8_t* data, size_t len);

    // NOTE: total_lenth = length + 2B
    // -------------------------
    // | length (2B) | payload |
    // -------------------------
    void write(const Big& b);

    // NOTE: total_length = length_x + length_y + 4B
    // -------------------------------------------
    // | length_x (2B) | length_y (2B) | payload |
    // -------------------------------------------
    void write(const GT& gt);

    size_t position() const;

  private:
    std::vector<uint8_t>& out_;
};
