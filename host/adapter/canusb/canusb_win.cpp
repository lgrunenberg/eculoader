#include <cstdio>
#include <string>
#include <list>

#include <iostream>
#include <algorithm>

#include "canusb.h"

// This whole file is a cluster fudge...

using namespace std;
using namespace msgsys;
using namespace logger;

extern "C"
{

    static HMODULE                    m_hmodule        = nullptr; // dll library pointer

    // Generate and fetch list of adapters connected to the system
	typedef FT_STATUS (*PtrToCreateDeviceList)(LPDWORD);
	static PtrToCreateDeviceList m_pCreateDeviceList = nullptr;
	typedef FT_STATUS (*PtrToDeviceInfoList)(FT_DEVICE_LIST_INFO_NODE*, LPDWORD);
	static PtrToDeviceInfoList m_pDeviceInfoList = nullptr;


	typedef FT_STATUS (*PtrtoFtPurge)(FT_HANDLE, ULONG);
	static PtrtoFtPurge m_pFtPurge = nullptr;

	typedef FT_STATUS (*PtrToFtWrite)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
	static PtrToFtWrite m_pFtWrite = nullptr;

	typedef FT_STATUS (*PtrToFtRead)(FT_HANDLE,LPVOID,DWORD,LPDWORD);
	static PtrToFtRead m_pFtRead = nullptr;



	typedef FT_STATUS (*PtrToFtGetStatus)(FT_HANDLE,DWORD*,DWORD*,DWORD *);
	static PtrToFtGetStatus m_pFtGetStatus = nullptr;

	typedef FT_STATUS (*PtrToFtSetEventNotif)(FT_HANDLE,DWORD,PVOID);
	static PtrToFtSetEventNotif m_pFtSetEventNotif = nullptr;

	typedef FT_STATUS (*PtrToFtSetLatencyTimer)(FT_HANDLE,UCHAR);
	static PtrToFtSetLatencyTimer m_pFtSetLatencyTimer = nullptr;



	typedef FT_STATUS (*PtrToFtOpenEx)(PVOID,DWORD, FT_HANDLE*);
	static PtrToFtOpenEx m_pFtOpenEx = nullptr;

	typedef FT_STATUS (*PtrToFtClose)(FT_HANDLE);
	static PtrToFtClose m_pFtClose = nullptr;



	typedef FT_STATUS (*PtrToResetDevice)(FT_HANDLE);
	static PtrToResetDevice m_pResetDevice = nullptr;

	typedef FT_STATUS (*PtrToSetBaud)(FT_HANDLE,ULONG);
	static PtrToSetBaud m_pSetBaud = nullptr;
}

bool canusb::loadLibrary()
{
    log("Load library");
    if ((m_hmodule = LoadLibrary("ftd2xx64.dll")) == nullptr)
    {
        log("Could not load ftd2xx.dll");
        return false;
    }

    m_pCreateDeviceList = (PtrToCreateDeviceList)GetProcAddress(m_hmodule, "FT_CreateDeviceInfoList");
    if (!m_pCreateDeviceList)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pDeviceInfoList = (PtrToDeviceInfoList)GetProcAddress(m_hmodule, "FT_GetDeviceInfoList");
    if (!m_pDeviceInfoList)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtPurge = (PtrtoFtPurge)GetProcAddress(m_hmodule, "FT_Purge");
    if (!m_pFtPurge)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtWrite = (PtrToFtWrite)GetProcAddress(m_hmodule, "FT_Write");
    if (!m_pFtWrite)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtClose = (PtrToFtClose)GetProcAddress(m_hmodule, "FT_Close");
    if (!m_pFtClose)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtRead = (PtrToFtRead)GetProcAddress(m_hmodule, "FT_Read");
    if (!m_pFtRead)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtGetStatus = (PtrToFtGetStatus)GetProcAddress(m_hmodule, "FT_GetStatus");
    if (!m_pFtGetStatus)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtSetEventNotif = (PtrToFtSetEventNotif)GetProcAddress(m_hmodule, "FT_SetEventNotification");
    if (!m_pFtSetEventNotif)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtOpenEx = (PtrToFtOpenEx)GetProcAddress(m_hmodule, "FT_OpenEx");
    if (!m_pFtOpenEx)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pFtSetLatencyTimer = (PtrToFtSetLatencyTimer)GetProcAddress(m_hmodule, "FT_SetLatencyTimer");
    if (!m_pFtSetLatencyTimer)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    m_pResetDevice = (PtrToResetDevice)GetProcAddress(m_hmodule, "FT_ResetDevice");
    if (!m_pResetDevice)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }



    m_pSetBaud = (PtrToSetBaud)GetProcAddress(m_hmodule, "FT_SetBaudRate");
    if (!m_pSetBaud)
    {
        log("Could not locate instance of function in ftd dll");
        unloadLibrary();
        return false;
    }

    return true;
}

bool canusb::unloadLibrary()
{

    if (m_hmodule != nullptr)
    {
        FreeLibrary((HMODULE)m_hmodule);
        m_hmodule = nullptr;
    }

    // m_pListDevices = nullptr;
    m_pCreateDeviceList = nullptr;
    m_pDeviceInfoList = nullptr;

    m_pFtPurge = nullptr;
    m_pFtWrite = nullptr;

    m_pFtClose = nullptr;

    m_pFtRead = nullptr;

    m_pFtGetStatus = nullptr;

    m_pFtSetEventNotif = nullptr;

    m_pFtOpenEx = nullptr;
    m_pFtSetLatencyTimer = nullptr;

    m_pResetDevice = nullptr;


    m_pSetBaud = nullptr;

    log("Unload dll");
    return true;
}

std::list<std::string> canusb::findCANUSB()
{
    list<string> adapters;
    DWORD numDevices;

    if (m_hmodule == nullptr)
    {
        return adapters;
    }

    // adapters.push_back("LW17DV0O");

    if (m_pCreateDeviceList(&numDevices) != FT_OK)
    {
        log("Could not generate list of ftdi devices");
        return adapters;
    }

    FT_DEVICE_LIST_INFO_NODE *devInfo = (FT_DEVICE_LIST_INFO_NODE *)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevices);

    if (numDevices == 0)
    {
        log("No ftdi devices connected to the system");
        return adapters;
    }

    if (m_pDeviceInfoList(devInfo, &numDevices) != FT_OK)
    {
        log("Could not retrieve list of ftdi devices");
        return adapters;
    }

    for (DWORD i = 0; i < numDevices; i++)
    {
        if (toString(devInfo[i].Description) == "CANUSB")
        {
            adapters.push_back(toString(devInfo[i].SerialNumber));
        }
    }

    free(devInfo);

    return adapters;
}

canusb::canusb()
{
    loadLibrary();
}

canusb::~canusb()
{
    this->close();

    unloadLibrary();
}

// Unused
void *canusb::m_open(string identifier)
{
    return nullptr;
}

static inline void canusbToCanMsg(char *p, message_t *msg)
{
    int val;
    short data_offset; // Offset to dlc byte
    char save;

    msg->typemask = 0;

    switch (*p)
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

    sscanf(p + 1, "%llx", &msg->id);
    save = *(p + data_offset + 2 * msg->length);

    // Fill in data
    if (!(msg->typemask & messageRemote))
    {
        // MIN(msg->length, 8)
        uint32_t ln = msg->length;
        if (ln > 8) ln = 8;
        for (uint32_t i = ln; i > 0; i--)
        {
            *(p + data_offset + 2 * (i - 1) + 2) = 0;
            sscanf(p + data_offset + 2 * (i - 1), "%x", &val);
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
            p[data_offset + 2 * (msg->length) + 4] = 0;
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
            sscanf((p + data_offset), "%x", &val);
            msg->timestamp = val;
        }
    }

    messageReceive(msg);
}

void canusb::messageThread(canusb *thisInstance)
{
    char gbufferRx[4096], msgReceiveBuf[80];
    DWORD eventStatus, nRcvCnt, nRxCnt, nTxCnt;
    message_t msg;
    bool inMessage = false;
    uint32_t cntMsgRcv = 0;

    HANDLE hEvent = CreateEvent(NULL,false,false,"");

    m_pFtSetEventNotif(thisInstance->canusbContext, FT_EVENT_RXCHAR, hEvent);

    // Start fresh
    m_pFtPurge((FT_HANDLE)thisInstance->canusbContext, FT_PURGE_RX);

    while (thisInstance->runThread)
    {
        WaitForSingleObject(hEvent, 500);

        uint32_t getstat = m_pFtGetStatus((FT_HANDLE)thisInstance->canusbContext, &nRxCnt, &nTxCnt, &eventStatus);
        if (getstat != FT_OK && thisInstance->runThread)
        {
            log("ft_status: " + to_hex(getstat));
        }

        if (nRxCnt)
        {
            if (nRxCnt > sizeof(gbufferRx))
            {
                log("Overflow!");
                nRxCnt = sizeof(gbufferRx);
            }

            if (m_pFtRead((FT_HANDLE)thisInstance->canusbContext, gbufferRx, nRxCnt, &nRcvCnt) == FT_OK)
            {
                for (uint32_t i = 0; i < nRcvCnt; i++)
                {
                    // Get character
                    char c = gbufferRx[i];

                    // Consecutive data
                    if (inMessage)
                    {
                        msgReceiveBuf[cntMsgRcv++] = c;

                        if (c == '\r')
                        {
                            canusbToCanMsg(msgReceiveBuf, &msg);
                            inMessage = false;
                        }

/*
                        if (cntMsgRcv > (1 + 3 + 1 + 16 + 1))
                        {
                            log("cntMsgRcv >! " + to_string((uint32_t)(cntMsgRcv-(1 + 3 + 1 + 16 + 1))));
                        }*/
                    }

                    // Start of message
                    else
                    {
                        switch (c)
                        {
                        case '\a': // Error
                            log("error response");
                            break;
                        case '\r':
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
                            log("Unknown char: " + to_hex((uint16_t)c, 1));
                            break;
                        }
                    }
                }
            }
            else
            {
                log("FT_Read failed");
            }
        }
    }

    log("Message thread going down");
}

bool canusb::closeChannel()
{
    char buf[3] = {'C', '\r', 0};
    FT_STATUS status = FT_OK;
    
    // log("Adapter close");
    if (canusbContext != nullptr)
    {
        // Close device
        m_pFtPurge((FT_HANDLE)canusbContext, FT_PURGE_RX | FT_PURGE_TX);

        DWORD retLen;
        m_pFtWrite((FT_HANDLE)canusbContext, buf, 2, &retLen);

        status = m_pFtClose((FT_HANDLE)canusbContext);
    }

    if (status == FT_OK)
    {
        return true;
    }

    log("Could not close transport to adapter");
    return false;
}

bool canusb::CalcAcceptanceFilters(FT_HANDLE ftHandle, list<uint32_t> idList)
{
    uint32_t code = ~0;
    uint32_t mask = 0;
    DWORD retLen;
    char buf[16];

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

    code |= code << 16;
    mask |= mask << 16;

    sprintf(buf, "M%08X\r", code);
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    if (m_pFtWrite((FT_HANDLE)ftHandle, buf, 10, &retLen) != FT_OK)
    {
        return false;
    }

    sprintf(buf, "m%08X\r", mask);
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    return (m_pFtWrite((FT_HANDLE)ftHandle, buf, 10, &retLen) == FT_OK) ? true : false;
}

bool canusb::openChannel(FT_HANDLE ftHandle, int nSpeed)
{
    char buf[16] = {0x0d, 0x0d, 0x0d, 0};
    DWORD retLen;

    // FT_STATUS stat = FT_OK;
    // list <uint32_t> canIDs = { 0x220, 0x238, 0x240, 0x258, 0x266 };
    // list<uint32_t> canIDs = { 0x220, 0x238, 0x23c, 0x240, 0x258, 0x25c, 0x266 };
    list<uint32_t> canIDs;
    // list <uint32_t> canIDs = { 0x740, 0x741 };
    // list <uint32_t> canIDs = {0};

    // Clear old commands
    m_pFtPurge(ftHandle, FT_PURGE_RX);
    if (m_pFtWrite((FT_HANDLE)ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed");
        return false;
    }

/*
        public const string CAN_BAUD_BTR_200K = "0x01:0x2f"; // 200 Kbit/s. SP: 85%, sjw: 1
        // public const string CAN_BAUD_BTR_300K = "0x42:0x15"; // 300 Kbit/s BAM
        public const string CAN_BAUD_BTR_400K = "0x00:0x2f"; // 400 Kbit/s. SP: 85%, sjw: 1
*/

/*
    // Set CAN baudrate
    sprintf(buf, "s%04X\r", 0x002f);
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    if (m_pFtWrite((FT_HANDLE)ftHandle, buf, 6, &retLen) != FT_OK)
    {
        log("Write failed");
        return false;
    }
*/


    // Set CAN baudrate
    sprintf(buf, "S%d\r", nSpeed);
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    if (m_pFtWrite((FT_HANDLE)ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed");
        return false;
    }



    sprintf(buf, "Z0\r"); // Fuck off, timestamps
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    if (m_pFtWrite((FT_HANDLE)ftHandle, buf, 3, &retLen) != FT_OK)
    {
        log("Write failed");
        return false;
    }

    if (!CalcAcceptanceFilters(ftHandle, canIDs))
    {
        log("Couldn't set can filters");
    }

    // Open device
    strcpy( buf, "O\r" );
    m_pFtPurge((FT_HANDLE)ftHandle, FT_PURGE_RX);
    return  (m_pFtWrite((FT_HANDLE)ftHandle, buf, 2, &retLen) == FT_OK) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public methods
list<string> canusb::adapterList()
{
    return findCANUSB();
}

bool canusb::send(message_t *msg)
{
    DWORD retLen;
    char txbuf[80];
    char hex[5];

    sprintf(txbuf, "t%03X%i", (uint32_t)msg->id, msg->length);

    for (uint32_t i = 0; i < msg->length; i++)
    {
        sprintf(hex, "%02X", msg->message[i]);
        strcat(txbuf, hex);
    }

    // Add CR
    strcat(txbuf, "\r");

    // Transmit frame
    return (m_pFtWrite(canusbContext, txbuf, strlen(txbuf), &retLen) == FT_OK) ? true : false;
}

bool canusb::close()
{
    runThread = false;

    if (!closeChannel())
    {
        log("Could not close channel");
    }

    if (messageThreadPtr)
    {
        messageThreadPtr->join();
        messageThreadPtr = 0;
    }

    canusbContext = nullptr;
    return true;
}

bool canusb::open(channelData device)
{
    string identifier = device.name;

    this->close();
    DWORD retLen;

    log("OpenEx");

    canusbContext = 0;

    if (m_pFtOpenEx((PVOID)identifier.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &canusbContext) == FT_OK)
    {
        if (canusbContext == nullptr)
        {
            log("m_pFtOpenEx nullptr");
            return false;
        }

        char buf[3] = {'C', '\r', 0};
        m_pFtPurge((FT_HANDLE)canusbContext, FT_PURGE_RX | FT_PURGE_TX);
        m_pFtWrite((FT_HANDLE)canusbContext, buf, 2, &retLen);

        if (m_pResetDevice(canusbContext) != FT_OK)
        {
            log("Could not reset device");
            return false;
        }

        // Set latency to 2 ms.
        if (m_pFtSetLatencyTimer(canusbContext, 2) != FT_OK)
        {
            log("Could not set latency to two ms");
            return false;
        }

        if (m_pSetBaud(canusbContext, FT_BAUD_921600) != FT_OK)
        {
            log("Could not baud");
        }

        if (!openChannel(canusbContext, 6))
        {
            log("Open channel fail");
            this->close();
            return FALSE;
        }

        log("Channel open");
        runThread = true;
        messageThreadPtr = new thread(messageThread, this);

        return true;
    }
    else
    {
        log(".. failed");
        canusbContext = 0;
        this->close();
    }

    return false;
}
