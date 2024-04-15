#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdio>
#include <mutex>
//#include "adapter.h"

namespace msgsys
{
	enum messagetypes
	{
		messageRemote   = 0x40,
		messageExtended = 0x80,
	};

    typedef struct
    {
        uint64_t id;
        uint64_t timestamp;
        uint32_t typemask; // mask of "messagetypes" flags
        uint32_t length;
        uint8_t  message[8]; // Way, way overkill atm.
    } message_t;

    message_t  newMessage(uint64_t id, uint32_t len);
	void       setupWaitMessage(uint64_t id);
	message_t *waitMessage(uint32_t waitms);
    void       queueInit();
    void       messageReceive(message_t *msg);
}

#endif
