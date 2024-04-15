#include "checksum.h"


namespace checksum
{

/// Checksums
uint64_t checksum64(const void *data, uint32_t size)
{
    const uint8_t *dat = (uint8_t *)data;
    uint64_t csum = 0;

    while (size--)
        csum += *dat++;

    return csum;
}

uint32_t checksum32(const void *data, uint32_t size)
{   return checksum64(data, size); }

uint16_t checksum16(const void *data, uint32_t size)
{   return checksum64(data, size); }

uint8_t checksum8(const void *data, uint32_t size)
{   return checksum64(data, size); }

/// Modsums
uint64_t modsum64(const void *data, uint32_t size)
{
	const uint8_t *dat = (uint8_t *)data;
	uint64_t sum = 0;
	size &= (uint32_t) ~7; // Paranoia

	for (uint32_t i = 0; i < size; i += 8)
		sum += ((uint64_t)dat[ i ] << 56 | (uint64_t)dat[1+i] << 48 | (uint64_t)dat[2+i] << 40 | (uint64_t)dat[3+i] << 32 | 
				(uint64_t)dat[4+i] << 24 | (uint64_t)dat[5+i] << 16 | (uint64_t)dat[6+i] <<  8 | (uint64_t)dat[7+i]);

	return sum;
}

uint32_t modsum32(const void *data, uint32_t size)
{
	const uint8_t *dat = (uint8_t *)data;
	uint32_t sum = 0;
	size &= (uint32_t) ~3; // Paranoia

	for (uint32_t i = 0; i < size; i += 4)
		sum += (dat[i] << 24 | dat[1+i] << 16 | dat[2+i] <<  8 | dat[3+i]);

	return sum;
}

uint32_t modsum32LE(const void *data, uint32_t size)
{
	const uint8_t *dat = (uint8_t *)data;
	uint32_t sum = 0;
	size &= (uint32_t) ~3; // Paranoia

	for (uint32_t i = 0; i < size; i += 4)
		sum += (dat[3+i] << 24 | dat[2+i] << 16 | dat[1+i] <<  8 | dat[0+i]);

	return sum;
}

uint16_t modsum16(const void *data, uint32_t size)
{
	const uint8_t *dat = (uint8_t *)data;
	uint16_t sum = 0;
	size &= (uint32_t) ~1; // Paranoia

	for (uint32_t i = 0; i < size; i += 2)
		sum += (dat[i] << 8 | dat[1+i]);

	return sum;
}

uint8_t modsum8(const void *data, uint32_t size)
{   return checksum64(data, size); } // modsum8 and checksum8 are one and the same...




}
