////////////////////////////////////////////////////////////////////////////////////
// MCP bootloader

#include "common.h"
#include "crc.h"
#include "mcp.h"

#include <stdlib.h>

#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"



/*
Annoying to save this file every time the loader has been changed
asm (
".section .rodata\n.global mcp_loader\n.global mcp_loaderSize\n"
"mcp_loader:\n.incbin \"target/bin/loader.bin\"\n"
"mcp_loaderSize: .long (mcp_loaderSize - mcp_loader)\n"
);

extern const uint8_t mcp_loader[];
extern const uint32_t mcp_loaderSize;
*/



void mcpUploadTest(void) {
    uint32_t address = LDR_ADDRESS;
    uint32_t bytesLeft = mcp_loaderSize;
    uint8_t snd[16] = { 0x19, 0x03 };
    uint8_t rec[16];
    const uint8_t *dPtr = mcp_loader;

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

    jumpRam( LDR_ADDRESS );


    sleep( 100 ) ;
    sauceReadAddress( 0xec00, 9 );

    readGMString();

    for (int i = 0; i < 16; i++)
        snd[ i ] = rand(); // (uint8_t)i;
    snd[ 0 ] = rand();
    uploadData(0x60, snd, 16);
    uploadData(0x60, snd, 16);

    sleep( 500 );

    sauceReadAddress( 0x60, 10 );

    // sauceReadAddress( 0x60, 0 );
    // sauceReadAddress( 0x60, 11 );

    // setFlashProt( 0xff );


    // sleep( 250 );

    // waitExtra = 1;

    setFlashProt( 0xff );

    // readGMString();

    // readFlashBoot();
    // readFlashBoot();
}