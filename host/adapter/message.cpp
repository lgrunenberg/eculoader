#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>
#include <cstring>

#include "message.h"
#include "../tools/tools.h"

#include <cstdlib>
#include <chrono>

using namespace timer;
using namespace msgsys;
using namespace logger;
using namespace checksum;
using namespace std;
using namespace std::chrono;

namespace msgsys
{

// Must be power of two!
#define qCOUNT 1024

static mutex               msgMutex;    // Lock access while pushing or popping the queue
static volatile uint32_t   inPos   = 0; // Positions in queue. 0 to qCOUNT - 1
static volatile uint32_t   outPos  = 0;
static volatile uint32_t   inQueue = 0; // How many queued messages. 0 to qCOUNT
static message_t           messageQueue[qCOUNT];
static message_t           tempMessage;
static volatile uint64_t   WaitMessageID = ~0;
static message_t           nullMessage;

static inline void printMessageContents(string what, message_t *msg)
{
    string strResp = what + ": ";

    strResp += to_hex((uint16_t)msg->id, 2) + " "; 
    for (uint32_t i = 0; i < msg->length; i++)
        strResp += to_hex((uint16_t)msg->message[i], 1);

    log("recd: " + strResp + "   (" + to_string_fcount(msg->timestamp, 6) + ")");
}

static message_t *queuePop()
{
    msgMutex.lock();

    if (inQueue)
    {
        memcpy(&tempMessage, &messageQueue[outPos++], sizeof(message_t));
        outPos &= (qCOUNT-1);
        inQueue--;

        msgMutex.unlock();
        return &tempMessage;
    }

    // ~no message available~
    msgMutex.unlock();
    return 0;
}

message_t newMessage(uint64_t id, uint32_t len)
{
    message_t msg = {0};
    msg.id = id;
    msg.length = len;

    // memset(&msg.message, 0, len);

    return msg;
}

// adapter::open is calling this just before a channel is opened
void queueInit()
{
    msgMutex.lock();

    inPos   = 0;
    outPos  = 0;
    inQueue = 0;

    msgMutex.unlock();
}

void setupWaitMessage(uint64_t id)
{
    msgMutex.lock();

    // Clear all old messages
    inPos   = 0;
    outPos  = 0;
    inQueue = 0;

	WaitMessageID = id;

    msgMutex.unlock();
}

// This thing is a resource hog!!
// Complete notifiers...
message_t *waitMessage(uint32_t waitms)
{
	auto oldTime = system_clock::now();
	auto newTime = oldTime;
	message_t *msg;

	do {
		if ((msg = queuePop()) != nullptr && msg->id == WaitMessageID)
				return msg;
		newTime = system_clock::now();
	} while (duration_cast<milliseconds>(newTime - oldTime).count() < waitms);

	memset(&nullMessage, 0, sizeof(message_t));
	return &nullMessage;
}

// Push received messages to the queue
void messageReceive(message_t *msg)
{
    // msg->timestamp = (uint64_t) (duration_cast< microseconds >(system_clock::now().time_since_epoch()).count() % 1000000);
    // printMessageContents("", msg);

    msgMutex.lock();

    message_t *qmsg = &messageQueue[inPos++];
	inPos &= (qCOUNT-1);

    qmsg->id        = msg->id;
    qmsg->timestamp = 0; // Implement something useful here..
    qmsg->typemask  = msg->typemask;
    qmsg->length    = msg->length;
    memcpy(qmsg->message, msg->message, msg->length);

    // Still room to spare
    if (inQueue < qCOUNT)
    {
        inQueue++;
    }

    // Buffer is full; The oldest message pointed to by outPos has been overwritten.
    // Update pointer to the next oldest message
    else
    {
        outPos++;
        outPos &= (qCOUNT-1);
    }

    msgMutex.unlock();
}

};
