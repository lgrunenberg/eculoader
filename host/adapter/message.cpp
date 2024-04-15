#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>
#include <cstring>

#include "message.h"
#include "../tools/tools.h"

using namespace std;
using namespace std::chrono;
using namespace logger;

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




/*
single:
0741    80 41 01 3e4f464600

multi:
0741    81 41 01 234d656e67
0741    00 41 01 2031300000
*/

/*
static void interPretCIM(message_t *msg)
{
    static uint8_t msgOffs[4] = {0};
    static uint8_t message[4][256] = {0x20};
    static uint8_t multi = 0;

    if (msg->message[1] == 0x41)
    {
        if ((msg->message[0] & 0x80) == 0x80)
        {
            multi = msg->message[0] & 0x0f;

            for (uint32_t i = 0; i < 4; i++)
                msgOffs[i] = 0;

            uint8_t line = msg->message[2];

            if (line > 0 && line < 5)
            {
                line--;
                for (uint32_t i = 0; i < 5 && msg->message[3 + i] > 0; i++)
                {
                    message [line] [ msgOffs[line]++ ] = msg->message[3 + i];
                }
            }
            
            if (line == 0)
            {
                // memset(message, 0x20, sizeof(message));
            }
            else if (multi > 0)
                return;

            if (multi == 0)
                for (uint32_t i = 0; i < 4; i++)
                msgOffs[i] = 0;
        }
        else if (multi > 0)
        {
            multi--;
            if (multi == (msg->message[0] & 0x0f))
            {
                uint8_t line = msg->message[2];

                if (line > 0 && line < 5)
                {
                    line--;
                    for (uint32_t i = 0; i < 5; i++)
                    {
                        message [line] [ msgOffs[line]++ ] = msg->message[3 + i];
                    }
                }
                else if (line == 0)
                {
                    // memset(message, 0x20, sizeof(message));
                }

                if (multi != 0)
                    return;

                for (uint32_t i = 0; i < 4; i++)
                    msgOffs[i] = 0;
            }
            else
            {
                for (uint32_t i = 0; i < 4; i++)
                    msgOffs[i] = 0;
                
                // Dropped package
                multi = 0;
                log("Dropped!");
                return;
            }
        }
        else
            return;
    }
    else
    {
        log("Misinterpreted message");
        printMessageContents("Received", msg);
        return;
    }

    for (uint32_t i = 0; i < 4; i++)
        message[i][20] = 0;

    printf("\033[H\033[J");
    printf("\033[0;0H");

    for (uint32_t i = 0; i < 4; i++)
    {
        printf("%s\n\r", &message[i][0]);
    }
}
*/

#include <cstdlib>
#include <chrono>

using namespace timer;
using namespace std;
using namespace msgsys;
using namespace logger;
using namespace checksum;
using namespace std::chrono;



// Push received messages to the queue
void messageReceive(message_t *msg)
{
    msg->timestamp = (uint64_t) (duration_cast< microseconds >( system_clock::now().time_since_epoch()).count() % 1000000);

    // printMessageContents("", msg);

    // if (msg->id == 0x258)
    
    /*if (msg->id == 0x741)
    {
        interPretCIM(msg);
        
    }*/
/*
    if (msg->id != 0x180 &&
        msg->id != 0x380 &&
        msg->id != 0x381 &&
        msg->id != 0x1f5 &&
        msg->id != 0x0c1 &&
        msg->id != 0x3cb &&
        msg->id != 0x385 &&
        msg->id != 0x4d7 &&
        msg->id != 0x0c5)
        */
    // Print error messages
    // if (msg->message[0] < 7 && msg->message[1] == 0x7f && msg->message[3] != 0x78)
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
