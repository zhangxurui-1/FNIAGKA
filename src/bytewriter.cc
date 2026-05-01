#include "FNIAGKA/bytewriter.h"

#include <cstddef>
#include <cstdint>

void
ByteWriter::write(const Big& b)
{
    char tmp[1024];

    uint16_t l = to_binary(b, 1024, tmp);
    write(l);
    write(reinterpret_cast<uint8_t*>(tmp), l);
}

void
ByteWriter::write(const GT& gt)
{
    ZZn x, y;
    gt.g.get(x, y);

    Big bx(x);
    Big by(y);

    char tmp_x[1024];
    char tmp_y[1024];

    uint16_t lx = to_binary(bx, 1024, tmp_x);
    write(lx);
    uint16_t ly = to_binary(by, 1024, tmp_y);
    write(ly);

    write(reinterpret_cast<uint8_t*>(tmp_x), lx);
    write(reinterpret_cast<uint8_t*>(tmp_y), ly);
}

void
ByteWriter::write(const uint8_t* data, size_t len)
{
    out_.insert(out_.end(), data, data + len);
}

size_t
ByteWriter::position() const
{
    return out_.size();
}
