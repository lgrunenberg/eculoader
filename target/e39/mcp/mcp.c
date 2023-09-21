////////////////////////////////////////////////////////////////////////////////////
// 
#include "common.h"
#include "crc.h"
#include "mcp.h"

#include <stdlib.h>

#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"








// static uint8_t seed[3];

extern uint32_t get_Timeout();
extern void set_Timeout(const uint16_t ms);



#define MAX_BYTES  ( 10 )
#define RAM_PREADDR  ( 0x00b0 )

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

void mcpUploadTest(void) {
    uint32_t address = RAM_PREADDR;
    uint32_t bytesLeft = mcp_loaderSize;
    uint8_t snd[16] = { 0x19, 0x03 };
    uint8_t rec[16];
    uint8_t *dPtr = mcp_loader;

    readGMString();

    printf("\n\r-- Test upload of loader --\n\r");

    while (bytesLeft > 0) {
        uint32_t thisSize = (bytesLeft > MAX_BYTES) ? MAX_BYTES : bytesLeft;
        uint32_t checksum = 0;
        
        snd[2] = (uint8_t)(address>>8);
        snd[3] = (uint8_t)address;
        snd[4] = (uint8_t)thisSize;

        for (uint32_t i = 0; i < thisSize; i++)
            snd[ 5 + i ] = *dPtr++;

        address += thisSize;
        bytesLeft -= thisSize;

        bootSecretStep( snd );

        for (uint32_t i = 0; i < 15; i++)
            checksum += snd[ i ];
        snd[15] = (uint8_t) checksum;

        sendSPIFrame( snd, rec, 16 , 1 );
    }

    jumpRam( RAM_PREADDR );

    sauceReadAddress( 0xec00, 9 );

    readGMString();

    for (int i = 0; i < 16; i++)
        snd[ i ] = rand(); // (uint8_t)i;
    snd[ 0 ] = rand();
    uploadData(0x60, snd, 16);
    uploadData(0x60, snd, 16);

    sleep( 500 );

    sauceReadAddress( 0x60, 10 );

    // setFlashProt( 0xff );


    // sleep( 250 );

    // waitExtra = 1;

    setFlashProt( 0xff );

    // readGMString();

    // readFlashBoot();
    // readFlashBoot();
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
    csumFrame( snd );

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
    csumFrame( snd );
    return sendSPIFrame( snd, rec, 16, 0 );
}

// Snd: 19 02 00 60 0a 00 00 00 00 00 00 00 00 00 00 85
// Rec: 06 82 00 60 0a d9 2d cf 46 29 04 b4 78 d8 68 a6
uint32_t dumpOk(const uint32_t addr, const uint32_t len, const uint8_t *buf) {
    // return ((buf[2] << 8 | buf[3]) == addr && buf[4] == len && buf[1] == 0x82) ? 1 : 0;

    if (buf[1] != 0x82) {
        printf("dumpOk: error cmd\n\r");
        return 0;
    }

    uint32_t tmpAddr = buf[2] << 8 | buf[3];

    if (tmpAddr != addr) {
        printf("dumpOk: error addr (%04x %04x)\n\r", (uint16_t)tmpAddr, (uint16_t)addr);
        return 0;
    }

    if (buf[4] != len) {
        printf("dumpOk: error len (%u : %u)\n\r", buf[4], (uint16_t)len);
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




uint32_t readChunk(uint32_t startAddr, uint32_t totLen, uint8_t *buf) {

    // Must store the whole response
    uint8_t tmp[16];

    uint32_t retries = 0;
    uint32_t restart = 1;

    uint32_t len = 10;
    uint32_t lastLeft = totLen;
    uint32_t lastStart = startAddr;
    uint32_t lastLen = len;

    while (totLen > 0) {

        if (totLen < len)
            len = totLen;

sendRead:
        // Failed due to frame sync or no pin toggle
        if ( !ldrReadAddr(startAddr, len, tmp, 0 ) ) {
            printf("Fail 1\n\r");
            if (++retries > 10)
                break;
            restart = 1;
            goto sendRead;
        }

        if ( restart ) {
            // We don't have any data to work on. Go ahead with the next read and take things from there
            restart = 0;
        } else {
            // Received buffer does not point at expected address
            if ( !dumpOk(lastStart, lastLen, tmp) ) {
                printf("Fail 2\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;
                goto sendRead;
            }

            for (uint32_t i = 0; i < lastLen; i++)
                *buf++ = tmp[5 + i];
        }

        lastStart = startAddr;
        lastLen = len;
        lastLeft = totLen;

        startAddr += len;
        totLen -= len;

        if ( totLen == 0 ) {
            // printf("Swooping up last bytes..\n\r");

            if ( !ldrIdle( tmp ) ) {
                printf("Fail 3\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;
                goto sendRead;
            }

            // Received buffer does not point at expected address
            if ( !dumpOk(lastStart, lastLen, tmp) ) {
                printf("Fail 4\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                startAddr = lastStart;
                len = lastLen;
                totLen = lastLeft;
                goto sendRead;
            }

            for (uint32_t i = 0; i < lastLen; i++)
                *buf++ = tmp[5 + i];
        }
    }

    return (retries < 11) ? 1 : 0;
}

void fullDump(void) {

    uint8_t tmp[512];

    // uint32_t start = 0xbe00;
    printf("\n\r-- fullDump --\n\r");

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

        readChunk(0xbe00 + i, toRead, tmp);

        if ( !memCompare(&mcp_bin[i], tmp, toRead) ) {
            printf("Error: Severe error. Data missmatch\n\r");
            break;
        }

        i+= toRead;
    }

    dwtStart = DWT->CYCCNT - dwtStart;
    dwtStart =  dwtStart / (48 * 1000);

    printf("Took %u ms\n\r", (uint16_t)dwtStart);

}

/*
void fullDump(void) {

    // Must store the whole response
    uint8_t tmp[16];


    uint32_t retries = 0;
    uint32_t restart = 1;

    printf("\n\r-- fullDump --\n\r");

    uint32_t bytesLeft = 16 * 1024;
    uint32_t start = 0;
    uint32_t len = 10;
    uint32_t lastLeft = bytesLeft;
    uint32_t lastStart = start;
    uint32_t lastLen = len;

    uint32_t dwtStart = DWT->CYCCNT;

    while (bytesLeft > 0) {

        if (bytesLeft < len)
            len = bytesLeft;

sendRead:
        // Failed due to frame sync or no pin toggle
        if ( !ldrReadAddr(0xbe00 + start, len, tmp, bytesLeft > 64 ? 0 : 1 ) ) {
            printf("Fail 1\n\r");
            if (++retries > 10)
                break;
            restart = 1;
            goto sendRead;
        }

        if ( restart ) {
            // We don't have any data to work on. Go ahead with the next read and take things from there
            restart = 0;
        } else {
            // Received buffer does not point at expected address
            if ( !dumpOk(0xbe00 + lastStart, lastLen, tmp) ) {
                printf("Fail 2\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                start = lastStart;
                len = lastLen;
                bytesLeft = lastLeft;
                goto sendRead;
            }

            if ( !memCompare(&mcp_bin[lastStart], &tmp[5], lastLen) ) {
                printf("Error: Severe error. Data missmatch\n\r");
                break;
            }
        }

        lastStart = start;
        lastLen = len;
        lastLeft = bytesLeft;

        start += len;
        bytesLeft -= len;

        if ( bytesLeft == 0 ) {
            printf("Swooping up last bytes..\n\r");

            if ( !ldrIdle( tmp ) ) {
                printf("Fail 3\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                start = lastStart;
                len = lastLen;
                bytesLeft = lastLeft;
                goto sendRead;
            }

            // Received buffer does not point at expected address
            if ( !dumpOk(0xbe00 + lastStart, lastLen, tmp) ) {
                printf("Fail 4\n\r");
                if (++retries > 10)
                    break;
                restart = 1;
                start = lastStart;
                len = lastLen;
                bytesLeft = lastLeft;
                goto sendRead;
            }

            if ( !memCompare(&mcp_bin[lastStart], &tmp[5], lastLen) ) {
                printf("Error: Severe error. Data missmatch\n\r");
                break;
            }

        }
    }

    dwtStart = DWT->CYCCNT - dwtStart;
    dwtStart =  dwtStart / (48 * 1000);

    printf("Retries: %u, %u ms\n\r", (uint16_t)retries, (uint16_t)dwtStart);
}*/


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







    fullDump();









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







