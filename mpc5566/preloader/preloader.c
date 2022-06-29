//////////////////////////////////////////////////////
// This part of the loader handles house keeping tasks:
// Configure most of the hardware except FlexCAN.
// Help out with interrupts, recover from ECC exceptions etc.
// Extract the main image.
#include "preloader.h"

// Os f*cks with or / and during while operations of volatile memory
#pragma GCC push_options
#pragma GCC optimize ("O1")

#define tablebase 0x40000000
#define mainbase  0x4000C000

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

#ifdef enableDebugBOX
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
    volatile uint32_t *FLASH_MCR = (uint32_t *)0xC3F88000;

    switch (hardVec)
    {

    /// Data storage exceptions
    // This fucker can be thrown by many internal devices (unfortunately)
    // It's up to the code to figure out who's actually shouting and why.
    // Implement SRAM ECC handler?
    case 0x30:

        // FLASH ECC error
        if (*FLASH_MCR & 0x8000) {
            *FLASH_MCR |= 0x8000;
            return 1;
        }

        // Read while write error
        else if (*FLASH_MCR & 0x4000)
            *FLASH_MCR |= 0x4000;

#ifdef enableDebugBOX
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
            volatile uint8_t *gpd4 = (uint8_t *)0xC3F90604;
            *gpd4 = 0;
            for (uint32_t i = 0; i < 32; i++)
                __asm volatile ("nop");
            *gpd4 = 1;
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
#ifdef enableDebugBOX
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
    uint32_t *ptr = (uint32_t *)tablebase;

    // Create a stupidly large table
    for (uint32_t i = 0; i < 256; i++) // 35
    {
        uint32_t branch = (((uint32_t) &intEntry) - ((uint32_t) ptr)) - 12;
        *ptr++ = 0x3821FFFC;          // addi   %sp, %sp, -4  (0)
        *ptr++ = 0x90610000;          // stw    %r3, 0(%sp)   (4)
        *ptr++ = 0x38600000 | i<<4;   // li     %r3, n        (8)
        *ptr++ = 0x48000000 | branch; // b xx                 (12)
    }
}

// Disable a little bit of everything..
static void disable_Internal()
{
    // No interrupts for you!
    uint8_t *PSRn = (uint8_t *)0xFFF48040;
    for (uint32_t i = 0; i < 330; i++)
        *PSRn++ = 0;

#ifndef BAMMODE
    // FlexCAN
//  *(uint32_t *)0xFFFC0000 = 0x80000000; // A
    *(uint32_t *)0xFFFC4000 = 0x80000000; // B
    *(uint32_t *)0xFFFC8000 = 0x80000000; // C
    *(uint32_t *)0xFFFCC000 = 0x80000000; // D

    // Disable CAN A interrupts
    *(uint32_t *)0xFFFC0024 = 0;
    *(uint32_t *)0xFFFC0028 = 0;

    // eTPU A and B (Must be taken down before eMIOS)
    *(volatile uint32_t *)0xC3FC0014 |= 0x40000000;
    *(volatile uint32_t *)0xC3FC0018 |= 0x40000000;

    // eDMA ints
    *(uint32_t *)0xFFF44008 = 0;
    *(uint32_t *)0xFFF4400C = 0;
    *(uint32_t *)0xFFF44010 = 0;
    *(uint32_t *)0xFFF44014 = 0;

    *(volatile uint32_t *)0xFFF44020 |= *(volatile uint32_t *)0xFFF44020;
    *(volatile uint32_t *)0xFFF44024 |= *(volatile uint32_t *)0xFFF44024;
    // *(volatile uint32_t *)0xFFF44020 = 0; // High (it is available. Bad DS)
    // *(volatile uint32_t *)0xFFF44024 = 0; // Low request

    *(uint32_t *)0xFFF90030 = 0; // DSPI ints
    *(uint32_t *)0xFFF94030 = 0;
    *(uint32_t *)0xFFF98030 = 0;
    *(uint32_t *)0xFFF9C030 = 0;

    *(uint32_t *)0xC3F90018 = 0; // SIU_DIRER (DMA Int)
    *(uint32_t *)0xC3F90024 = 0; // SIU_ORER
    *(uint32_t *)0xC3F90028 = 0; // SIU_IREER
    *(uint32_t *)0xC3F9002C = 0; // SIU_IFEER

    // eQADC ints
    *(uint32_t *) 0xFFF80000 = 0; // No debug, no SSI
    uint16_t *eQADCptr = (uint16_t *)0xFFF80060;
    for (uint32_t i = 0; i < 6; i++)
        *eQADCptr++ = 0;

    // DSPI
    *(volatile uint32_t *) 0xFFF90000 |= 0x4000; // A
    *(volatile uint32_t *) 0xFFF94000 |= 0x4000; // B
    *(volatile uint32_t *) 0xFFF98000 |= 0x4000; // C
    if (modeWord != MODE_E78)
        *(volatile uint32_t *) 0xFFF9C000 |= 0x4000; // D

    // eSCI
    // Not even present in the firmware from what I could find..
    // *(uint16_t *) 0xFFFB0004 |= 0x8000;
    // *(uint16_t *) 0xFFFB4004 |= 0x8000;
    // *(uint32_t *) 0xFFFB000C = 0;
    // *(uint32_t *) 0xFFFB400C = 0;
    if (modeWord == MODE_E39)
        *(uint16_t *)0xC3F90048 = 0x203;

    // Disable eMIOS module
    *(volatile uint32_t *)0xC3FA0000 |= 0x40000000;

#endif

    // Let level 1 - 15 interrupt
    *(volatile uint32_t*)0xFFF48008 =  0;
}

// Set up most of the hardware, enable interrupts
static void configure_Internal()
{
    // Set up ASAP!
    // Main loader has not been extracted and is _NOT_ fit to receive external interrupts
    mainReady = 0;
    disable_Internal();
    setupIntTable();

    // Force software vectored mode
    *(volatile uint32_t *)0xFFF48000 &= ~1;

    // Use new table
    rebaseInterrupt(tablebase);

    // Service watchdog(s) every 8.192 ms @ 128 MHz
    // FP:    25:24 Fixed interval timer period
    // FIE:   23    Fixed interval interrupt ena
    // FPEXT: 16:13 Fixed-interval extension

    uint32_t TCR;
    asm volatile (
        "mfspr  %0, 340"
        : "=r" (TCR) );
    TCR &= 0x301E000;

#ifdef BAMMODE
    // Every 2.7 -> ??? ms at ?? MHz (Depends on ECU)
    TCR |= (47&3) << 24 | 1 << 23 | ((47>>2)&15) << 13;
#else
    // Every 8 ms @ 128 MHz
    TCR |= (44&3) << 24 | 1 << 23 | ((44>>2)&15) << 13;
#endif

    asm volatile (
        "mtspr  340, %0"
        : : "r" (TCR) );

    enableEE();

#ifdef BAMMODE
    __asm volatile ("wrteei 1");
#endif

}

// Extract the main loader image
static uint32_t mainExtract(const uint8_t *in, uint8_t *out)
{
    uint32_t chksum = *(uint32_t*)&in[0];
    uint32_t insize = *(uint32_t*)&in[4];
    uint32_t outpos = 0, inpos = 0;

    if (insize < (( 4 * 1024) + 8) || // The image should never be smaller than 4K
        insize > ((14 * 1024) + 8)  ) // The image should never be bigger than 16K
        return 1;

    for (uint32_t i = 4; i < insize; i++)
        chksum += in[i];

    if (chksum)
        return 2;

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
                for (uint32_t i = 0; i < length; i++)
                {
                    out[outpos] = out[outpos - ofs];
                    outpos++;
                }
            }
            else
                out[outpos++] = in[inpos++];
        }
    }

    return 0;
}

// Entry point of C code
void __attribute__((noreturn)) loaderEntry() 
{
    // Set up hardware
    configure_Internal();

    // Extract main image and jump there
    if (!mainExtract(mainloader,(uint8_t *)mainbase))
        mainloop();

    // Trigger system reset
    *(volatile uint32_t*)0xC3F90010 = 0x80000000;
    while (1)  ;
}

#pragma GCC pop_options
