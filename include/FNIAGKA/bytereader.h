#pragma once

#define MR_PAIRING_SSP

#include "pairing_1.h"

#include <big.h>
#include <cstddef>
#include <cstdint>
#include <zzn2.h>

template <typename T>
struct ByteReadTrait;

class ByteReader
{
  public:
    ByteReader(const uint8_t* data, size_t len)
        : data_(data),
          len_(len),
          pos_(0)
    {
    }

    bool eof() const;
    size_t remaining() const;
    size_t position() const;
    uint8_t readByte();
    const uint8_t* readBytes(size_t len);

    template <typename T>
    T read()
    {
        return ByteReadTrait<T>::read(*this);
    }

  private:
    const uint8_t* data_;
    size_t len_;
    size_t pos_;
};

// template sepcialization
template <typename T>
struct ByteReadTrait
{
    static_assert(std::is_integral_v<T>, "ByteReadTrait not specialized for this type");

    static T read(ByteReader& r)
    {
        if (r.remaining() < sizeof(T))
        {
            throw std::runtime_error("ByteReader: underflow");
        }

        using U = std::make_unsigned_t<T>;
        U v = 0;

        for (size_t i = 0; i < sizeof(T); ++i)
        {
            v = (v << 8) | r.readByte();
        }

        return static_cast<T>(v);
    }
};

template <>
struct ByteReadTrait<GT>
{
    static GT read(ByteReader& r)
    {
        auto lx = r.read<uint16_t>();
        auto ly = r.read<uint16_t>();

        const uint8_t* px = r.readBytes(lx);
        const uint8_t* py = r.readBytes(ly);

        Big bx = from_binary(lx, const_cast<char*>(reinterpret_cast<const char*>(px)));
        Big by = from_binary(ly, const_cast<char*>(reinterpret_cast<const char*>(py)));
        ZZn x(bx);
        ZZn y(by);
        ZZn2 z(x, y);

        GT gt;
        gt.g = z;
        return gt;
    }
};

template <>
struct ByteReadTrait<Big>
{
    static Big read(ByteReader& r)
    {
        auto len = r.read<uint16_t>();
        const uint8_t* p = r.readBytes(len);
        Big b = from_binary(len, const_cast<char*>(reinterpret_cast<const char*>(p)));
        return b;
    }
};
