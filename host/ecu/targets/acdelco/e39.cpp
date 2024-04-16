
#include "acdelco.h"

#include "../../../adapter/message.h"

#include "../../../tools/tools.h"


#include <cstring>

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

// Calculated key e7f0 from seed a7a1

static void calculateKeyForE39(uint8_t *seed)
{
    uint32_t _seed = seed[0] << 8 | seed[1];
    uint32_t origSeed = _seed;

    if ( _seed != 0xffff )
    {
        _seed = (_seed + 0x6C50) & 0xFFFF;
        _seed = ((_seed << 8 | _seed >> 8) - 0x22DA) & 0xffff;
        _seed = ((_seed << 9 | _seed >> 7) - 0x8BAC);
    }
    else
    {
        _seed = 1;
    }

    _seed &= 0xffff;

    log("Calculated key " + to_hex(_seed, 2) + " from seed " + to_hex(origSeed, 2));

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
    else
    {
        calculateKeyForE39( &ret[4] );
    }

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

/*
7e8 E 017eaaaaaaaaaaaa     1,362
7e0 H 053400001f200000   103,245
7e8 E 037f3422aaaaaaaa     3,004
*/




bool e39::initSessionE39()
{
    fileManager fm;
    string ident = "";
    uint8_t *tmp;



    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

    log("Checking loader state..");

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

        if ( !InitiateDiagnosticOperation(3) ) // 10
        {
            log("initDiag error");
            return false;
        }

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



        sleepMS( 100 );



        fileHandle *file = fm.open("../mpc5566/out/loader.bin");

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

        tmpBuf[4] = modeE39 >> 24;
        tmpBuf[5] = modeE39 >> 16;
        tmpBuf[6] = modeE39 >> 8;
        tmpBuf[7] = modeE39;

        log("Uploading bootloader");

        // The morons use a weird request without the format byte..
        if (!requestDownload_24( alignedSize ))
        {
            // The 39a log shows the ECU outright refusing but still accepting a transfer after. Weird fella..

            log("Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

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

    return true;
}

///////////////////////////////////////////////////////////
// e39a dump

/*
static key at    0x7C8
static seed at   0x7D0

SDA regs
< Recovery >
r13: 0x40021000

< Main >
r13: 0x40021000
r14: 0x40001000
r15: 0x40011000

0x40015264 - secAccLocked     , byte
0x40015259 - progModeEnabled  , byte

< Blocked reads >
400003a0
400003b0
400003c0
400003d0
400003e0
400003f0
40000400
40000410
40000420

40000870
40000880

< Outright crashed while trying to read these >
40007bc0
40007bd0
*/


bool e39::getSecBytes(uint32_t secLockAddr, uint32_t progModeAddr)
{
    uint8_t *b;
    
    if ( (b = readMemoryByAddress_32_16(secLockAddr, 1, 1)) != nullptr )
    {
        log("secLock: " + to_hex((volatile uint32_t)*b, 1));
        delete[] b;
    }

    if ( (b = readMemoryByAddress_32_16(progModeAddr, 1, 1)) != nullptr )
    {
        log("progMode: " + to_hex((volatile uint32_t)*b, 1));
        delete[] b;
    }

    return true;
}

bool e39::initSessionE39A()
{
    fileManager fm;
    string ident = "";
    uint8_t *tmp;



    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

    log("Checking loader state..");

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

// TODO: Investigate why it SOMETIMES wants this to be two
        if ( !InitiateDiagnosticOperation(3) ) // 10
        {
            log("initDiag error");
            return false;
        }

        sleepMS(10);

        sleepMS(10);

        disableNormalCommunication(); // 28
        sleepMS(10);

        testerPresent();

        if (!secAccE39( 1 ))
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

        sleepMS( 100 );

        fileHandle *file = fm.open("../mpc5566/out/loader.bin");

        if (!file || file->size < (6 * 1024))
        {
            log("No file or too small");
            return false;
        }

        log("Really here");

        uint32_t alignedSize = (file->size + 15) & ~15;
        uint8_t *tmpBuf = new uint8_t[ alignedSize ];
        if (tmpBuf == nullptr)
        {
            log("Could not allocate buffer");
            return false;
        }

        memcpy(tmpBuf, file->data, file->size);

        // Mine couldn't care less?
/*
        tmpBuf[4] = modeE39A >> 24;
        tmpBuf[5] = modeE39A >> 16;
        tmpBuf[6] = modeE39A >> 8;
        tmpBuf[7] = modeE39A;
*/

        // Just to check if it returned states for some weird reason
        getSecBytes( 0x40015264, 0x40015259 );

        log("Uploading bootloader");

        // The morons use a weird request without the format byte..
        if (!requestDownload_24( alignedSize ))
        {
            // The 39a log shows the ECU outright refusing but still accepting a transfer after. Weird fella..

            log("Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

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

    return true;
}

bool e39::play()
{
    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

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


    uint8_t *b;

    if ( !InitiateDiagnosticOperation(2) ) // 10
    {
        log("Init diag error");
        return false;
    }

    sleepMS(10);

    disableNormalCommunication(); // 28
    sleepMS(100);

    testerPresent();

    // Not allowed to read more than 16 bytes per chunk on e39/a
    const uint32_t chunkSz = 16;


    const uint32_t agSz = 16;
    uint32_t totLen = 128 * 1024;

    uint32_t lenLeft = totLen;

    uint32_t bufIdx = 0;
    uint32_t rdAddr = 0x40000000;


    uint8_t *buf = new uint8_t[ totLen ];

    memset(buf, 0x00, totLen);

    while ( lenLeft != 0 )
    {
        testerPresent();

        // Not allowed to read more than 16 bytes in one go!
        if ( (b = readMemoryByAddress_32_16(rdAddr, agSz, chunkSz)) != nullptr )
        {
            
            memcpy(&buf[bufIdx], b, agSz);

            delete[] b;
        }
        else
        {
            log("Failed addres: " + to_hex(rdAddr, 4));
        }

        bufIdx += agSz;
        rdAddr += agSz;
        lenLeft -= agSz;

    }

    fileManager fm;
    fm.write("ramdump.bin", buf, totLen);

    delete[] buf;

    return false;
}


bool e39::dump()
{
    log("Dump");


    // bamFlash(0x40004000);
    // return false;

    configProtocol();

    // play();

    initSessionE39A();

    // return false;

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
