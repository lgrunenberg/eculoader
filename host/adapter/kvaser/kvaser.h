#ifndef KVASER_H
#define KVASER_H

#include <thread> 
#include "../adapter.h"
#include "../../tools/tools.h"

class kvaser : public adapter_t
{
public:
    explicit kvaser();
    ~kvaser();
    std::list <std::string> adapterList();
    bool open(channelData);
    bool close();
    bool send(msgsys::message_t*);
private:
    bool CalcAcceptanceFilters(std::list<uint32_t>) ;
    void messageThread(kvaser *);
    bool m_open(channelData, int);
    int kvaserHandle = -1;
};

#endif
