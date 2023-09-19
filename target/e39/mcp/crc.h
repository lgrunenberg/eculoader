#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>

uint16_t crcBuffer(
    const uint8_t  *data,
    const int       nBytes,
          uint16_t  currCRC,
    const uint16_t *tabCRC);

uint16_t crcBuffer_16(
    const uint8_t *data,
    const int      nBytes,
          uint16_t currCRC);

#endif
