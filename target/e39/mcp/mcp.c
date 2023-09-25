////////////////////////////////////////////////////////////////////////////////////
// 
#include "common.h"
#include "crc.h"
#include "mcp.h"

#include <stdlib.h>

#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"


asm (
".section .rodata\n.global mcp_bin\n.global mcp_binSz\n"
"mcp_bin:\n.incbin \"compbin.bin\"\n"
"mcp_binSz: .long (mcp_binSz - mcp_bin)\n"
);

extern const uint8_t mcp_bin[];
extern const uint32_t mcp_binSz;




// static uint8_t seed[3];

extern uint32_t get_Timeout();
extern void set_Timeout(const uint16_t ms);




/*
Some specs
SPI freq: 1 MHz


FIRMWARE mode:
5 us CS hold
8 + 100 us / byte
82.5 us CS post hold
100 us cooldown between frames

== 1915.5 us / frame ==





Recovery / bootloader (pin-polled mode):
_NO_ CS hold
8 + 50 us / byte (Tested working down to 8 + 25 us)
_NO_ CS post hold
_NO_ cooldown (you read the pin to determine when it's ready)

== 928 us / frame - THEORETICAL - ==
You'll obv never see this since it has to process the request, reset counters etc etc but 10 read bytes / 950 us seem possible


e39 specific:
SPI A CS0 -> hc08 MCP
SPI A CS1 -> PIC12F MCU

SPI B various TPIC chips. Haven't really investigated since they're not of much interest



*/









void uploadData(const uint32_t addr, const uint8_t *data, const uint32_t len) {
    uint32_t address = addr;
    uint8_t snd[16] = { 0x19, 0x03 };
    uint8_t rec[16];
    uint32_t bytesLeft = len;

    printf("\n\r-- Test upload of data --\n\r");

    while (bytesLeft > 0) {
        uint32_t thisSize = (bytesLeft > MAX_BYTES) ? MAX_BYTES : bytesLeft;
        uint32_t checksum = 0;
        
        snd[2] = (uint8_t)(address>>8);
        snd[3] = (uint8_t)address;
        snd[4] = (uint8_t)thisSize;

        for (uint32_t i = 0; i < thisSize; i++)
            snd[ 5 + i ] = *data++;

        address += thisSize;
        bytesLeft -= thisSize;

        bootSecretStep( snd );

        for (uint32_t i = 0; i < 15; i++)
            checksum += snd[ i ];
        snd[15] = (uint8_t) checksum;

        sendSPIFrame( snd, rec, 16, 1 );
    }
}



// < XX 02 > Read anything in addressable memory space
uint32_t ldrReadAddr(const uint32_t address, const uint8_t count, uint8_t *dat, const uint8_t pFrame) {
    uint8_t rec[16], snd[16] = {
        0x19,       // Step
        0x02,       // Command
        (uint8_t)(address>>8), (uint8_t)address, // Address
        count       // Count
    };

    if ( count < 1 || count > 10 ) {
        printf("ldrReadAddr: Count err\n\r");
        return 0;
    }

    bootSecretStep( snd );
    mcp_ChecksumFrame( snd );

    if ( !sendSPIFrame( snd, rec, 16, pFrame ) ) {
        printf("ldrReadAddr: sendSPIFrame err\n\r");
        return 0;
    }

    for (uint32_t i = 0; i < 16; i++)
        *dat++ = rec[ i ];

    return 1;
}

uint32_t ldrIdle(uint8_t *rec) {
    uint8_t snd[16] = { 0x00, 0x01 };
    bootSecretStep( snd );
    mcp_ChecksumFrame( snd );
    return sendSPIFrame( snd, rec, 16, 0 );
}

// Snd: 19 02 00 60 0a 00 00 00 00 00 00 00 00 00 00 85
// Rec: 06 82 00 60 0a d9 2d cf 46 29 04 b4 78 d8 68 a6
uint32_t dumpOk(const uint16_t addr, const uint8_t len, const uint8_t *buf) {
    // return ((buf[2] << 8 | buf[3]) == addr && buf[4] == len && buf[1] == 0x82) ? 1 : 0;

    if (buf[1] != 0x82) {
        printf("dumpOk: error cmd\n\r");
        return 0;
    }

    if ((buf[2] << 8 | buf[3]) != addr) {
        printf("dumpOk: error addr (%04x %04x)\n\r", (buf[2] << 8 | buf[3]), addr);
        return 0;
    }

    if (buf[4] != len) {
        printf("dumpOk: error len (%u : %u)\n\r", buf[4], len);
        return 0;
    }

    uint8_t csum = 0;

    for (int i = 0; i < 15; i++)
        csum += buf[i];
    
    if (buf[15] != csum) {
        printf("dumpOk: error csum (%u : %u)\n\r", buf[15], csum);
        return 0;
    }

    return 1;
}

uint32_t memCompare(const uint8_t *bufA, const uint8_t *bufB, const uint32_t len) {
    for (uint32_t i = 0; i < len; i++)
        if ( bufA[i] != bufB[i ] )
            return 0;
    return 1;
}


uint32_t failCnt1r0 = 0;
uint32_t failCnt1r1 = 0;
uint32_t failCnt2 = 0;
uint32_t failCnt3 = 0;
uint32_t failCnt4 = 0;

uint32_t overFlow1 = 0;
uint32_t overFlow2 = 0;
uint32_t failCnt = 0;
uint32_t succCnt = 0;
uint32_t bytesTransf = 0;

static void printErrStats(void) {
    printf("f0r0 %9lu, f0r1 %9lu\n\r", failCnt1r0, failCnt1r1);
    printf("f2   %9lu, f3   %9lu, f4 %9lu\n\r", failCnt2, failCnt3, failCnt4);
    printf("ovf1 %9lu, ovf2 %9lu\n\r", overFlow1, overFlow2);
    printf("Fail counter   : %lu\n\r", failCnt);
    printf("Success counter: %lu\n\r", succCnt);
    printf("Byte count     : %lu\n\r", bytesTransf);
}

uint32_t readChunk(uint32_t startAddr, uint32_t totLen, uint8_t *buf) {

    // Must store the whole response
    uint8_t tmp[16];

    uint32_t retries = 0;
    uint32_t restart = 1;

    uint32_t len = 10;
    uint32_t lastLeft = totLen;
    uint32_t lastStart = startAddr;
    uint32_t lastLen = len;

    uint32_t bufLim = totLen;
    uint32_t bufCnt = 0;

    while (totLen > 0) {

sendRead:
        if (totLen < len)
            len = totLen;

        // Failed due to frame sync or no pin toggle
        if ( !ldrReadAddr( startAddr, len, tmp, 0 ) || (rand() % 100) == 50 ) {
            
            if ( restart == 0 ) { /* printf("Fail 1 ( 0 )\n\r"); */ failCnt1r0++; }
            else {                /* printf("Fail 1 ( 1 )\n\r"); */ failCnt1r1++; }

            if (++retries > 10)
                return 0;

            restart = 1;
            startAddr = lastStart;
            len = lastLen;
            totLen = lastLeft;

            goto sendRead;
        }

        if ( restart ) {
            // We don't have any data to work on. Go ahead with the next read and take things from there
            restart = 0;
        } else {
            // Received buffer does not point at expected address
            if ( !dumpOk(lastStart, lastLen, tmp) || (rand() % 100) == 50 ) {

                // printf("Fail 2\n\r");
                failCnt2++;

                if (++retries > 10)
                    return 0;

                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;

                goto sendRead;
            }

            for (uint32_t i = 0; i < lastLen; i++) {
                if ( ++bufCnt > bufLim ) {
                    printf("Buffer overrun location 1!\n\r");
                    overFlow1++;
                    return 0;
                }
                *buf++ = tmp[5 + i];
                
            }
        }

        lastStart = startAddr;
        lastLen = len;
        lastLeft = totLen;

        startAddr += len;
        totLen -= len;

        if ( totLen == 0 ) {
            // printf("Swooping up last bytes..\n\r");

            if ( !ldrIdle( tmp ) || (rand() % 100) == 50 ) {

                // printf("Fail 3\n\r");
                failCnt3++;

                if (++retries > 10)
                    return 0;

                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;

                goto sendRead;
            }

            // Received buffer does not point at expected address
            if ( !dumpOk(lastStart, lastLen, tmp) || (rand() % 100) == 50 ) {

                // printf("Fail 4\n\r");
                failCnt4++;

                if (++retries > 10)
                    return 0;

                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;

                goto sendRead;
            }

            for (uint32_t i = 0; i < lastLen; i++) {
                if ( ++bufCnt > bufLim ) {
                    printf("Buffer overrun location 2!\n\r");
                    overFlow2++;
                    return 0;
                }
                *buf++ = tmp[5 + i];
            }
        }
    }

    return 1;
}

uint8_t tmpBuf[ 1024 ];

uint32_t fullDump(void) {

    uint16_t steps = 0;

    // uint32_t start = 0xbe00;
    // printf("\033[XA");
    printf("\033[H");
    // printf("\n\r-- fullDump --\n\r");

    uint32_t dwtStart = DWT->CYCCNT;

    // for (uint32_t i = 0; i < (1024 * 16); i+=256) {
    // rand();
    uint32_t i = 0;
    uint32_t totSize = (1024 * 16);

    while (i < totSize) {

        uint32_t toRead = rand() & 511;
        if (toRead == 0) toRead = 1;
        if (i + toRead > totSize)
            toRead = totSize - i;

        if ( !readChunk(0xbe00 + i, toRead, tmpBuf) ) {
            return 0;
            break;
        }
        steps++;

        if ( !memCompare(&mcp_bin[i], tmpBuf, toRead) ) {
            printf("Error: Severe error. Data missmatch\n\r");
            return 0;
            break;
        }

        bytesTransf += toRead;
        i += toRead;
    }

    dwtStart = DWT->CYCCNT - dwtStart;
    dwtStart =  dwtStart / (48 * 1000);

    printf("Took %u ms (%u steps)\n\r", (uint16_t)dwtStart, steps);
    return 1;
}

void mcpPlay(void) {

    uint8_t snd[16] = { 0x4f, 0x00 , 0x00 };
    uint8_t rec[16] = { 0 };

    printf("\033[2J");

    enterBoot();

    // printf("\n\r-- In boot --\n\r");

    // sleep( 10 );
    // getBootDescs();

    // prepErase();

    // testErase();
    
    // sleep( 500 );
    
    enterSecretBoot();
    enterSecretBoot();
    // enterSecretBoot();

    sleep( 10 );
    
    // readFlashBoot();
    // readGMString();

    mcpUploadTest();

    while (1) ;

    printf("\033[2J");

    while (1) {
        srand( DWT->CYCCNT );

        if ( fullDump() )
            succCnt++;
        else
            failCnt++;

        // printf("Succ: %u | Fail: %u\n\r", succCnt, failCnt);
        printErrStats();
    }




    return;









    printf("\n\r-- Testing 4f --\n\r");

    uint32_t cntr = 0;

    while (cntr < 16) {
        // snd[ 1 ] = (uint8_t) cntr++;
        cntr++;

        sendSPIFrame( snd, rec, 16, 1 );
        snd[ 1 ] = 1;
        snd[ 2 ] = 1;

    }
}







