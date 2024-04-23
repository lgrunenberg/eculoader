//////////////////////////////
// Flash routines for MPC5566
#include "mainloader.h"
#include "mpc55xx.h"

// Sensitive code. -Os is known from testing to f*ck this up
#pragma GCC push_options
#pragma GCC optimize ("Og")

/*
static const uint32_t mpc5566Partitions[] =  {
    0x00000000, 0x00004000, 0x00010000, 0x0001C000,
    0x00020000, 0x00030000, 0x00040000, 0x00060000,
    0x00080000, 0x000A0000, 0x000C0000, 0x000E0000,
    0x00100000, 0x00120000, 0x00140000, 0x00160000,
    0x00180000, 0x001A0000, 0x001C0000, 0x001E0000,
    0x00200000, 0x00220000, 0x00240000, 0x00260000,
    0x00280000, 0x002A0000, 0x002C0000, 0x002E0000,
    0x00300000, // End
};
*/

// Address   high                         >                         low
//           shadow    high (20 bits)              mid (2 bits)    low (6 bits)
// MASK: 0b  (1)       (1111 1111 1111 1111 1111)  (11)            (11 1111)
// Ret: 1 = Succ
// val > 1 = fail
uint32_t FLASH_Format(const uint32_t mask)
{
#ifdef disableFlash
    return 1;
#else
    volatile uint32_t *FLASH_LMLR = (uint32_t *)&FLASHREGS.LMLR.direct;
    volatile uint32_t *interAddr  = (uint32_t *)0;
    uint32_t status, tries  = 50;
    uint32_t lockmask = ~mask;

    // Shadow partition selected. interlock write must happen there
    if (mask & 0x10000000)
        interAddr = (uint32_t*)0xFFFC00;

    // Unlock access
    FLASH_LMLR[0]  = 0x80000000; // Unlock all partitions. Low / Mid / Shadow
    FLASH_LMLR[1]  = 0x80000000; // Unlock all partitions. High
    // Select partitions
    FLASH_LMLR[3]  = ((mask>>6)&3)<<16|(mask&0x3F); // Mid / Low
    FLASH_LMLR[4]  = (mask>>8)&0xFFFFF;             // High
    // Lock access
    FLASH_LMLR[0] |= ((lockmask>>28)&1)<<20|((lockmask>>6)&3)<<16|(lockmask&0x3F);
    FLASH_LMLR[1] |= (lockmask>>8)&0xFFFFF;

#ifdef enableDebugBox
    sendDebug(1, mask);
#endif

    do
    {
        FLASHREGS.MCR.direct |= FLASH_MCR_ERS_MSK;

        // Erase interlock write
        *interAddr = 0;

        FLASHREGS.MCR.direct |= FLASH_MCR_EHV_MSK;

        // Wait for it to terminate
        while ( !FLASHREGS.MCR.fields.DONE )  ;
        
        asm volatile("sync\n isync\n");

        // Check if PEG is one (ie good)
        status = ( FLASHREGS.MCR.fields.PEG ) ? 1 : 2;

        FLASHREGS.MCR.direct &= ~FLASH_MCR_EHV_MSK;
        FLASHREGS.MCR.direct &= ~FLASH_MCR_ERS_MSK;

    } while (tries-- && status != 1);

    return status;
#endif
}

uint32_t FLASH_Write(const uint32_t addr, const uint32_t len, const void *buffer)
{
#ifdef disableFlash
    return 0;
#else  
    volatile uint32_t *flPtr = (uint32_t*)addr;
    volatile uint32_t *bfPtr = (uint32_t*)buffer;
    uint32_t tempLen = len / 8;
    uint32_t tries = 50;

#ifdef enableDebugBox
    sendDebug(addr, len);
#endif

    // Set PGM
    FLASHREGS.MCR.direct |= FLASH_MCR_PGM_MSK;

    while ( tempLen && tries )
    {
        flPtr[0] = bfPtr[0]; // Interlock-write
        flPtr[1] = bfPtr[1]; // Consecutive write

        FLASHREGS.MCR.direct |= FLASH_MCR_EHV_MSK;

        // Wait for it to terminate
        while ( !FLASHREGS.MCR.fields.DONE )  ;

        if ( FLASHREGS.MCR.fields.PEG )
        {
            tries  = 50;
            flPtr += 2;
            bfPtr += 2;
            tempLen--;
        }
        else
        {
            tries--;
        }

        FLASHREGS.MCR.direct &= ~FLASH_MCR_EHV_MSK;
    }

    FLASHREGS.MCR.direct &= ~FLASH_MCR_PGM_MSK;

    return tries ? 0 : 1;
#endif
}

#pragma GCC pop_options
