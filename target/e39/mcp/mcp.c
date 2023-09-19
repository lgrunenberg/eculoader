#include "common.h"
#include "crc.h"
#include "mcp.h"

// rand()
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

        sendSPIFrame( snd, rec, 16 );
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

        sendSPIFrame( snd, rec, 16 );
    }

    jumpRam( RAM_PREADDR );

    sauceReadAddress( 0xec00, 9 );

    readGMString();

    for (int i = 0; i < 16; i++)
        snd[ i ] = rand(); // (uint8_t)i;
    snd[ 0 ] = rand();
    uploadData(0x60, snd, 16);
    uploadData(0x60, snd, 16);

    sauceReadAddress( 0x60, 10 );


    // readGMString();

    // readFlashBoot();
    // readFlashBoot();
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

    return;









    printf("\n\r-- Testing 4f --\n\r");

    uint32_t cntr = 0;

    while (cntr < 16) {
        // snd[ 1 ] = (uint8_t) cntr++;
        cntr++;

        sendSPIFrame( snd, rec, 16 );
        snd[ 1 ] = 1;
        snd[ 2 ] = 1;

    }
}







