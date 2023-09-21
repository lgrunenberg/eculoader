#ifndef __MCP_H__
#define __MCP_H__

#include "stdint.h"

/*
MCP flash map

BE00 - E9FF: Software
EA00 - EBFF: Calibration
EC00 - FDFF: Boot


Main 16k
BE00 - FDFF

14 bytes (yes, really)
FFB0 - FFBD

14 bytes (yes, really)
FFC2 - FFCF

48 bytes, vectors
FFD0 - FFFF


Protection range is a weird one

11 [ xxxx xxxx ] 00 0000
   [   FLBPR   ] << 6

BE00 - BFFF is always protected unless FLBPR is 0xff



*/



// mcp_trgbin.s
extern uint8_t mcp_loader[];
extern const uint32_t mcp_loaderSize;

extern uint8_t mcp_bin[];
extern const uint32_t mcp_binSize;






// mcp.c


// mcp_commands.c
void csumFrame(uint8_t *snd);


uint32_t sendSPIFrame(const uint8_t *snd, uint8_t *rec, const int nBytes, const uint8_t pFrame);

void bootSecretStep(uint8_t *snd);
void enterBoot(void);
void enterSecretBoot(void);
void sauceReadAddress(const uint32_t address, const uint8_t count);
void jumpRam(const uint32_t address); // 4
void readGMString(void);              // 5
void exitLoader(void);                // 6
void setFlashProt(const uint8_t msk); // 7


uint8_t waitExtra;

// mcp_loader.c










#endif // __MCP_H__
