////////////////////////////////////////////////////////////////////////////////////
// 
#include "common.h"
#include "crc.h"
#include "mcp.h"

#include <stdlib.h>

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"





extern uint32_t get_Timeout();
extern void set_Timeout(const uint16_t ms);

// Alternate between these to reset
// uint8_t resetAlt[] = { 0x19, 0x06 };

static uint8_t rxSeedIdx = 0;

uint8_t lastSeedFail = 0xff;

static uint8_t txArrayIndex = 0;

uint32_t txRamDone;
uint16_t txRamProg = 0;
uint16_t rxRamAddr = 0;
uint16_t txRamAddr = 0;

// uint8_t flashBuf[16 * 1024];

uint8_t lastCommSts = 0;

uint8_t waitExtra = 0;


void printFrame(const uint8_t *snd, const uint8_t *rec) {
    printf("Snd: ");
    for (int i = 0; i < 16; i++)
        printf("%02x ", snd[i]);
    printf("\n\r");

    printf("Rec: ");
    for (int i = 0; i < 16; i++)
        printf("%02x ", rec[i]);
    
    printf("\n\r");
/*
    printf(" ( ");

    for (int i = 0; i < 16; i++)
        printf("%c ", rec[i]);
    printf(" )\n\r");*/
}

void mcp_ChecksumFrame(uint8_t *snd) {
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < 15; i++)
        checksum += snd[ i ];
    snd[15] = (uint8_t) checksum;
}

void sendSPIFrame_noWait(const uint8_t *snd, uint8_t *rec, const int nBytes) {
    const uint8_t *m_snd = snd;
    const uint8_t *m_rec = rec;

    printf("Pin: %0x\n\r", (uint16_t)(GPIOA->IDR & (1 << 8)));

    GPIOB->BRR = (1UL << 12); // Set CS low

    // 5 us
    dwtWait( 48 * 5 );

    // Wait for TX empty to be set
    while (!(SPI2->SR & SPI_I2S_FLAG_TXE))   ;

    for (int i = 0; i < nBytes; i++) {

        SPI2->DR = *snd++;

        // Wait for rx not empty to be set
        while (!(SPI2->SR & SPI_I2S_FLAG_RXNE))   ;
        *rec++ = (uint8_t)SPI2->DR;

        // Wait 100 micro between each transmission
        dwtWait( 48 * 100 );
    }

    // 82.5 us
    dwtWait( 48 * 83 );

    GPIOB->BSRR = (1UL << 12); // Set CS high

    printFrame( m_snd, m_rec );
}




uint32_t sendSPIFrame(const uint8_t *snd, uint8_t *rec, const int nBytes, const uint8_t pFrame) {
    const uint8_t *m_snd = snd;
    const uint8_t *m_rec = rec;
    uint32_t timo = 0;

    static int firstTransm = 1;
    static uint32_t oldPin = 0;

    // static uint32_t randCnt = 5000;

    uint32_t retVal = 1;

    if (firstTransm) {
        firstTransm = 0;
        oldPin = GPIOA->IDR & (1UL << 8);
    } else {
        set_Timeout( 50 );
        uint32_t nowPin;
        do {
            nowPin = GPIOA->IDR & (1UL << 8);
        } while (nowPin == oldPin && !(timo = get_Timeout()));
        
        if (timo) {
            printf("Timeout!\n\r");
            retVal = 0;
            // Pin could switch two times if state is reset in the middle of a frame
            // return 0;
        }

        oldPin = nowPin;
        // while ((GPIOA->IDR & (1 << 8)) == oldPin && !get_Timeout())  ;
    }

    // printf("Pin: %0x (tim: %x)\n\r", (uint16_t)(GPIOA->IDR & (1 << 8)), (uint16_t)timo);

    GPIOB->BRR = (1UL << 12); // Set CS low

    // 5 us
    // dwtWait( 48 * 5 );

    // Wait for TX empty to be set
    while (!(SPI2->SR & SPI_I2S_FLAG_TXE))   ;

    for (int i = 0; i < nBytes; i++) {

        SPI2->DR = *snd++;

        // Wait for rx not empty to be set
        while (!(SPI2->SR & SPI_I2S_FLAG_RXNE))   ;
        *rec++ = (uint8_t)SPI2->DR;

        // Wait 100 micro between each transmission
        dwtWait( 48 * 100 );

        // 25 bare minimum
        // dwtWait( 48 * 50 );



/*
        if ( (rand() % 100) == 50 ) {
            if ( --randCnt == 0 ) {
                printf("<< Injecting SPI transaction error >>\n\r");
                sleep( 20 );
                randCnt = 2000;
            }
        }*/

/*
        if (waitExtra != 0) {
            sleep( 2 );
            printf("Wait..\n\r");
            waitExtra = 0;
        }*/
    }

    // 82.5 us
    // dwtWait( 48 * 83 );

    GPIOB->BSRR = (1UL << 12); // Set CS high

    if (pFrame)
        printFrame( m_snd, m_rec );

    return retVal;
}

static inline uint16_t crcFrame(uint8_t *buf) {
    uint16_t crc = crcBuffer_16(&buf[3], 6, 0);
    crc = crcBuffer_16(&buf[0], 1, crc);
    buf[1] = (uint8_t) (crc >> 8);
    buf[2] = (uint8_t) crc;
    return crc;
}

int readFrame(const uint8_t *buf) {
    uint16_t crc = crcBuffer_16(&buf[3], 3, 0);
    crc = crcBuffer_16(buf, 1, crc);
    if (buf[1] != ((crc >> 8) & 0xff) || buf[2] != (crc & 0xff) ) {
        printf("-- CRC error --\n\r");
        return -1;
    }

    lastCommSts = buf[ 5 ];

    switch (txRamProg) {
    case 0: // Do nothing
        break;
    case 1: // Second return, store data
        /*
        if (rxRamAddr < (sizeof(flashBuf) - 1)) {
            flashBuf[ rxRamAddr + 0 ] = buf[ 14 ];
            flashBuf[ rxRamAddr + 1 ] = buf[ 15 ];
        } else {
            printf("Ram Addr out of bounds!!!\n\r");
        }*/
        txRamDone = 1;
        // 1 -> 0, 2 -> 1
    default:
        txRamProg--;
        break;
    }

    // Retrieve remote seed index
    rxSeedIdx = buf[ 3 ] & 7;

    return 0;
}

// Sign a regular frame (ie where mcp is in a operational state and not in boot mode)
void prepOperationalFrame(uint8_t *buf) {
    static const uint8_t seedKeys[] = {
        0x55,
        0xaa,
        0x33,
        0xcc,
        0xee,
        0x11,
        0xf0,
        0x0f
    };

    // First step is 0x20
    // then 00, 10, 20 ..
    static int spiStep = 2;
    static uint8_t stArr[] = { 0x00, 0x10, 0x20 };

    buf[0] = (buf[0] & 0xcf) | stArr[ spiStep++ ];
    if (spiStep >= 3)
        spiStep = 0;

    // Power down fuel pump and OR in spi status
    //  (lastCommSts & (1 << 6));
    buf[ 3 ] = (1 << 4) | (lastCommSts & (1 << 6));

    // Echo seed index. Set engine off
    buf[ 4 ] = rxSeedIdx | (1 << 6);

    // Key calc HERE!
    buf[ 5 ] = seedKeys[ rxSeedIdx & 7 ];

    // Set array index
    buf[ 9 ] = txArrayIndex;

    if (txRamProg == 2) {
        buf[ 14 ] = (uint8_t)(txRamAddr>>8);
        buf[ 15 ] = (uint8_t)(txRamAddr);
    } else {
        buf[ 14 ] = 0;
        buf[ 15 ] = 0;
    }

    crcFrame( buf );
}

void enterBoot(void) {
    uint8_t rec[16], snd[16] = { 0x0f };
    int cnt = 10;

    printf("\n\r-- Entering boot --\n\r");

    while (cnt--) {
        prepOperationalFrame(snd);
        sendSPIFrame_noWait(snd, rec, 16);
        readFrame( rec ); // Retrieve important data
    }
}

void getBootDescs(void) {
    uint8_t rec[16], snd[16] = { 0xaf, 0x00 , 0x00 };
    uint32_t cntr = 0;
    printf("\n\r-- Fetching HEX descriptors --\n\r");
    while (cntr < 4) {
        snd[ 1 ] = (uint8_t) cntr++;
        sendSPIFrame( snd, rec, 16, 1 );
    }
}

void prepErase(void) {
    uint8_t rec[16], snd[16] = { 0x4f };
    uint32_t cntr = 0;
    printf("\n\r-- Prep erase --\n\r");
    while (cntr < 4) {
        snd[ 1 ] = (uint8_t) cntr++;
        sendSPIFrame( snd, rec, 16, 1 );
    }
}

void testErase(void) {
    uint8_t rec[16], snd[16] = { 0x6f, 0x00 , 0x01, 0x00 };
    uint32_t cntr = 0;
    printf("\n\r-- Test erase --\n\r");
    while (cntr < 8) {
        snd[ 1 ] = (uint8_t) cntr++;
        sendSPIFrame( snd, rec, 16, 1 );
    }
}




// Alternate between 06/19
void bootSecretStep(uint8_t *snd) {
    static uint32_t step = 0;
    static const uint8_t altByt[] = { 0x06, 0x19 };
    snd[ 0 ] = altByt[ (step++) & 1 ];
}

// < XX 01 > Enter secret section by sending AT LEAST four commands prefixed with 06/19
// "ping" is the least intrusive since it does literally nothing to the internal state except for giving a response
void enterSecretBoot(void) {

    uint8_t rec[16], snd[16] = { 0x00, 0x01 };
    int cnt = 10;

    printf("\n\r-- Entering secret boot --\n\r");

    while (cnt--) {
        bootSecretStep( snd );
        mcp_ChecksumFrame( snd );
        sendSPIFrame( snd, rec, 16, 1 );
        // sleep ( 5 );
    }
}

void sendSecretPing(void) {
    uint8_t rec[16], snd[16] = { 0x00, 0x01 };
    bootSecretStep( snd );
    mcp_ChecksumFrame( snd );
    sendSPIFrame( snd, rec, 16, 1 );
}




// < XX 02 > Read anything in addressable memory space
void sauceReadAddress(const uint32_t address, const uint8_t count) {
    uint8_t rec[16], snd[16] = {
        0x19,       // Step
        0x02,       // Command
        (uint8_t)(address>>8), (uint8_t)address, // Address
        count       // Count
    };
    int cnt = 10;

    printf("\n\r-- Test read --\n\r");

    while (cnt--) {
        bootSecretStep( snd );
        mcp_ChecksumFrame( snd );
        sendSPIFrame( snd, rec, 16, 1 );
    }
}

// < XX 04 > Jump to ram address
void jumpRam(const uint32_t address) {
    uint8_t rec[16], snd[16] = { 0x19, 0x04 };
    printf("\n\r-- Test RAM jump --\n\r");

    snd[2] = (uint8_t)(address>>8);
    snd[3] = (uint8_t)address;

    bootSecretStep( snd );
    mcp_ChecksumFrame( snd );
    sendSPIFrame( snd, rec, 16, 1 );
}


// < XX 05 > Return boot firmware ascii tag
void readGMString(void) {
    uint8_t rec[16], snd[16] = { 0x19, 0x05 };
    int cnt = 10;
    printf("\n\r-- Test GM tag --\n\r");

    while (cnt--) {
        bootSecretStep( snd );
        mcp_ChecksumFrame( snd );
        sendSPIFrame( snd, rec, 16, 1 );
    }
}

// < XX 06 > Exit loader
void exitLoader(void) {
    uint8_t rec[16], snd[16] = { 0x19, 0x06 };
    int cnt = 10;
    printf("\n\r-- Exit loader --\n\r");

    while (cnt--) {
        bootSecretStep( snd );
        mcp_ChecksumFrame( snd );
        sendSPIFrame( snd, rec, 16, 1 );
    }
}

// < XX 07 > Set flash protection
void setFlashProt(const uint8_t msk) {
    uint8_t rec[16], snd[16] = { 0x19, 0x07 };
    int cnt = 10;

    snd[2] = msk;

    printf("\n\r-- Set protection --\n\r");

    while (cnt--) {
        bootSecretStep( snd );
        mcp_ChecksumFrame( snd );
        sendSPIFrame( snd, rec, 16, 1 );
    }
}