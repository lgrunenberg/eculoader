#include <cstdio>
#include <cstdint>
#include <cstring>

#include "gmloader.h"

#include "../../tools/tools.h"

using namespace logger;
using namespace timer;

bool gmloader::loader_StartRoutineById(uint8_t service, uint32_t param1, uint32_t param2)
{
    uint32_t tries = 5;
    uint8_t *ret, buf[10] = {0x31};

    buf[1] = (uint8_t)service;
    buf[2] = (uint8_t)(param1 >> 24);
    buf[3] = (uint8_t)(param1 >> 16);
    buf[4] = (uint8_t)(param1 >> 8);
    buf[5] = (uint8_t)(param1);

    buf[6] = (uint8_t)(param2 >> 24);
    buf[7] = (uint8_t)(param2 >> 16);
    buf[8] = (uint8_t)(param2 >> 8);
    buf[9] = (uint8_t)(param2);

    while (--tries > 0)
    {
        if ((ret = sendRequest(buf, 10)) == nullptr)
            continue;

        if ((uint32_t)(ret[0] << 8 | ret[1]) != 2 || ret[2] != 0x71 || ret[3] != service)
        {
            delete[] ret;
            continue;
        }
        else
        {
            delete[] ret;
            return true;
        }
    }

    return false;
}

// 23
uint8_t *gmloader::loader_readMemoryByAddress(uint32_t address, uint32_t len, uint32_t blockSize)
{
    if (len == 0)
        return nullptr;

    uint8_t *ret, buf[9] = {0x23};
    uint8_t *dataBuf = new uint8_t[len];
    uint32_t dataLeft = len;
    uint32_t dataPtr = 0;

    progress( 0 );

    while (dataLeft > 0)
    {
        uint32_t thisLen = (dataLeft > blockSize) ? blockSize : dataLeft;
        buf[1] = (address >> 24);
        buf[2] = (address >> 16);
        buf[3] = (address >> 8);
        buf[4] = address;
        buf[5] = (thisLen >> 8);
        buf[6] = thisLen;

        if ((ret = sendRequest(buf, 7)) == nullptr)
        {
            log(gmlanlog, "No retbuffer for read by address");
            delete[] dataBuf;
            return nullptr;
        }

        uint32_t retLen = (ret[0] << 8 | ret[1]);

        // Catch ff-skip
        if (retLen == 5 &&
            ret[2] == 0x63 &&
            ret[3] == 0xff &&
            ret[4] == 0xff &&
            ret[5] == 0xff &&
            ret[6] == 0xff)
        {
            memset(&dataBuf[dataPtr], 0xff, thisLen);
        }
        else
        {

            // xx xx 63 xx xx xx xx .. CS CS
            if (retLen != (thisLen + 7) || ret[2] != 0x63)
            {
                log(gmlanlog, "Read unexpected response");
                delete[] ret;
                delete[] dataBuf;
                return nullptr;
            }

            uint32_t checksum = 0;

            for (uint32_t i = 0; i < thisLen; i++)
            {
                checksum += ret[(7) + i];
                dataBuf[dataPtr + i] = ret[(7) + i];
            }

            if ((uint32_t)(ret[thisLen + 7] << 8 | ret[thisLen + 8]) != (checksum & 0xffff))
            {
                log(gmlanlog, "Checksum mismatch!");
                delete[] ret;
                delete[] dataBuf;
                return nullptr;
            }
        }

        delete[] ret;
        dataLeft -= thisLen;
        address += thisLen;
        dataPtr += thisLen;

        progress( (uint32_t)(float) ( 100.0 * (len-dataLeft)) / len );
    }

    return dataBuf;
}

uint8_t *gmloader::loader_requestRoutineResult(uint8_t service)
{
    uint32_t tries = 5;
    uint8_t *ret, buf[2] = {0x33, service};

    while ( --tries )
    {
        if ((ret = sendRequest(buf, 2)) != nullptr)
        {
            uint32_t retLen = (uint32_t)(ret[0] << 8 | ret[1]);

            if (retLen < 2)
            {
                // .. do nothing and let it delete data
            }
            // Busy, come again. This is actually a kwp2k command but it suits our needs
            else if (retLen > 2 && ret[2] == 0x7f && ret[3] == 0x33 && ret[4] == 0x21)
            {
                tries = 5;
            }
            else if (ret[2] == 0x73 && ret[3] == service)
            {
                retLen -= 2;
                ret[0] = retLen >> 8;
                ret[1] = retLen;

                for (uint32_t i = 0; i < retLen; i++)
                {
                    ret[2 + i] = ret[4 + i];
                }

                return ret;
            }

            delete[] ret;
        }

        sleepMS(25);
    }

    return nullptr;
}
