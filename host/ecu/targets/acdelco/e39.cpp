
#include "acdelco.h"

#include "../../../adapter/message.h"

#include "../../../tools/tools.h"

using namespace logger;
using namespace timer;
using namespace std;
using namespace std::chrono;
using namespace msgsys;

enum mpc5566Mode : uint32_t
{
    modeBAM   = 0, // Defunct. It must be compiled in this mode
    modeE39   = 1,
    modeE78   = 2,
    modeE39A  = 3,
};

e39::e39()
{
}

e39::~e39()
{
}

void e39::configProtocol()
{
    setTesterID( 0x7e0 );
    setTargetID( 0x7e8 );
}


uint64_t lazySWAP(uint64_t data)
{
    uint64_t retval = 0;
    for (int i = 0; i < 8; i++)
    {
        retval <<= 8;
        retval |= (data & 0xff);
        data >>= 8;
    }
    return retval;
}

// To be moved somewhere else. It's only here while in the thrash branch
// uint64_t bamKEY = 0x7BC10CBD55EBB2DA; // E78
// uint64_t bamKEY = 0xFEEDFACECAFEBEEF; // Public
uint64_t bamKEY = 0xFD13031FFB1D0521; // 881155AA
//                   0x________________; // Since the editor is ***...

static uint64_t rU64Mgs(message_t *msg)
{
    uint64_t retval = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        retval <<= 8;
        retval |= msg->message[7 - i];
    }
    return retval;
}

static void wU64Mgs(message_t *msg, uint64_t dat)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        msg->message[i] = (uint8_t)dat;
        dat >>= 8;
    }
}

bool e39::bamFlash(uint32_t address)
{
    message_t sMsg = newMessage(0x11, 8);

    uint32_t tries = 10;

    wU64Mgs(&sMsg, lazySWAP(bamKEY));
    uint64_t retVal = ~lazySWAP(bamKEY);

    setupWaitMessage(1);
    do
    {
        if (!send(&sMsg))
        {
            log("Could not send!");
            return false;
        }

        message_t *rMsg = waitMessage(250);
        retVal = rU64Mgs(rMsg);

        // tries--;
        if (tries == 0)
        {
            log("Could not start bam!");
            return false;
        }
    } while (retVal != lazySWAP(bamKEY));

    log("Key accepted");

    fileManager fm;
    fileHandle *file = fm.open("./loaderblobs/mpc5566_loader_bam.bin");

    if (!file || file->size < (6 * 1024))
    {
        log("No file or too small");
        return false;
    }

    uint32_t alignedSize = (file->size + 7) & ~7;
    uint8_t *tmpBuf = new uint8_t[alignedSize];
    if (tmpBuf == nullptr)
    {
        log("Could not allocate buffer");
        return false;
    }

    memcpy(tmpBuf, file->data, file->size);

    uint64_t cmd = lazySWAP((uint64_t)address << 32 | alignedSize);
    sMsg = newMessage(0x12, 8);

    wU64Mgs(&sMsg, cmd);
    setupWaitMessage(2);
    if (!send(&sMsg))
    {
        log("Could not send!");
        return false;
    }

    // 0xFFFFF00

    message_t *rMsg = waitMessage(50);
    retVal = rU64Mgs(rMsg);

    uint32_t bufPntr = 0;
    if (retVal == cmd)
    {
        log("Address and size accepted");

        sMsg = newMessage(0x13, 8);

        while (alignedSize > 0)
        {
            cmd = 0;
            for (int e = 0; e < 8; e++)
            {
                cmd |= (uint64_t)tmpBuf[bufPntr++] << (e * 8);
            }

            // cmd = lazySWAP(cmd);
            wU64Mgs(&sMsg, cmd);

            setupWaitMessage(3);
            sleepMS(4);
            if (!send(&sMsg))
            {
                log("Could not send!");
                return false;
            }

            // return false;
            // response = m_canListener.waitMessage(200);
            // respData = response.getData();

            rMsg = waitMessage(500);
            retVal = rU64Mgs(rMsg);

            if (retVal != cmd)
            {
                log("Did not receive the same data");
                // CastInfoEvent("Did not receive the same data: " + respData.ToString("X16"), ActivityType.ConvertingFile);
                return false;
            }

            alignedSize -= 8;
        }

        return true;
    }

    return false;
}



static void calculateKeyForE39(uint8_t *seed)
{
    uint32_t _seed = seed[0] << 8 | seed[1];
    _seed = (_seed + 0x6C50) & 0xFFFF;
    _seed = ((_seed << 8 | _seed >> 8) - 0x22DA) & 0xffff;
    _seed = ((_seed << 9 | _seed >> 7) - 0x8BAC);
    seed[0] = _seed >> 8;
    seed[1] = _seed;
}

bool e39::secAccE39(uint8_t lev)
{
    uint8_t buf[8] = {0x27, lev};

    uint8_t *ret = sendRequest(buf, 2);
    if (ret == 0)
        return false;

    uint16_t retLen = (ret[0] << 8 | ret[1]) + 2;

    // 00 04 67 __ xx xx
    if (retLen < 6 || ret[2] != 0x67 || ret[3] != lev)
    {
        delete[] ret;
        return false;
    }

    if (ret[4] == 0 && ret[5] == 0)
    {
        log("Already granted");
        delete[] ret;
        return true;
    }

    calculateKeyForE39(&ret[4]);

    buf[0] = 0x27;
    buf[1] = lev + 1;
    buf[2] = ret[4];
    buf[3] = ret[5];
    delete[] ret;

    ret = sendRequest(buf, 4);
    if (ret == 0)
        return false;
    retLen = (ret[0] << 8 | ret[1]) + 2;

    // 00 04 67 __ xx xx
    if (retLen < 4 || ret[2] != 0x67 || ret[3] != (lev + 1))
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}


bool e39::dump()
{
    log("Dump");


    // bamFlash(0x40004000);
    // return false;

    configProtocol();


    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

    log("Checking loader state..");

    string ident = "";
    uint8_t *tmp;
    if ( (tmp = ReadDataByIdentifier(0x90)) != nullptr )
    {
        uint32_t retLen = (tmp[0] << 8 | tmp[1]);
        for (uint32_t i = 0; i < retLen; i++)
            ident += tmp[i + 2];
        delete[] tmp;
    }

    if ( ident != "MPC5566-LOADER: TXSUITE.ORG" )
    {
        log("Loader not active. Starting upload..");
        InitiateDiagnosticOperation(3); // 10
        sleepMS(10);

        disableNormalCommunication(); // 28
        sleepMS(10);

        testerPresent();

        if (!secAccE39(1))
        {
            log("Could not gain security access");
            return false;
        }

        testerPresent();
        if (!ProgrammingMode(1))
            return false;

        // Won't give a response
        ProgrammingMode(3);

        testerPresent();

/*
        uint8_t toWrite[512];
        for (uint32_t i = 0; i < 0x100; i++)
        {
            uint8_t *ret = ReadDataByIdentifier(i);

            if (ret)
            {
                string gotString = "";
                string charString = "";
                uint16_t retLen = (ret[0] << 8 | ret[1]);
                for (uint32_t sr = 0; sr < retLen; sr++)
                {
                    gotString += to_hex((volatile uint32_t)ret[sr + 2], 1);
                    if (ret[sr + 2] >= 0x20 && ret[sr + 2] < 0x7f)
                    {
                        charString += ret[sr + 2];
                    }
                    ret[sr + 2] = (uint8_t)rand();
                }
                log("id " + to_hex(i,1) + ": " + gotString + "   (" + charString + ")");

                //
                // WriteDataByIdentifier(i, &ret[2], retLen);
                testerPresent();
            }
        }

        return false;
*/

        fileManager fm;
        fileHandle *file = fm.open("./loaderblobs/mainloader.bin");

        if (!file || file->size < (6 * 1024))
        {
            log("No file or too small");
            return false;
        }

        uint32_t alignedSize = (file->size + 15) & ~15;
        uint8_t *tmpBuf = new uint8_t[ alignedSize ];
        if (tmpBuf == nullptr)
        {
            log("Could not allocate buffer");
            return false;
        }

        memcpy(tmpBuf, file->data, file->size);

        tmpBuf[4] = modeE39A >> 24;
        tmpBuf[5] = modeE39A >> 16;
        tmpBuf[6] = modeE39A >> 8;
        tmpBuf[7] = modeE39A;

        log("Uploading bootloader");

        // The morons use a weird request without the format byte..
        if (!requestDownload_24(alignedSize))
        {
            log("Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        // return false;

        testerPresent();
        if (!transferData_32(tmpBuf, 0x40004000, alignedSize, 0x80, true))
        {
            log("Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        log("Bootloader uploaded");
        delete[] tmpBuf;
    }
    else
    {
        log("Bootloader already active");
    }

    if (loader_StartRoutineById(0, 0, 0x300000))
    {
        uint8_t *dat;
        if ((dat = loader_requestRoutineResult(0)) != 0)
        {
            uint32_t retLen = (uint32_t)(dat[0] << 8 | dat[1]);
            string md5 = "";
            for (uint32_t i = 0; i < retLen; i++)
                md5 += to_hex((volatile uint32_t)dat[2 + i], 1);

            log("Success: " + md5);
            delete[] dat;
        }
    }
    // newWriteDataById(0x91, new byte[] { (byte)(delay >> 8), (byte)delay });

    uint32_t del = 250;
    uint8_t byId[2] = { (uint8_t)(del >> 8), (uint8_t)del };

    if (WriteDataByIdentifier(byId, 0x91, 2))
    {
        log("Delay set ok");
    }

    sleepMS(100);
    uint8_t *dat;
    auto timeStart = system_clock::now();

    if ((dat = loader_readMemoryByAddress(0, 0x300000, 245)) != 0)
    {
        log("Data Read ok");
        /*
        string md5 = "";
        for (uint32_t i = 0; i < 0x100; i++)
        {
            md5 += to_hex((volatile uint32_t)dat[i], 1);
            if ((i & 15) == 15)
            {
                log("Dat: " + md5);
                md5 = "";
            }
        }
        */
        delete[] dat;
    };
    uint64_t msTaken = duration_cast<milliseconds>(system_clock::now() - timeStart).count();
    uint32_t secTaken = msTaken / 1000;
    uint32_t minTaken = msTaken / 60000;
    msTaken %= 1000;
    log("Duration " + to_string(minTaken) + "m, " + to_string(secTaken) + "s, " + to_string(msTaken) + "ms");


    sleepMS(25000);
    returnToNormal();

    return true;
}




