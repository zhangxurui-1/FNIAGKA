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
ByteWriter::write(const uint8_t* data, size_t len)
{
    out_.insert(out_.end(), data, data + len);
}

size_t
ByteWriter::position() const
{
    return out_.size();
}
