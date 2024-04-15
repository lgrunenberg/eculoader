#ifndef __ACDELCO_E39_H__
#define __ACDELCO_E39_H__

#include "../../protocol/gmloader.h"

class e39 : public gmloader
{
    void configProtocol();
    bool secAccE39(uint8_t);

public:
    explicit e39();
    ~e39();

    bool dump();
};

#endif