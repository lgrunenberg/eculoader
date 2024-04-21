// PowerPC utils

#ifndef __POWERPC_H__
#define __POWERPC_H__

#define writeSPR(spr, dat) \
    asm volatile ( "mtspr    %0, %1\n" :: "i" (spr), "r" (dat) )

#define readSPR(spr, dat) \
    asm volatile ( "mfspr    %0, %1\n" : "=r" (dat) : "i" (spr) )

#endif
