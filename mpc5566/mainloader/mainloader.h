#ifndef __MAINLOADER_H__
#define __MAINLOADER_H__

#include <stdio.h>
#include <stdint.h>
#include "../../shared/gmlan.h"

///////////////////////////////////////////
///////////////////////////////////////////
// main.c
void     canSend    (const void    *data);
void     canSendFast(const void    *data);
void     sendDebug  (const uint32_t step , const uint32_t extra);
void     sendDebug2 (const void    *ptr);
extern uint32_t processorID;
extern uint32_t clockFreq;

///////////////////////////////////////////
///////////////////////////////////////////
// helper.S
void     waitTimerZero();
uint8_t __attribute__ ((noinline)) ReadData(uint32_t ptr);

#endif
