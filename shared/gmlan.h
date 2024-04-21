#include <stdio.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Must be implemented on the loader side
extern void     canSend     (const void    *data); // Consecutive frames
extern void     canSendFast (const void    *data); // One-shot
extern uint32_t FLASH_Format(const uint32_t mask);
extern uint32_t FLASH_Write (const uint32_t addr, const uint32_t len, const void *buffer);
extern void    *hashMD5     (const uint32_t addr, const uint32_t len);

// In Hz
extern uint32_t clockFreq;

// data[0] = length
// data[++] = string
extern const uint8_t loaderID[];

// Read, md5, write.. has to be within these limits
// [0]: Number of sets
// [1]: Start
// [2]: End
// [..]: Start
// [..]: End
extern const uint32_t flashRange[];

// Do not allow these combinations while erasing
// [0]: Number of sets
// [1]: Bits that are not allowed no matter what
// [2]: These partition bits
// [3]: Can not be set at the same time as any of these
// [..] ..
// [..] ..
extern const uint32_t maskCombination[];

extern uint32_t processorID;

#ifdef enableBroadcast
extern volatile uint32_t msTimer;
extern void broadcastMessage();
#endif

extern uint32_t canInterframe;

void GMLAN_Reset();
void GMLAN_MainLoop();
void GMLAN_Arbiter(const void *dataptr);
