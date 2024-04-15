#include <cstdio>
#include <string>
#include <list>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <iostream>

#include "../lowlev/ftdi/ftd2xx.h"
#include "canusb.h"

using namespace std;
using namespace msgsys;
using namespace logger;
// using namespace tools;

#define VENDOR_ID  0x0403
#define PRODUCT_ID 0x6001

// LW17DV0O
// iManufacturer           1 LAWICEL
// iProduct                2 CANUSB
// iSerial                 3 LW17DV0O



std::list <std::string> canusb::findCANUSB() 
{
    struct usb_device *dev;
    struct usb_bus *bus;
    char VID[257] = {0};
    char PID[257] = {0};
    char DSN[257] = {0};
    list <string> adapters;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    for (dev = bus->devices;     dev; dev = dev->next)
    {
        if (dev->descriptor.idVendor  == VENDOR_ID && 
            dev->descriptor.idProduct == PRODUCT_ID )
        {
            usb_dev_handle *handle;

            if (!(handle = usb_open(dev)))
            {
                log("Could not open a device that had id match");
                continue;
            }
            usb_get_string_simple(handle, dev->descriptor.iManufacturer, VID, 256);
            usb_get_string_simple(handle, dev->descriptor.iProduct     , PID, 256);
            usb_get_string_simple(handle, dev->descriptor.iSerialNumber, DSN, 256);
            if (toString(VID) == "LAWICEL" && 
                toString(PID) == "CANUSB"  && 
                toString(DSN).size() > 0    )
            {
                log("Found it!");
                adapters.push_back(toString(DSN));
            }

            usb_close(handle);
        }
    }

    return adapters;
}

void *canusb::m_open(string identifier)
{
    struct usb_device *dev;
    struct usb_bus *bus;
    char VID[257] = {0};
    char PID[257] = {0};
    char DSN[257] = {0};

    for (bus = usb_get_busses(); bus; bus = bus->next)
    for (dev = bus->devices;     dev; dev = dev->next)
    {
        if (dev->descriptor.idVendor  == VENDOR_ID && 
            dev->descriptor.idProduct == PRODUCT_ID )
        {
            usb_dev_handle *handle;

            if (!(handle = usb_open(dev)))
                continue;

            usb_get_string_simple(handle, dev->descriptor.iManufacturer, VID, 256);
            usb_get_string_simple(handle, dev->descriptor.iProduct     , PID, 256);
            usb_get_string_simple(handle, dev->descriptor.iSerialNumber, DSN, 256);
            if (toString(VID) == "LAWICEL" && 
                toString(PID) == "CANUSB"  && 
                toString(DSN) == identifier )
            {
                log("Found device of serial no");
                return handle;
            }

            usb_close(handle);
        }
    }

    return nullptr;
}

canusb::~canusb()
{
    log("Canusb going down");
    this->close();
}

list <string> canusb::adapterList()
{
    // Verbosity is nice and all but this is just ANNOYING
    // usb_set_debug(255);
    usb_init();
    usb_find_busses();
    usb_find_devices();


    return findCANUSB();
}

// CAN Frame
typedef struct {
  unsigned long id;         // Message id
  unsigned long timestamp;  // timestamp in milliseconds
  unsigned char flags;      // [extended_id|1][RTR:1][reserver:6]
  unsigned char len;        // Frame size (0.8)
  unsigned char data[8];    // Databytes 0..7
} CANMsg;

#define SPEED   6  // Speed for interface - See CANUSB 'O' command

bool canusb::send(message_t *msg)
{
    uint32_t retLen;
    char txbuf[80];
    char hex[5];

    sprintf(txbuf, "t%03lX%i", msg->id, msg->length);

    for (uint32_t i = 0; i < msg->length; i++)
    {
        sprintf(hex, "%02X", msg->message[i]);
        strcat(txbuf, hex);
    }

    // Add CR
    strcat(txbuf, "\r");

    // Transmit frame
    return (FT_Write(canusbContext, txbuf, strlen(txbuf), &retLen) == FT_OK) ? true : false;
}

static bool closeChannel(FT_HANDLE ftHandle)
{
    char buf[3] = { 'C', '\r', 0 };
    uint32_t retLen;

    // Close device
    FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);

    return  (FT_Write(ftHandle, buf, 2, &retLen) == FT_OK) ? true : false;
}

bool canusb::close()
{
    // This is to prevent a deadlock
    // Due to how those IDIOTS! wrote the library, there is no way to attach a callback...
    runThread = false;

    if (canusbContext && deviceHandle)
    {
        if (!closeChannel(canusbContext))
        {
            // printf("Failed to close channel\n");
            // return FALSE;
        }
    }

    canusbContext = 0;

    if (messageThreadPtr)
    {
        messageThreadPtr->join();
        messageThreadPtr = 0;
    }

    if (deviceHandle)
    {
        usb_close((usb_dev_handle*)deviceHandle);
        deviceHandle = 0;
    }

    return true;
}

// M00000000 // Acc code
// mFFFFFFFF // acc mask
static bool CalcAcceptanceFilters(FT_HANDLE ftHandle, list <uint32_t> idList)
{
    uint32_t code = ~0;
    uint32_t mask = 0;
    uint32_t retLen;
    char buf[11];

    if (idList.size() == 0)
    {
        code = 0;
        mask = ~0;
    }
    else
    {
        for (uint32_t canID : idList)
        {
            if (canID == 0)
            {
                log("Found illegal id");
                code = 0;
                mask = ~0;
                break;
            }
            code &= (canID & 0x7FF) << 5;
            mask |= (canID & 0x7FF) << 5;
        }
    }

    // code = (code & 0xFF) << 8 | code >> 8;
    // mask = (mask & 0xFF) << 8 | mask >> 8;

    code |= code << 16;
    mask |= mask << 16;

    FT_Purge(ftHandle, FT_PURGE_RX);
    sprintf(buf, "M%08X\r", code);
    if (FT_Write(ftHandle, buf, 10, &retLen) != FT_OK)
        return false;

    FT_Purge(ftHandle, FT_PURGE_RX);
    sprintf(buf, "m%08X\r", mask);
    return  (FT_Write(ftHandle, buf, 10, &retLen) == FT_OK) ? true : false;
}

static bool openChannel(FT_HANDLE ftHandle, int nSpeed)
{
    char buf[4] = { 0x0d, 0x0d, 0x0d, 0};
    uint32_t retLen;
    // list <uint32_t> canIDs = { 0x220, 0x238, 0x240, 0x258, 0x266 };
    list <uint32_t> canIDs = { 0x220, 0x238, 0x240, 0x258, 0x266 };
    // list <uint32_t> canIDs = { 0x740, 0x741 };

    // Clear old commands
    FT_Purge(ftHandle, FT_PURGE_RX);
    if (FT_Write(ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed\n");
        return false;
    }

    // Set CAN baudrate
    FT_Purge(ftHandle, FT_PURGE_RX);
    sprintf(buf, "S%d\r", nSpeed);

    if (FT_Write(ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed\n");
        return false;
    }

    FT_Purge(ftHandle, FT_PURGE_RX);
    sprintf(buf, "Z0\r"); // Fuck off, timestamps

    if (FT_Write(ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed\n");
        return false;
    }


    if (!CalcAcceptanceFilters(ftHandle, canIDs))
    {
        log("Couldn't set can filters");
    }

    // Open device
    FT_Purge(ftHandle, FT_PURGE_RX);
    strcpy( buf, "O\r" );

    return  (FT_Write(ftHandle, buf, 2, &retLen) == FT_OK) ? true : false;
}

static inline void canusbToCanMsg(char *p, message_t *msg)
{
    int val;
    short data_offset;   // Offset to dlc byte
    char save;

    msg->typemask = 0;

    switch(*p)
    {
    case 'r': // Standard remote frame
        msg->typemask |= messageRemote;
    case 't': // Standard frame
        data_offset = 5;
        msg->length = p[4] - '0';
        p[4] = 0;
        break;
    case 'R': // Extended remote frame
        msg->typemask |= messageRemote;
    case 'T': // Extended frame
        msg->typemask |= messageExtended;
        data_offset = 10;
        msg->length = p[9] - '0';
        p[9] = 0;
        break;
    default:
        log("canusbToCanMsg: Big oops!");
        return;
    }

    sscanf(p + 1, "%lx", &msg->id);
    save = *(p + data_offset + 2 * msg->length);
  
    // Fill in data
    if (!(msg->typemask & messageRemote))
    {
        for (uint32_t i = MIN(msg->length, 8); i > 0; i--)
        {
            *(p + data_offset + 2 * (i-1) + 2 )= 0;
            sscanf( p + data_offset + 2 * (i-1), "%x", &val );
            msg->message[i - 1] = val;
        }
    }

    *(p + data_offset + 2 * msg->length) = save;
    msg->timestamp = 0;

    // Not remote frame
    if (!(msg->typemask & messageRemote))
    {
        // If timestamp is active - fetch it
        if (*(p + data_offset + 2 * msg->length) != 0x0d)
        {
            p[ data_offset + 2 * (msg->length) + 4] = 0;
            sscanf((p + data_offset + 2 * (msg->length)), "%x", &val);
            msg->timestamp = val;
        }
    }

    // Remote frame
    else
    {
        if (*(p + data_offset) != 0x0d)
        {
            p[data_offset + 4] = 0;
            sscanf((p + data_offset ), "%x", &val);
            msg->timestamp = val;
        }
    }

    messageReceive(msg);
}

#include <sys/time.h>
// #include <time.h>

// Response to sending a message
// 0x5a/0x7a, 0x0d

void canusb::messageThread(canusb *thisInstance)
{
    char         gbufferRx[4096], msgReceiveBuf[80];
    uint32_t     nRcvCnt, nRxCnt, nTxCnt;
    uint32_t     EventMask, eventStatus;
    EVENT_HANDLE eh;
    int          rc;
    message_t    msg;
    bool         inMessage = false;
    uint32_t     cntMsgRcv = 0;
    uint32_t     getstat;

    pthread_mutex_init(&eh.eMutex  , NULL);
    pthread_cond_init (&eh.eCondVar, NULL);

    EventMask = FT_EVENT_RXCHAR; //  | FT_EVENT_MODEM_STATUS;
    FT_SetEventNotification(thisInstance->canusbContext,
                            EventMask,
                            (PVOID)&eh);

    timeval  timep;
    timespec time = {0};

    // Start fresh
    FT_Purge(thisInstance->canusbContext, FT_PURGE_RX);

    while(thisInstance->runThread)
    {
        eventStatus = 0;
        nRxCnt = 0;
        rc     = 0;

        pthread_mutex_lock(&eh.eMutex);
        while ((getstat = FT_GetStatus(thisInstance->canusbContext, &nRxCnt, &nTxCnt, &eventStatus)) == FT_OK &&
                eventStatus       == 0 && 
                nRxCnt            == 0 &&
                (rc & ~ETIMEDOUT) == 0 &&
                thisInstance->runThread)
        {
            // Hackjob to prevent deadlock while shutting down
            gettimeofday(&timep, NULL);
            time.tv_sec  = timep.tv_sec + 1;
            time.tv_nsec = timep.tv_usec * 1000;
            rc = pthread_cond_timedwait(&eh.eCondVar, &eh.eMutex, &time);
            // rc = pthread_cond_wait(&eh.eCondVar, &eh.eMutex);
        }
        pthread_mutex_unlock(&eh.eMutex);

        if (!thisInstance->runThread) break;
        if ((rc & ~ETIMEDOUT))        log("thread cond: " + to_hex(rc));
        if (getstat != FT_OK)         log("ft_status: " + to_hex(getstat));
        if (eventStatus & ~EventMask) log("evt_status: " + to_hex(eventStatus));

        if (nRxCnt)
        {
            if (nRxCnt > sizeof(gbufferRx))
            {
                log("Overflow!");
                nRxCnt = sizeof(gbufferRx);
            }

            if ((rc = FT_Read(thisInstance->canusbContext, gbufferRx, nRxCnt, &nRcvCnt)) == FT_OK)
            {
                for (uint32_t i = 0; i < nRcvCnt; i++)
                {
                    // Get character
                    char c = gbufferRx[i];

                    // Consecutive data
                    if (inMessage)
                    {
                        msgReceiveBuf[cntMsgRcv++] = c;
        
                        if (c == 0x0d)
                        {
                            canusbToCanMsg(msgReceiveBuf, &msg);
                            inMessage = false;
                        }

                        if (cntMsgRcv > (1 + 3 + 1 + 16 + 1))
                            log("cntMsgRcv >!");
                    }

                    // Start of message
                    else
                    {
                        switch (c)
                        {
                        case 0x07: // Error
                            log("error response");
                            break;
                        case 0x0d: // \n
                            inMessage = false;
                            break;
                        case 'R': // Remote extended frame
                        case 'r': // Remote standard frame
                        case 'T': // Extended frame
                        case 't': // Standard frame
                            inMessage = true;
                            msgReceiveBuf[0] = c;
                            cntMsgRcv = 1;
                            break;
                        case 'Z': // Response to sent extended frame
                        case 'z': // Response to sent standard frame
                            break;
                        default:
                            log("Unknown char: " + to_hex((uint16_t) c, 1));
                            break;
                        }
                    }
                }
            }
            if (rc != 0) log("rc is not 0 1");
        }
    }

    log("Message thread going down");
}

bool canusb::open(std::string identifier)
{
    char dname[32] = {0};

    this->close();

    usb_init();
    usb_find_busses();
    usb_find_devices();

    if ((deviceHandle = this->m_open(identifier)))
    {
        // Is it attached to a driver?
        if (usb_get_driver_np((usb_dev_handle*)deviceHandle, 0, dname, 31) == 0)
        {
            // Yup. This is mine, thank you!
            if (usb_detach_kernel_driver_np((usb_dev_handle*)deviceHandle, 0) != 0)
            {
                // Could not take ownership of the interface for one reason or another.
                this->close();
                return false;
            }
        }

        if (FT_OpenEx((PVOID)identifier.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &canusbContext) == 0)
        {
            // Set latency to 2 ms.
            FT_SetLatencyTimer(canusbContext, 2);

            if (!openChannel(canusbContext, 6))
            {
                this->close();
                return FALSE;
            }

            runThread = true;
            messageThreadPtr = new thread(messageThread, this);
            return true;
        }
        else
        {
            canusbContext = 0;
            this->close();
        }
    }

    return false;
}
