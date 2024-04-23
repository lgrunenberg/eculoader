#include "gmlan.h"

#include "../../tools/tools.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <time.h>

using namespace timer;
using namespace std;
using namespace msgsys;
using namespace logger;
using namespace std::chrono;

void gmlan::setTesterID(uint64_t ID)
{
    this->testerID = ID;
}

void gmlan::setTargetID(uint64_t ID)
{
    this->targetID = ID;
}

uint64_t gmlan::getTesterID()
{
    return this->testerID;
}

uint64_t gmlan::getTargetID()
{
    return this->targetID;
}

#warning "This is only a half-assed implementation."

void gmlan::sendAck()
{
    message_t sMsg = newMessage(this->testerID, 8);

    sMsg.message[0] = 0x30;
    sMsg.message[1] = 0x00;
    sMsg.message[2] = 0x00;
    send(&sMsg);
}

/* USDT PCI encoding
Byte          __________0__________ ____1____ ____2____
Bits         | 7  6  5  4    3:0   |   7:0   |   7:0   |
Single       | 0  0  0  0     DL   |   N/A   |   N/A   |   (XX, .. ..)
First consec | 0  0  0  1   DL hi  |  DL lo  |   N/A   |   (1X, XX ..)
Consecutive  | 0  0  1  0     SN   |   N/A   |   N/A   |   (21, 22, 23 .. 29, 20, 21 ..)
Flow Control | 0  0  1  1     FS   |   BS    |  STmin  |   (3X, XX, XX)
Extrapolated:

Exd single   | 0  1  0  0   addr?  | ????:dl |   N/A   |   101 [41 92    (1a 79)   00000000]

*/

uint8_t *gmlan::sendRequest(uint8_t *req, uint16_t len)
{
    message_t  sMsg = newMessage(this->testerID, 8);
    message_t *rMsg;
    uint8_t   *recdat = 0;
    uint8_t    msgReq = req[0];

    len &= 0x0fff;

    // Single frame
    if (len < 8)
    {
        sMsg.message[0] = len;

        for (uint32_t i = 0; i < len; i++)
            sMsg.message[1+i] = *req++;
        
        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return nullptr;
        }
    }
    else
    {
        sMsg.message[0] = 0x10 | (len >> 8);
        sMsg.message[1] = len;

        for (uint32_t i = 2; i < 8; i++)
            sMsg.message[i] = *req++;

        len -= 6;

        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return nullptr;
        }

        rMsg = waitMessage(500);

        if ((rMsg->message[0] & 0x30) != 0x30)
        {
            log(gmlanlog, "Got no ack!");
            return nullptr;
        }
        uint8_t stp = 0x21;

        // Consecutive sent frames before forced to wait for a new flow control
        uint32_t BS = rMsg->message[1];
        // Delay between consecutive frames
        uint32_t ST = rMsg->message[2];
        if (ST < 0x80)
            ST *= 1000; // Milliseconds
        else if (ST > 0xf0 && ST < 0xfa)
            ST = (ST&15) * 100; // microsec * 100
        else
        {
            log(gmlanlog, "Received funny flow control frame");
            ST = 0;
        }

        if (BS > 0)
        {
            log(gmlanlog, "BS is set. Implement!");
        }

        while (len > 0)
        {
            uint32_t thisCount = (len > 7) ? 7 : len;
            len -= thisCount;

            for (uint32_t i = 0; i < thisCount; i++)
                sMsg.message[1 + i] = *req++;

            sMsg.message[0] = stp++;
            stp&= 0x2f;

            if (len == 0)
                setupWaitMessage(this->targetID);

            if(!send(&sMsg))
            {
                log(gmlanlog, "Could not send!");
                return nullptr;
            }

            sleepMicro( ST );
        }
    }

    // auto oldTime = system_clock::now();

busyWait:

    rMsg = waitMessage(500);
    if (rMsg->id != this->targetID)
    {
        // log(gmlanlog, "No response");
        // This maaaay not be a showstopper. if the previous response was a 7f 78.
        return recdat;
    }

    // Single message
    if (rMsg->message[0] > 0 && rMsg->message[0] < 8)
    {
        // This will _NOT_ work correctly for kwp-esque protocols since they're event-driven!!!!
        // (ie they will not broadcast this until they're done on their own, you have to poll them)
        // Busy, wait for a new update
        if (rMsg->message[0] >  0x02 && rMsg->message[0] < 8 &&
            rMsg->message[1] == 0x7f && 
            rMsg->message[2] == msgReq &&
            rMsg->message[3] == 0x78)
        {
#warning "CIM takes a VERY long time"
            // if (duration_cast<milliseconds>(system_clock::now() - oldTime).count() < busyWaitTimeout)
            {
                goto busyWait;
            }
        }
        
        if (recdat)
            delete[] recdat;

        recdat = new uint8_t[ rMsg->message[0] + 2 ];
        recdat[0] = 0;

        for (uint32_t i = 0; i < ((uint32_t)rMsg->message[0] + 1); i++)
            recdat[i + 1] =  rMsg->message[i];

        return recdat;
    }

    if ((rMsg->message[0] & 0xf0) == 0x10)
    {
        // bytes, total to receive including the header
        uint16_t toRec = ((rMsg->message[0] << 8 | rMsg->message[1]) & 0xfff) + 2;
        if (toRec <= 8)
        {
            log(gmlanlog, "Target is drunk. Received undersized extended frame");
            return recdat;
        }

        if (recdat)
            delete[] recdat;

        recdat = new uint8_t[ toRec ];
        uint32_t stp = 0x21;
        uint32_t bufPtr = 0;

        for (uint32_t i = 0; i < 8; i++)
            recdat[bufPtr++] =  rMsg->message[i];
        // Strip high nible
        recdat[0] &= 0x0f;

        // Remove size of header (2) and number of bytes stored (6)
        toRec -= 8;

        setupWaitMessage(this->targetID);
        sendAck();

        while (toRec > 0)
        {
            rMsg = waitMessage(500);

            uint16_t thisCount = (toRec > 7) ? 7 : toRec;
            toRec -= thisCount;

            if (rMsg->message[0] != stp) {
                delete[] recdat;
                return nullptr;
            }

            stp = (stp+1)&0x2f;

            for (uint32_t i = 0; i < thisCount; i++)
                recdat[bufPtr++] =  rMsg->message[i+1];
        }

        return recdat;
    }
    else
    {
        log(gmlanlog, "Unknown PCI");
    }

    if (recdat)
        delete[] recdat;

    return nullptr;
}

// 04
bool gmlan::clearDiagnosticInformation()
{
    uint8_t *ret, buf[8] = { 0x04 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x44)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 10
// sub 02 == disableAllDTCs
// sub 03 == enableDTCsDuringDevCntrl
// sub 04 == wakeUpLinks
bool gmlan::InitiateDiagnosticOperation(uint8_t mode)
{
    uint8_t *ret, buf[8] = { 0x10, mode };

    if ((ret = sendRequest(buf, 2)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x50)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 1a
uint8_t *gmlan::ReadDataByIdentifier(uint8_t id)
{
    uint8_t *ret, buf[3] = { 0x1a, id };

    if ((ret = sendRequest(buf, 2)) == nullptr)
        return nullptr;

    uint16_t retLen = (ret[0] << 8 | ret[1]) + 2;

#warning "Clean me!"
    if (retLen < 5 ||  ret[2] != 0x5a || ret[3] != id)
    {
        delete[] ret;
        return nullptr;
    }

    retLen-=4;
    ret[0] = (retLen>>8);
    ret[1] = (retLen);
    retLen += 2;

    for (uint32_t i = 2; i < retLen; i++)
        ret[i] = ret[i + 2];

    ret[retLen] = 0;
    ret[retLen+1] = 0;

    return ret;
}

// 20
bool gmlan::returnToNormal()
{
    uint8_t *ret, buf[8] = { 0x20 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x60)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 23
// Read from a 24-bit address using a 16-bit size argument
uint8_t *gmlan::readMemoryByAddress_24_16(uint32_t address, uint32_t len, uint32_t blockSize)
{
    if (len == 0)
        return nullptr;

    uint8_t  *ret, buf[8] = { 0x23 };
    uint8_t *dataBuf = new uint8_t[len];
    uint32_t dataLeft = len;
    uint32_t dataPtr = 0;

    while (dataLeft > 0)
    {
        uint32_t thisLen = (dataLeft > blockSize) ? blockSize : dataLeft;
        buf[1] = (address >> 16);
        buf[2] = (address >> 8);
        buf[3] = address;
        buf[4] = (thisLen >> 8);
        buf[5] = thisLen;

        if ((ret = sendRequest(buf, 6)) == nullptr)
        {
            log(gmlanlog, "No retbuffer for read by address");
            delete[] dataBuf;
            return nullptr;
        }

        if ((uint32_t)(ret[0] << 8 | ret[1]) != (thisLen + 4) || ret[2] != 0x63)
        {
            log(gmlanlog, "Read unexpected response");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }

        // xx xx 63 HH MM LL .. .. ..
        for (uint32_t i = 0; i < thisLen; i++)
            dataBuf[dataPtr + i] = ret[(6) + i];

        delete[] ret;
        dataLeft-=thisLen;
        address+=thisLen;
        dataPtr+=thisLen;
    }

    return dataBuf;
}

// 23
// Read from a 32-bit address using a 16-bit size argument
uint8_t *gmlan::readMemoryByAddress_32_16(uint32_t address, uint32_t len, uint32_t blockSize)
{
    if (len == 0)
        return nullptr;

    uint8_t  *ret, buf[8] = { 0x23 };
    uint8_t *dataBuf = new uint8_t[len];
    uint32_t dataLeft = len;
    uint32_t dataPtr = 0;

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

        if ((uint32_t)(ret[0] << 8 | ret[1]) != (thisLen + 5) || ret[2] != 0x63)
        {
            log(gmlanlog, "Read unexpected response");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }

        // xx xx 63 HH MM LL .. .. ..
        for (uint32_t i = 0; i < thisLen; i++)
            dataBuf[dataPtr + i] = ret[(7) + i];

        delete[] ret;
        dataLeft-=thisLen;
        address+=thisLen;
        dataPtr+=thisLen;
    }

    return dataBuf;
}

#warning "Correct response"

// 23
// Read from a 32-bit address using a 24-bit size argument
uint8_t *gmlan::readMemoryByAddress_32_24(uint32_t address, uint32_t len, uint32_t blockSize)
{
    if (len == 0)
        return nullptr;

    uint8_t  *ret, buf[10] = { 0x23 };
    uint8_t *dataBuf = new uint8_t[len];
    uint32_t dataLeft = len;
    uint32_t dataPtr = 0;

    while (dataLeft > 0)
    {
        uint32_t thisLen = (dataLeft > blockSize) ? blockSize : dataLeft;
        buf[1] = (address >> 24);
        buf[2] = (address >> 16);
        buf[3] = (address >> 8);
        buf[4] = address;

        buf[5] = (thisLen >> 16);
        buf[6] = (thisLen >> 8);
        buf[7] = thisLen;

        if ((ret = sendRequest(buf, 8)) == nullptr)
        {
            log(gmlanlog, "No retbuffer for read by address");
            delete[] dataBuf;
            return nullptr;
        }

        if ((uint32_t)(ret[0] << 8 | ret[1]) != (thisLen + 4) || ret[2] != 0x63)
        {
            log(gmlanlog, "Read unexpected response");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }

        // xx xx 63 HH MM LL .. .. ..
        for (uint32_t i = 0; i < thisLen; i++)
            dataBuf[dataPtr + i] = ret[(6) + i];

        delete[] ret;
        dataLeft-=thisLen;
        address+=thisLen;
        dataPtr+=thisLen;
    }

    return dataBuf;
}

// Weird responses will not be interpreted as they should
#warning "This is not compliant"

// 28
bool gmlan::disableNormalCommunication(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;
        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x28;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x28;
    }

    setupWaitMessage(this->targetID);

    if(!send(&sMsg))
    {
        log(gmlanlog, "Could not send!");
        return false;
    }

    message_t *rMsg = waitMessage(500);
    return (
        rMsg->message[0] < 8 &&
        rMsg->message[0] > 0 &&
        rMsg->message[1] == 0x68) ? true : false;
}

// 28
bool gmlan::disableNormalCommunicationNoResp(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;
        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x28;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x28;
    }

    if(!send(&sMsg))
    {
        log(gmlanlog, "Could not send!");
        return false;
    }

    return true;
}

// 34
bool gmlan::requestDownload_16(uint32_t size, uint8_t fmt)
{
    uint8_t *ret, buf[8] = { 0x34, fmt };
    buf[3] = size >> 8;
    buf[4] = size;

    if ((ret = sendRequest(buf, 4)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x74)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 34
bool gmlan::requestDownload_24(uint32_t size, uint8_t fmt)
{
    uint8_t *ret, buf[8] = { 0x34, fmt };
    buf[2] = size >> 16;
    buf[3] = size >> 8;
    buf[4] = size;

    if ((ret = sendRequest(buf, 5)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x74)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 34
bool gmlan::requestDownload_32(uint32_t size, uint8_t fmt)
{
    uint8_t *ret, buf[8] = { 0x34, fmt };
    buf[2] = size >> 24;
    buf[3] = size >> 16;
    buf[4] = size >> 8;
    buf[5] = size;

    if ((ret = sendRequest(buf, 6)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x74)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 36
bool gmlan::transferData_32(const uint8_t *data, uint32_t address, uint32_t len, uint32_t blockSize, bool execute)
{
    uint8_t *ret, buf[256 + 6];
    uint32_t bufPtr = 0;
    uint32_t addrPtr = address;
    uint32_t remain = len;

#warning "needs work"

    buf[0] = 0x36;
    buf[1] = 0;

    progress( 0 );

    // Prevent overflow of message buffer
    if (blockSize > 256)
        blockSize = 256;
    if (blockSize == 0)
        return false;

    // We want it to send the request
    while (remain > 0)
    {
        size_t thisLen = (remain > blockSize) ? blockSize : remain;
        buf[2] = addrPtr >> 24;    // 2
        buf[3] = addrPtr >> 16;    // 3
        buf[4] = addrPtr >> 8;     // 4
        buf[5] = addrPtr;          // 5

        memcpy(&buf[6], &data[bufPtr], thisLen);

        if ((ret = sendRequest(buf, thisLen + 6)) == nullptr)
        {
            log(gmlanlog, "Transfer failed (no answer)");
            return false;
        }

        // 00 xx 76
        if ((uint32_t)(ret[0] << 8 | ret[1])  < 1 || ret[2] != 0x76)
        {
            log(gmlanlog, "Received fail");
            delete[] ret;
            return false;
        }
        delete[] ret;

        bufPtr += thisLen;
        remain -= thisLen;
        addrPtr += thisLen;

        progress( (uint32_t)(float) ( 100.0 * (len-remain)) / len );
    }

    if (execute == true)
    {
        buf[1] = 0x80;
        buf[2] = address >> 24;    // 2
        buf[3] = address >> 16;    // 3
        buf[4] = address >> 8;     // 4
        buf[5] = address;          // 5

        // Special case. It's expected NOT to respond when requesting start of code
        if ((ret = sendRequest(buf, 6)) == nullptr)
        {
            return true;
        }

        // 00 xx 76
        if ((uint32_t)(ret[0] << 8 | ret[1])  < 1 || ret[2] != 0x76)
        {
            log(gmlanlog, "Received fail");
            delete[] ret;
            return false;
        }

        delete[] ret;
    }

    return true;
}

// 3b
bool gmlan::WriteDataByIdentifier(const uint8_t *dat, uint8_t id, uint32_t len)
{
#warning "Clean me!"
    uint8_t *ret, buf[256 + 2] = { 0x3b, id };

    if (len > 256)
        return false;

    if (dat)
        memcpy(&buf[2], dat, len);
    else
        memset(&buf[2], 0  , len);

    if ((ret = sendRequest(buf, len + 2)) == nullptr)
        return false;

    // 20202020202020202020
    // 00 02 7b __ xx xx
    if ((uint32_t)(ret[0] << 8 | ret[1]) < 2 || ret[2] != 0x7b || ret[3] != id)
    {
        delete[] ret;
        return false;
    }

    log(gmlanlog, "Succ??");
    delete[] ret;
    return true;
}


#warning "Mend"

// 3e
bool gmlan::testerPresent(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;

        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x3e;

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return false;
        }

        return true;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x3e;

        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return false;
        }
    }

    message_t *rMsg = waitMessage(500);
    return (rMsg->message[0] < 8 && rMsg->message[1] == 0x7e) ? true : false;
}

/* a2
00: fully programmed
01: no op s/w or cal data
02: op s/w present, cal data missing
50: General Memory Fault
51: RAM Memory Fault
52: NVRAM Memory Fault
53: Boot Memory Failure
54: Flash Memory Failure
55: EEPROM Memory Failure */
bool gmlan::ReportProgrammedState()
{
    uint8_t *ret, buf[8] = { 0xa2 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
    {
        log(gmlanlog, "Could not retrieve programmed state");
        return false;
    }

    // 00 02 e2 <st>
    if ((uint32_t)(ret[0] << 8 | ret[1])  < 2 || ret[2] != 0xe2)
    {
        log(gmlanlog, "Could not retrieve programmed state");
        delete[] ret;
        return false;
    }

    string state;
    switch (ret[3]) {
    case 0x00: state = "Fully programmed"; break;
    case 0x01: state = "No op s/w or cal data"; break;
    case 0x02: state = "Op s/w present, cal data missing"; break;
    case 0x50: state = "General Memory Fault"; break;
    case 0x51: state = "RAM Memory Fault"; break;
    case 0x52: state = "NVRAM Memory Fault"; break;
    case 0x53: state = "Boot Memory Failure"; break;
    case 0x54: state = "Flash Memory Failure"; break;
    case 0x55: state = "EEPROM Memory Failure"; break;
    default:
        state = "Unknown response: 0x" + to_hex((volatile uint32_t)ret[3], 1);
        break;
    }

    log(gmlanlog, "Target state: " + state);

    delete[] ret;
    return true;
}

// a5
// sub: 1 == requestProgrammingMode
// sub: 2 == requestProgrammingMode_HighSpeed
// sub: 3 == enableProgrammingMode
bool gmlan::ProgrammingMode(uint8_t lev)
{
    uint8_t *ret, buf[8] = { 0xa5, lev };

    if ((ret = sendRequest(buf, 2)) == nullptr)
        return false;

    // 00 01 e5 ..?
    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0xE5)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// ae
bool gmlan::DeviceControl(const uint8_t *dat, uint8_t id, uint8_t len)
{
    uint8_t *ret, buf[256 + 2] = { 0xae, id };

    if (len > 256)
        return false;

    if (dat)
        memcpy(&buf[2], dat, len);
    else
        memset(&buf[2], 0  , len);
    
    if ((ret = sendRequest(buf, len + 2)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0xee)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}
