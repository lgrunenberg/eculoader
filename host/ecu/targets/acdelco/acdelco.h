#ifndef __ACDELCO_E39_H__
#define __ACDELCO_E39_H__

#include "../../protocol/gmloader.h"

class e39 : public gmloader
{
    void configProtocol();
    bool secAccE39(uint8_t);

    bool initSessionE39();
    bool initSessionE39A();
    bool play();

    bool getSecBytes(uint32_t secLockAddr, uint32_t progModeAddr);

public:
    explicit e39();
    ~e39();


    bool bamFlash(uint32_t address);

    bool dump();
};

#endif