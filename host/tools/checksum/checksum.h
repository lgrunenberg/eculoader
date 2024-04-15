#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <cstdio>
#include <cstdint>

namespace checksum
{
    uint64_t checksum64(const void *data, uint32_t size);
    uint32_t checksum32(const void *data, uint32_t size);
    uint16_t checksum16(const void *data, uint32_t size);
    uint8_t  checksum8(const void *data, uint32_t size);

    uint64_t modsum64(const void *data, uint32_t size);
    uint32_t modsum32(const void *data, uint32_t size);
    uint32_t modsum32LE(const void *data, uint32_t size);
    uint16_t modsum16(const void *data, uint32_t size);
    uint8_t  modsum8(const void *data, uint32_t size);
}

#endif
