//////////////////////////////////////////////////////
// This part of the loader handles house keeping tasks:
// Configure most of the hardware except FlexCAN.
// Help out with interrupts, recover from ECC exceptions etc.
// Extract the main image.

#include "preloader.h"
#include "mpc55xx.h"
#include "../config.h"

// Os f*cks with or / and during while operations of volatile memory
#pragma GCC push_options
#pragma GCC optimize ("O1")

typedef struct {
    uint32_t *ptr;
    uint32_t data;
} runInit_t;

typedef struct {
    const uint32_t checksum;
    const uint32_t sizeIn;
    const uint8_t  data[];
} lzImage_t;

extern uint8_t __s_mainLoader[];

static uint32_t mainReady = 0;

//////////////////////////////////////////////////////
// TODO:
// Determine if SRAM ECC exceptions are worth the hassle
// Thing is, ECU is fried if one were to happen...
//
// A few other exceptions

// Service external watchdog on e78
#ifndef BAMMODE
static void broadcast_e78SPIWatchdog()
{
    static   uint16_t dArr[]       = { 0x542C, 0, 0 };
    volatile uint32_t *DSPId_sr    = (volatile uint32_t *)0xFFF9C02C;
    volatile uint32_t *DSPId_PUSHR = (volatile uint32_t *)0xFFF9C034;
    // volatile uint32_t *DSPId_POPR  = (volatile uint32_t *)0xFFF9C038;
    *dArr ^= 0x3E00; // Notice static!

    for (uint32_t i = 0; i < 3; i++)
    {
        *DSPId_sr |= 0x80000000;
        uint32_t data = 0x90010000 | dArr[i]; // CONT, CTAR1, CS0

        if (i == 2)
            data &= 0x3FFFFF;

        *DSPId_PUSHR = data;
        while (! ((*DSPId_sr) & 0x80000000) )   ;
    }
}
#endif

#ifdef enableDebugBox
static void m_sendDebug(const uint32_t step, const uint32_t extra)
{
    static uint32_t stSent = 0;
    static uint32_t exSent = ~0;
    if (mainReady && ( stSent != step  ||
                       exSent != extra ))
    {
        stSent = step;
        exSent = extra;
        sendDebug(step,extra);
    }
}
#endif

// Do not refer to this one directly!
// Install "intHelper" as a pointer.
// the interrupt attribute is ignored so this is the best I could do..
// Figure out who called and why
uint32_t hardArbiter(const uint32_t hardVec)
{
    switch (hardVec)
    {

    /// Data storage exceptions
    // This fucker can be thrown by many internal devices (unfortunately)
    // It's up to the code to figure out who's actually shouting and why.
    // Implement SRAM ECC handler?
    case 0x30:

        // FLASH ECC error
        if (FLASHREGS.MCR.direct & FLASH_MCR_EER_MSK) {
            FLASHREGS.MCR.direct |= FLASH_MCR_EER_MSK;
            return 1;
        }

        // Read while write error
        else if (FLASHREGS.MCR.direct & FLASH_MCR_RWE_MSK)
            FLASHREGS.MCR.direct |= FLASH_MCR_RWE_MSK;

#ifdef enableDebugBox
        // Something else caused this interrupt, oh sh!t
        else m_sendDebug(0x3636, hardVec);
#endif
        break;


    /// "Software" interrupts
    // If main has been extracted, jump there.
    case 0x50:
        if (mainReady)
            softArbiter(((*(volatile uint32_t *)0xFFF48010)&0x7FF)>>2);
        break;


    /// IVOR11, Fixed Interval Interrupt
    // Internal timer
    case 0xD0:
#ifndef BAMMODE
        if (modeWord == MODE_E39)
        {
            SIU_GPDO[ 4 ] = 0;
            for (uint32_t i = 0; i < 32; i++)
                __asm volatile ("nop");
            SIU_GPDO[ 4 ] = 1;
        }
        else if (modeWord == MODE_E78)
        {
            broadcast_e78SPIWatchdog();
            broadcast_e78SPIWatchdog();
        }
#endif

#ifdef enableBroadcast
        msTimer += 8;
#endif

        ackTSR();
        break;

    /// I don't know this caller (or, more precisely, didn't expect the code to ever cause it)
    default:
#ifdef enableDebugBox
        m_sendDebug(0x3635, hardVec);
#endif
        break;
    }

    return 0;
}

/// Generate interrupt table
// One can only branch +- 7fff if I understand this instruction correctly.
// Maybe make sure intEntry() is as near as possible to the table?
static void setupIntTable()
{
    // Base of SRAM
    uint32_t *ptr = (uint32_t*)tablebase;

    // Set up a table totaling 512 bytes
    for (uint32_t i = 0; i < 32; i++) // ( 0 to 15 + 32 to 34 + padding to make it nice and even)
    {
        // uint32_t branch = (((uint32_t) &intEntry) - ((uint32_t) ptr)) - 12;
        *ptr++ = 0x3821FFFC;          // addi   %sp, %sp, -4  (0)
        *ptr++ = 0x90610000;          // stw    %r3, 0(%sp)   (4)
        *ptr++ = 0x38600000 | i<<4;   // li     %r3, n        (8)
        *ptr   = 0x48000000 |         // b xx                 (12)
                    (((uint32_t) &intEntry) - ((uint32_t) ptr)); 
        ptr++;
    }
}

// Disable a little bit of everything..
static void disableInternal()
{
    // Set lowest priority for ever INTC source
    uint8_t *PSRn = (uint8_t*)INTC_PSR;
    for (uint32_t i = 0; i < 330; i++)
        *PSRn++ = 0;

#ifndef BAMMODE

    disableDevices();

/*
    // .. eQADC ints
    uint16_t *eQADCptr = (uint16_t *)0xFFF80060;
    for (int i = 0; i < 6; i++)
        *eQADCptr++ = 0;
*/
    // Kill eDMA ints (if any)
    *(volatile uint32_t *)0xFFF44020 |= *(volatile uint32_t *)0xFFF44020;
    *(volatile uint32_t *)0xFFF44024 |= *(volatile uint32_t *)0xFFF44024;

    if (modeWord != MODE_E78)
    {
        // Pad configuration 4
        if (modeWord == MODE_E39 || modeWord == MODE_E39A)
            SIU_PCR0[ 4 ] = 0x203;

        // e39a is only seen setting this to 1. It's never touched after
        if (modeWord == MODE_E39A)
            SIU_GPDO[ 4 ] = 1;

        // Turn off since it's not used on this platform
        *(volatile uint32_t *)0xFFF9C000 = 0x4000; // DSPI D
    }

    // eSCI
    // Not even present in the firmware from what I could find..
    // *(uint16_t *) 0xFFFB0004 |= 0x8000;
    // *(uint16_t *) 0xFFFB4004 |= 0x8000;
    // *(uint32_t *) 0xFFFB000C = 0;
    // *(uint32_t *) 0xFFFB400C = 0;

#endif

    // Let level 1 - 15 interrupt
    INTC_CPR = 0;
}

// Set up most of the hardware, enable interrupts
static void configure_Internal()
{
    // Set up ASAP!
    // Main loader has not been extracted and is _NOT_ fit to receive external interrupts
    mainReady = 0;

    disableInternal();

    setupIntTable();

    // Force software vectored mode
    INTC_MCR &= ~1;

    // Use new table
    rebaseInterrupt((void*)tablebase);

    // Service watchdog(s) every 8.192 ms @ 128 MHz
    // FP:    25:24 Fixed interval timer period
    // FIE:   23    Fixed interval interrupt ena
    // FPEXT: 16:13 Fixed-interval extension

    uint32_t TCR;

    readSPR(SPR_TCR, TCR);

    TCR &= 0x301E000;

#ifdef BAMMODE
    // Every 2.7 -> ??? ms at ?? MHz (Depends on ECU)
    TCR |= (47&3) << 24 | 1 << 23 | ((47>>2)&15) << 13;
#else
    // Every 8 ms @ 128 MHz
    TCR |= (44&3) << 24 | 1 << 23 | ((44>>2)&15) << 13;
#endif

    writeSPR(SPR_TCR, TCR);

    // Initialise ECC and clear upper portion of SRAM
    initECC_hi();

    // Enable external ints
    enableEE();

#ifdef BAMMODE
    __asm volatile ("wrteei 1");
#endif

}

// Extract the main loader image
static uint32_t mainExtract(const lzImage_t *img, uint8_t *out)
{
    uint32_t chksum = img->checksum;
    uint32_t insize = img->sizeIn;
    uint32_t outpos = 0, inpos = 0;
    const uint8_t *in = (uint8_t*)img;

    for (uint32_t i = 4; i < insize; i++)
        chksum += in[i];

    if (chksum)
    {
        return 2;
    }

    in     += 8;
    insize -= 8;

    while (inpos < insize)
    {
        uint32_t flags = in[inpos++];

        for (uint32_t mask = 0x80; inpos < insize && mask; mask = (mask >> 1))
        {
            if (flags & mask)
            {
                uint32_t length = ((in[inpos] >> 4) & 15) + 3;
                uint32_t ofs    = ((in[inpos]&15)<<8 | in[inpos+1]) + 1;

                inpos += 2;

                while ( length-- )
                {
                    out[outpos] = out[outpos - ofs];
                    outpos++;
                }
            }
            else
            {
                out[outpos++] = in[inpos++];
            }
        }
    }

    return 0;
}

// Entry point of C code
void __attribute__((noreturn)) loaderEntry(const uint32_t loaderBase) 
{
    // Set up hardware
    configure_Internal();

    // Extract main image and jump there
    if (!mainExtract((lzImage_t*)mainloader,(uint8_t *)__s_mainLoader))
        mainloop( &mainReady );

    // Trigger system reset
    SIU_SRCR = 0x80000000;

    while (1)  ;
}

#pragma GCC pop_options
