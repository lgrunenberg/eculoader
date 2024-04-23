#ifndef __PRELOADER_H__
#define __PRELOADER_H__

#include <stdio.h>
#include <stdint.h>

// Linker magic; Refer to sections of main loader
extern void mainloop    (uint32_t *const mainReady);
extern void softArbiter (const uint32_t softVec);
extern void sendDebug   (const uint32_t step, const uint32_t extra);

// "pointer" to compressed image of main loader
extern uint8_t mainloader[];

// Keep track of which mode we are in (to trigger external watchdogs)
extern uint32_t modeWord;
enum e_mode {
    MODE_BAM   = 0, // Defunct
    MODE_E39   = 1,
    MODE_E78   = 2,
    // Note - shuffle this?
    MODE_E39A  = 3,
};

// Used by the main loader (specifically gmlan) if the broadcast feature is turned on
// It's only a counter
extern volatile uint32_t msTimer;

// Actual interrupt entry, since we have to perform some magic
void     intEntry();
// The processor has to know where the new table is
void     rebaseInterrupt(void *base);
// Enable interrupts
void     enableEE();
// Ack timer
void     ackTSR();
// Configure timer register
uint32_t readTCR();
void     writeTCR (const uint32_t val);

// Write 0 to the high portion of SRAM to initialise ECC ( and .bss )
void     initECC_hi();

// Disable devices from array
void     disableDevices();

#endif
