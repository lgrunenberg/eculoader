#include <stdint.h>
#include "../../shared/gmlan.h"

#define localID     0x7e8
#define hostID      0x7e0
#define broadcastID 0x101
#define debugID     0x111

///////////////////////////////////////////
///////////////////////////////////////////
// main.c
void     canSend    (const void    *data);
void     canSendFast(const void    *data);
void     sendDebug  (const uint32_t step , const uint32_t extra);
void     sendDebug2 (const void    *ptr);
uint32_t processorID;
uint32_t clockFreq;

///////////////////////////////////////////
///////////////////////////////////////////
// helper.S
void     writeMAS0(const uint32_t data);
uint32_t readMAS1();
uint32_t readMAS2();
uint32_t readMAS3();
void     configureTBL();
// uint32_t readTimer();
void     waitTimerZero();
void     setTimer       (const uint32_t val);
uint8_t __attribute__ ((noinline)) ReadData(uint32_t ptr);
