#ifndef __COMMON_H__
#define __COMMON_H__

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "stm32f10x.h"

void sleep(const uint16_t ms);

static inline void dwtWait(uint32_t cnt) {
    uint32_t time = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - time) < cnt)  ;
}

#endif
