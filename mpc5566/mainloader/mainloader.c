//////////////////////////////////////////////////////
// This part of the loader handles main tasks:
// External interrupts, FlexCAN, GMLAN.
// Flashing, Dumping, anything accessible by the host
// MD5..
#include "mainloader.h"
#include "mpc55xx.h"
#include "../config.h"

// Os f*cks with or / and during while operations of volatile memory
#pragma GCC push_options
#pragma GCC optimize ("Og")

// Can be found in mainhelper.S
// const uint8_t loaderID[] = { "\eMPC5566-LOADER: TXSUITE.ORG_" };

// Where is read, write and md5 allowed?
const uint32_t flashRange[] = {
    2,                      // Number of sets
    0x00000000, 0x00300000, // Start, End. Main
    0x00FFFC00, 0x01000000, // Start, End. Shadow
};

// Erase, forbidden combinations
const uint32_t maskCombination[] = {
    1,          // Number of sets
    0xE0000000, // These bits must ALWAYS be 0
    // Set 1
    0x10000000, // These..
    0x0FFFFFFF  // ..are not allowed to be set at the same time as these
};

// This one is actually .bss so the preloader must make sure it's cleared (which it does)
// That's further reinforced by the main loader clearing the whole bss area
volatile uint32_t mainReady = 0;

#ifdef enableBroadcast
volatile uint32_t msTimer = 0;

void broadcastMessage()
{
    CAN_A_BOX[ CANBOX_BRC ].CODE |= 0x0C;
}

static void preloadBroadcast()
{
    uint8_t *data = CAN_A_BOX[ CANBOX_BRC ].data;

    data[ 0 ] = 0xFE;
    data[ 1 ] = 0x01;
    data[ 2 ] = 0x3E;

    for (int i = 3; i < 8; i++)
        data[ i ] = 0;
}

#endif

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// External / Software interrupts. Called from preloader

static void service_FlexCAN(const uint32_t what)
{
    if      (what <  3) { } // Errors etc
    else if (what < 19)     // Box 0 - 15
    {
        // Ignore message if crc or frame error is set.
        // Bits are automatically cleared by a read
        if ((CAN_A.ESR.direct & 0x1800) == 0 && what == 3)
            GMLAN_Arbiter( CAN_A_BOX[ what - 3 ].data );

        // Flag reception as captured
        // 31 - 0
        CAN_A.IFRL |= 1 << (what - 3);
    }
}

void softArbiter(const uint32_t softVec)
{
    __asm volatile ("wrteei 1");

    // 152 - 172: FlexCAN
    if (softVec > 151 && softVec < 173)
        service_FlexCAN(softVec - 152);

    __asm volatile ("sync\n wrteei 0");

    INTC_EOIR = 0;
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Main functions
void canSendFast(const void *data)
{
    uint8_t *boxData = CAN_A_BOX[ CANBOX_TX ].data;
    const uint8_t *locData = (uint8_t *)data;

    // Add .3 ms ~ish
    for (uint32_t i = 0; i < 7000; i++)
        __asm("nop");

    for (uint32_t i = 0; i < 8; i++)
        *boxData++ = *locData++;

    CAN_A_BOX[ CANBOX_TX ].CODE |= 0x0C;
}

void canSend(const void *data)
{
    uint8_t *boxData = CAN_A_BOX[ CANBOX_TX ].data;
    const uint8_t *locData = (uint8_t *)data;

    waitTimerZero();

    for (uint32_t i = 0; i < 8; i++)
        *boxData++ = *locData++;

    CAN_A_BOX[ CANBOX_TX ].CODE |= 0x0C;

    writeSPR(SPR_DEC, canInterframe);
}

#ifdef enableDebugBOX
void canSendFast_d(const void *data)
{
    uint32_t *box     = (uint32_t*)(0xFFFC00A0);
    uint8_t  *boxData = (uint8_t *)&box[2];
    uint8_t  *locData = (uint8_t *)data;

    for (uint32_t i = 0; i < 14000; i++)
        __asm volatile("nop");

    for (uint32_t i = 0; i < 8; i++)
        *boxData++ = *locData++;

    *box  |= 0x0C000000;

     for (uint32_t i = 0; i < 14000; i++)
        __asm volatile("nop");
}
void sendDebug(const uint32_t step, const uint32_t extra)
{
    uint32_t data[2] = { step, extra };

    canSendFast_d(data);
}
void sendDebug2(const void *ptr)
{
    canSendFast_d(ptr);
}
#endif

// Read FlexCAN A settings to determine cpu frequency
// We know the target is 500 kbit
static void determineFrequency()
{
#ifdef BAMMODE

    // It's 24 for E39 and 12 for E78
    // Better to be a lil slow on one than too fast on the other
    clockFreq = 24000000;
#else

    uint32_t *MCR   = (uint32_t *)0xFFFC0004;
    uint32_t *SYNCR = (uint32_t *)0xC3F80000;
    // FlexCAN
    uint32_t DIV    = (*MCR >> 24) & 255;
    uint32_t PS1    = (*MCR >> 19) &   7;
    uint32_t PS2    = (*MCR >> 16) &   7;
    uint32_t prop   =  *MCR        &   7;
    // PLL
    uint32_t PREDIV = (*SYNCR >> 28) &  7;
    uint32_t MFD    = (*SYNCR >> 23) & 31;
    uint32_t RFD    = (*SYNCR >> 19) &  7;
    RFD = 1 << RFD; // 1, 2, 4, 8..

    clockFreq = (DIV + 1) * (prop + PS1 + PS2 + 4) * 500000;

    // Set to external crystal, we have to do some more crap..
    // Crystal mode / Ext ref mode: fsys = xtal * ((mfd + 4) / ((prediv + 1) * 2^rfd))
    if (!(*MCR & 0x2000))
        clockFreq = clockFreq * ((MFD + 4) / ((PREDIV + 1) * RFD));
#endif
}

static void configureFlash()
{
    // Disable prefetch, set up delays etc
    *(volatile uint32_t*)&FLASHREGS.BIUCR.direct = (
        // Sane values according to the datasheet
        2 << 13 | // APC
        1 << 11 | // WWSC
        3 <<  8   // RWSC
    );

    // Lock out eDMA, EBI, FEC
    *(volatile uint32_t*)&FLASHREGS.BIUAPR.direct = 0xF;

    volatile uint32_t *FLASH_LMLR = &FLASHREGS.LMLR.direct;

    // Low partitions
    if (!(*FLASH_LMLR & 0x80000000)) // Unlock access
        *FLASH_LMLR = 0xA1A11111;
    *FLASH_LMLR = 0x801FFFFF; // Lock

    // High partitions
    if (!(FLASH_LMLR[1] & 0x80000000)) // Unlock access
        FLASH_LMLR[1] = 0xB2B22222;
    FLASH_LMLR[1] = 0x8FFFFFFF; // Lock

    // Disable secondary lock
    if (!(FLASH_LMLR[2] & 0x80000000)) // Unlock access
        FLASH_LMLR[2] = 0xC3C33333;
    FLASH_LMLR[2] = 0x80000000;

    // Select 0 partitions
    FLASH_LMLR[3] = 0;
    FLASH_LMLR[4] = 0;
}

static uint32_t configureFlexCAN()
{
    volatile uint32_t *CANa_MCR = (uint32_t *)&CAN_A.MCR.direct;
             uint32_t *CANa_MB0 = (uint32_t *)CAN_A_BOX;
             uint32_t *tmp      = CANa_MB0;

#ifdef BAMMODE
    // Enable CANRX / TX
    uint16_t *PCR83 = (uint16_t *)0xC3F900E6;
    PCR83[0] = 0x613;
    PCR83[1] = 0x413;

    *CANa_MCR &= ~0x80000000;
#endif

    *CANa_MCR |=  0x5000<<16; // Freeze enable / halt

    // Wait for it to hit the brakes
    while (!(*CANa_MCR & 0x01000000)) ;

    *CANa_MCR &= ~0x21003F;    // no buffers, disable MBFEN and warnings

#if (defined(enableDebugBOX) && defined(enableBroadcast))
    *CANa_MCR |=  0x020000 | 3; // No stupid self-reception and use 4 buffers
#elif (defined(enableDebugBOX) || defined(enableBroadcast))
    *CANa_MCR |=  0x020000 | 2; // No stupid self-reception and use 3 buffers
#else
    *CANa_MCR |=  0x020000 | 1; // No stupid self-reception and use 2 buffers
#endif

    // Make sure bus off is automatically recovered + more
    CANa_MCR[1] &= 0xFFFF2097;

    // Disable all boxes
    for (uint32_t i = 0; i < 64; i++) {
        *tmp++ &= 0xF080FFFF; // Disable
        *tmp++  = 0;          // Set id to 0
         tmp   += 2;
    }

    CANa_MB0[0] |= 0x04080000; // (0) Receive
    CANa_MB0[1]  = hostID << 18;
    CANa_MB0[4] |= 0x08080000; // (1) Send
    CANa_MB0[5]  = localID << 18;

#if (defined(enableDebugBOX) && defined(enableBroadcast))
    CANa_MB0[8] |= 0x08080000;  // (2) Send
    CANa_MB0[9]  = debugID << 18;
    CANa_MB0[12] |= 0x08080000; // (3) Send
    CANa_MB0[13]  = broadcastID << 18;
#elif defined(enableDebugBOX)
    CANa_MB0[8] |= 0x08080000;  // (2) Send
    CANa_MB0[9]  = debugID << 18;
#elif defined(enableBroadcast)
    CANa_MB0[8] |= 0x08080000; // (2) Send
    CANa_MB0[9]  = broadcastID << 18;
    preloadBroadcast();
#endif

    // Set mask to exact match
    *(uint32_t *) 0xFFFC0010 = ~0; // Global mask
    *(uint32_t *) 0xFFFC0014 = ~0; // Box 14 mask
    *(uint32_t *) 0xFFFC0018 = ~0; // Box 15 mask
    *(uint32_t *) 0xFFFC001C =  0; // Clear counter
    *(uint32_t *) 0xFFFC002C = ~0; // Clear interrupt flags
    *(uint32_t *) 0xFFFC0030 = ~0; // Clear interrupt flags
    *(uint32_t *) 0xFFFC0028 =  1; // Enable interrupts for receive

    // Set priority
    uint8_t *PSRn = (uint8_t *)0xFFF480D8;
    PSRn[3] = 14; // Box 0 priority

    *CANa_MCR &= ~0x10000000; // Negate halt
    
    // Read register to clear flags
    return *(volatile uint32_t*)0xFFFC0020;
}

static void main_Init()
{
    // Hardware interrupts are still active. No need to throw TWO of them
    *(volatile uint8_t *)0xFFF40043 = 0; // !EFNCR[0](FLASH ECC), !ERNCR[1](SRAM ECC)
    *(volatile uint8_t *)0xFFF48049 = 0; // 9 ECC, interrupt level

#ifdef BAMMODE
    // Negate STOP bit in flash_mcr
    *(volatile uint32_t*)0xC3F88000 &= ~0x40;
#endif

    // The linker script is already offset-ing the address so there's no need to subtract one from the target
    asm volatile (
        " li      %%r3,       bss_size@l \n"
        " mtctr   %%r3                   \n"
        " lis     %%r3,       bss_start@h\n"
        " ori     %%r3, %%r3, bss_start@l\n"
        " li      %%r4, 0                \n"
        " clearBSSL:                     \n"
        " stbu    %%r4, 1(%%r3)          \n"
        " bdnz clearBSSL                 \n"
        : : : "r3","r4");

    determineFrequency();

    processorID = SIU_MIDR;

    GMLAN_Reset();

    writeSPR(SPR_DEC, 100);

    configureFlexCAN();
    configureTBL();
    configureFlash();
}

void mainloop()
{
    main_Init();
    mainReady = 1;

    GMLAN_MainLoop();

    writeSPR(SPR_DEC, 24000000);

    waitTimerZero();
}

#pragma GCC pop_options