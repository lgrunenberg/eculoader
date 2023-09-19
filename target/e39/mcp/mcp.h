#ifndef __MCP_H__
#define __MCP_H__

#include "stdint.h"

// mcp_loader.s
extern uint8_t mcp_loader[];
extern const uint32_t mcp_loaderSize;

// mcp.c




// mcp_commands.c
uint32_t sendSPIFrame(const uint8_t *snd, uint8_t *rec, const int nBytes);

void bootSecretStep(uint8_t *snd);
void enterBoot(void);
void enterSecretBoot(void);
void sauceReadAddress(const uint32_t address, const uint8_t count);
void jumpRam(const uint32_t address);
void readGMString(void);


#endif // __MCP_H__
