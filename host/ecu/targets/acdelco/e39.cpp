
#include "acdelco.h"

#include "../../../tools/tools.h"

using namespace logger;
using namespace timer;
using namespace std;
using namespace std::chrono;

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

        FILE *btldr;
        size_t fileSize, alignedSize;
        uint8_t *tmpBuf;

        log("Retreiving bootloader blob..\n");

        if ( (fileSize = openRead("loaderblobs/mpc5566_loader.bin", &btldr)) == 0 )
        {
            log("Could read bootloader blob");
            return false;
        }

        if ( (tmpBuf = new uint8_t[ (alignedSize = (fileSize + 15) & ~15) ]) == nullptr )
        {
            log("Could not allocate buffer");

            fclose( btldr );
            return false;
        }

        if ( fread( tmpBuf, 1, fileSize, btldr ) != fileSize )
        {
            log("Unable to read byte from file");

            fclose( btldr );
            delete[] tmpBuf;

            return false;
        }

        fclose( btldr );

        tmpBuf[4] = modeE39 >> 24;
        tmpBuf[5] = modeE39 >> 16;
        tmpBuf[6] = modeE39 >> 8;
        tmpBuf[7] = modeE39;




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




