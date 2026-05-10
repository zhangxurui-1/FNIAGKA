#include "FNIAGKA/bytewriter.h"

#include <cstddef>
#include <cstdint>

namespace
{
void
WriteZZn2(ByteWriter& writer, const ZZn2& value)
{
    Big x;
    Big y;
    value.get(x, y);
    writer.write(x);
    writer.write(y);
}

void
WriteZZn4(ByteWriter& writer, const ZZn4& value)
{
    ZZn2 x;
    ZZn2 y;
    value.get(x, y);
    WriteZZn2(writer, x);
    WriteZZn2(writer, y);
}

void
WriteZZn8(ByteWriter& writer, const ZZn8& value)
{
    ZZn4 x;
    ZZn4 y;
    value.get(x, y);
    WriteZZn4(writer, x);
    WriteZZn4(writer, y);
}
} // namespace

void
ByteWriter::write(const Big& b)
{
    char tmp[1024];

    uint16_t l = to_binary(b, 1024, tmp);
    write(l);
    write(reinterpret_cast<uint8_t*>(tmp), l);
}

void
ByteWriter::write(const uint8_t* data, size_t len)
{
    out_.insert(out_.end(), data, data + len);
}

void
ByteWriter::write(const G1& g1)
{
    Big x;
    Big y;
    g1.g.get(x, y);
    write(x);
    write(y);
}

void
ByteWriter::write(const G2& g2)
{
    ZZn4 x;
    ZZn4 y;
    g2.g.get(x, y);
    WriteZZn4(*this, x);
    WriteZZn4(*this, y);
}

void
ByteWriter::write(const GT& gt)
{
    ZZn8 x;
    ZZn8 y;
    ZZn8 z;
    gt.g.get(x, y, z);
    WriteZZn8(*this, x);
    WriteZZn8(*this, y);
    WriteZZn8(*this, z);
}

size_t
ByteWriter::position() const
{
    return out_.size();
}
