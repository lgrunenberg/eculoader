/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// TODO
//
// TransferLzData: Reject further messages after everything has been written?
// It won't write more but a message to indicate as such would be nice.
//
// RequestDownload: ???
// It's kinda useless since compression really need its own message format..?

// Don't give a shit what the project settings are.
// Os and O2 are both known to make things.. weird
#pragma GCC push_options
#pragma GCC optimize ("Og")

#include "gmlan.h"

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Constants, target specific etc

// Due to ECC, data can not be written however you please
#ifdef mpc5566
#define boundaryMask (7)
#else
#error "no such target"
#endif

// mpc5566, what can I say?
// It'll crap out if you read an address that has not been correctly written
#if defined(safereads) || defined(mpc5566)
extern uint8_t __attribute__ ((noinline)) ReadData(uint32_t ptr);
#else
#define ReadData(d) \
    *(volatile uint8_t*)(d)
#endif

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Global parameters

uint32_t canInterframe;

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Static parameters

// What's up doc?
static volatile uint32_t wrkMask = 0;
// So, should I keep doing this?
static volatile uint32_t keepAlive = 1;

// Perform operation on this address
static volatile uint32_t opAddress = 0;
// lz is out of order so it's safer to give it its own address
static volatile uint32_t lzAddress = 0;
// For this many bytes
static volatile uint32_t opLength = 0;
// Only used for format. Mask of partitions to be erased
static volatile uint32_t opMask = 0;

// MD5
// Notice behaviour; You can request routine results several times and get the same response
// Care must be taken when starting the service to make sure the correct result is received later on
// 0 = has no hash stored, !0 has hash
static volatile uint32_t md5Has = 0;
static volatile uint8_t *md5Hash = 0; // Generated hash

// Format
// 0 = idle, I have no results to give you
// 1 = Last erase went well. Notice md5 caveat
// 2 = Erase failed. This will reset to 0 after the response has been sent
static volatile uint32_t formatHas = 0; // Step, same as above

// ByID has to know what it's responding to
static volatile uint8_t  respToID = 0;

// 0 / 1: Ignore GoAhead from host
// 2: First response has been sent, transition to 1 when a GoAhead is received and start sending remaining packages
static volatile uint32_t waitGoAhead = 0;

// Frame to be sent consists of this many messages / Remaining messages
static volatile uint32_t respMsgs = 0;
// Gotta store the response somewhere; slightly oversized just to stay safe
// (36 * 8) + 8 from the first one = 296 + 1 runt stepper (function is out of order)
static uint8_t  respData[300];

// Global buffers
static uint8_t lzRecChBuf[256]; // LZ. Received data, ring buffer
static uint8_t lzExBuf   [256]; // LZ. Extracted data, ring buffer
static uint8_t tempBuf   [256] __attribute__((aligned(4))); // Flash, md5 etc
static uint32_t *tempZero = (uint32_t*)&tempBuf[0]; // Alignment STFU-helper

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Enums
enum e_GMREQ {
    ReadDataByIdentifier      = 0x1A,
    ReturnToNormalMode        = 0x20,
    ReadMemoryByAddress       = 0x23,
    GoAhead                   = 0x30,
    StartRoutineByIdentifier  = 0x31, // This one is borrowed from KWP. It's just too good a request not to be used!
    RequestRoutineResult      = 0x33, // Ditto
    TransferData              = 0x36,
    TransferLzData            = 0x37, // I've tried not to make up weird custom requests but this one really needs it...
    WriteDataByIdentifier     = 0x3B,
};

enum e_GMFAULTS {
    SubFuncNotSupInvForm      = 0x12,
    BusyRepeatRequest         = 0x21, // Borrowed from KWP
    condNotCorrReqSeqErr      = 0x22,
    RequestOutOfRange         = 0x31,
    GeneralProgrammingFailure = 0x85,
};

enum e_operations {
    DO_READ      = 0x00000001,
    DO_RETURNID  = 0x00000002,
    DO_HASHMD5   = 0x00000004,
    DO_FORMAT    = 0x00000008,
    DO_WRITE     = 0x00000010,
    DO_LZWRITE   = 0x00000020,
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// LZ madness

typedef struct {
    // Extractor context. Saved and restored as need be
    uint8_t  flag;    // Current flag / mode
    uint8_t  mask;    // Current mask in that mode
    uint32_t winPos;  // Ringbuffer position, private window
    uint32_t inWin;   // If extraction was paused inside a window, this will indicate how many more bytes
    uint32_t indOfs;

    uint32_t inCnt;   // Current input count
    uint32_t inLen;   // How many bytes received thus far?
    uint32_t outCnt;  // Current output count
    uint32_t outLen;  // Expected, total, output length

    uint32_t exCnt;   // Number of bytes in extraction buffer.

    // Shared ring buffer "pointers". (will wrap)
    uint8_t  recWrTo; // How far along the receiver has filled the buffer.
    uint8_t  extRdTo; // How far along the extractor has gotten reading the compressed data.
    uint8_t  extWrTo; // How far along the extractor has filled the buffer.
    uint8_t  wrRdTo;  // How far the write function has read from.
} lzParams_t;

static lzParams_t lzParams;
static uint8_t lzExpStep = 0;

static void lzResetParam(const uint32_t outLen)
{
//  lzParams.flag    = 0; // No need, extractor resets when mask is 0
    lzParams.mask    = 0;
    lzParams.winPos  = 0;
    lzParams.inWin   = 0;
//  lzParams.indOfs  = 0; // No need, extractor resets when inWin is 0

    lzParams.inCnt   = 0;
    lzParams.inLen   = 0;
    lzParams.outCnt  = 0;
    lzParams.outLen  = outLen;

    lzParams.exCnt   = 0;

    // Ring buffer counters
    lzParams.recWrTo = 0;
    lzParams.extRdTo = 0;
    lzParams.extWrTo = 0;
    lzParams.wrRdTo  = 0;
}

static void lzExtract(uint8_t *buf)
{
    uint8_t  m_flag    = lzParams.flag;
    uint8_t  m_mask    = lzParams.mask;
    uint32_t m_winPos  = lzParams.winPos;
    uint32_t m_inWin   = lzParams.inWin;
    uint32_t m_indOfs  = lzParams.indOfs;

    uint32_t m_inCnt   = lzParams.inCnt;
    uint32_t m_inLen   = lzParams.inLen;
    uint32_t m_outCnt  = lzParams.outCnt;
    uint32_t m_outLen  = lzParams.outLen;

    uint8_t  m_extRdTo = lzParams.extRdTo;
    uint8_t  m_extWrTo = lzParams.extWrTo;
    uint32_t m_exCnt   = lzParams.exCnt;

    static uint8_t winBuf[4096];

    while ((m_inCnt < m_inLen) && (m_outCnt < m_outLen) && (m_exCnt < 256))
    {
        // Start of chunk and can read at least one byte
        if (!m_mask)
        {
            m_flag = buf[m_extRdTo++];
            m_mask = 0x80;
            m_inCnt++; // Total count in
        }

        for ( ; (m_outCnt < m_outLen) && (m_mask) && (m_exCnt < 256); m_mask >>= 1)
        {
            // Have a flag and at least 2 more bytes can be read
            if ((m_flag & m_mask) && (m_inCnt + 1 < m_inLen))
            {
                if (!m_inWin)
                {
                    m_inWin  = ((buf[m_extRdTo] >> 4) & 15) + 3;
                    m_indOfs = (m_winPos-(((buf[m_extRdTo]&15)<<8|buf[((m_extRdTo+1)&0xFF)])+1))&0xFFF;
                }

                while ((m_exCnt < 256) && (m_inWin) && (m_outCnt < m_outLen))
                {
                    lzExBuf[m_extWrTo++] = winBuf[m_indOfs  ];
                    winBuf [m_winPos++ ] = winBuf[m_indOfs++];
                    m_winPos &= 0xFFF;
                    m_indOfs &= 0xFFF;
                    m_outCnt++; // Total count out
                    m_exCnt++;  // Count in ringbuffer out
                    m_inWin--;
                }

                // Abort right here since m_mask must be left alone
                if (m_inWin)
                    goto backupState;

                m_extRdTo += 2;
                m_inCnt += 2;
            }

            // Have NO flag and at least one more byte can be read
            else if ((!(m_flag & m_mask)) && (m_inCnt < m_inLen))
            {
                uint8_t tmp = buf[m_extRdTo++];
                lzExBuf[m_extWrTo++] = tmp;
                winBuf [m_winPos++ ] = tmp;
                m_winPos  &= 0xFFF;
                m_inCnt++;  // Total count in
                m_outCnt++; // Total count out
                m_exCnt++;  // Count in ringbuffer out
            }

            // Wrap up, can not read more bytes
            else
                goto backupState;
        }
    }

backupState:

    lzParams.flag    = m_flag;
    lzParams.mask    = m_mask;
    lzParams.winPos  = m_winPos;
    lzParams.inWin   = m_inWin;   // If it had to abort inside a window, this stores number of bytes left
    lzParams.indOfs  = m_indOfs;

    lzParams.inCnt   = m_inCnt;   // Total count in
    lzParams.outCnt  = m_outCnt;  // Total count out

    lzParams.extRdTo = m_extRdTo; // Read to
    lzParams.extWrTo = m_extWrTo; // Written to
    lzParams.exCnt   = m_exCnt;   // Number of extracted bytes in the ringbuffer
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Shortcuts

// 03 7f req reason
static void GMLAN_SendFail(const uint8_t req, const uint8_t reason)
{
    uint8_t tmp[4] = { 0x03, 0x7F, req, reason };
    canSendFast(tmp);
}

// 30 00 00 ..
static void GMLAN_SendGoAhead()
{
    uint32_t tmp[1] = { GoAhead << 24 };
    canSendFast(tmp);
}

static void GMLAN_PackageResponse(void *dataPtr)
{
    uint8_t *rptr = respData;
    uint8_t *ptr  = dataPtr;
    uint32_t Len = *ptr + 1;
    uint32_t Stp = 0x21;

    // Single frame
    if (Len <= 8)
    {
        respMsgs = 1;

        for (uint32_t i = 0; i < Len; i++)
            *rptr++ = *ptr++;
    }
    else
    {
        respMsgs = 0;
        *rptr++ = 0x10;

        do
        {
            uint8_t toCopy = (Len > 7) ? 7 : Len;
            Len -= toCopy;
            respMsgs++;

            for (uint32_t i = 0; i < toCopy; i++)
                *rptr++ = *ptr++;

            *rptr++ = Stp++;
            Stp    &= 0x2F;
        } while (Len);
    }
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Called from main

static void GMLAN_RespMech()
{
    // No response to give. Return
    if (!respMsgs || waitGoAhead  == 2)
        return;
    // Keep sending on previous reply
    else if (waitGoAhead == 1)
    {
        uint8_t *rptr = respData;
        do {
            rptr+=8;
            canSend(rptr);
        } while (--respMsgs);
        waitGoAhead = 0;
    }
    else
    {
        respMsgs--;
        canSendFast(respData);
        if (respMsgs)
            waitGoAhead = 2;
    }
}

static void GMLAN_RdDataById()
{
    uint8_t resp[opLength + 3];
    resp[0] = opLength + 2;
    resp[1] = ReadDataByIdentifier + 0x40;
    resp[2] = respToID;

    for (uint32_t i = 0; i < opLength; i++)
        resp[i + 3] = ReadData(opAddress++);

    GMLAN_PackageResponse(resp);
}

static uint32_t scanFF(uint32_t ptr, uint32_t len)
{
    do {
        if (ReadData(ptr++) != 0xFF)
            return 0;
    } while (--len);

    return 1;
}

static void GMLAN_RdMemByAddr()
{
    uint8_t resp[opLength + 8];
    resp[1] = ReadMemoryByAddress + 0x40;

    // FF. Skip
    if (scanFF(opAddress, opLength))
    {
        resp[0] = 5;
        resp[2] = 0xFF;
        resp[3] = 0xFF;
        resp[4] = 0xFF;
        resp[5] = 0xFF;
    }
    else
    {
        uint16_t CSUM16 = 0;
        uint8_t *rptr = (uint8_t*)&resp[6];

        resp[0] = opLength + 7; // Take header and checksum into account
        resp[2] = (opAddress >> 24);
        resp[3] = (opAddress >> 16);
        resp[4] = (opAddress >>  8);
        resp[5] = (opAddress);

        for (uint32_t i = 0; i < opLength; i++)
        {
            uint8_t tdat = ReadData(opAddress++);
            CSUM16 += tdat;
            *rptr++ = tdat;
        }

        *rptr++ = (CSUM16>>8)&0xFF;
        *rptr++ = CSUM16&0xFF;
    }

    GMLAN_PackageResponse(resp);
}

static void GMLAN_HashMD5()
{
    uint8_t resp[3];

    resp[0] = 2;
    resp[1] = StartRoutineByIdentifier + 0x40;
    resp[2] = 0;

    canSendFast(resp);

    md5Hash = hashMD5(opAddress, opLength);
    md5Has = 1;
}

static void GMLAN_Format()
{
    uint8_t resp[3];

    resp[0] = 2;
    resp[1] = StartRoutineByIdentifier + 0x40;
    resp[2] = 1;

    canSendFast(resp);

    formatHas = FLASH_Format(opMask);
}

static void GMLAN_TransferData()
{
    if (FLASH_Write(opAddress, opLength, tempBuf))
    {
        GMLAN_SendFail(TransferData, GeneralProgrammingFailure);
    }
    else
    {
        uint8_t resp[2] = { 1, TransferData + 0x40 };
        canSendFast(resp);
    }
}

static void GMLAN_TransferLzData()
{
    uint32_t len, status = 0;

    do
    {
        lzExtract(lzRecChBuf);

#ifdef boundaryMask
        len = lzParams.exCnt & ~boundaryMask;
#else
        len = lzParams.exCnt;
#endif

        if (len)
        {
            for (uint32_t i = 0; i < len; i++)
                tempBuf[i] = lzExBuf[lzParams.wrRdTo++];

            status = FLASH_Write(lzAddress, len, tempBuf);
            lzParams.exCnt -= len; // Subtract written bytes from extraction counter
            lzAddress      += len; // Increment address
        }

    } while (!status && len);

    if (status)
    {
        lzExpStep = 0;
        GMLAN_SendFail(TransferLzData, GeneralProgrammingFailure);
    }
    else
    {
        uint8_t resp[2] = { 1, TransferLzData + 0x40 };
        canSendFast(resp);
    }
}

void GMLAN_Reset()
{
    canInterframe = 1200 * (clockFreq / 1000000);
}

void GMLAN_MainLoop()
{
#ifdef enableBroadcast
    uint32_t oldTime = msTimer;
#endif

    while(keepAlive)
    {
#ifdef enableBroadcast
        if ((msTimer - oldTime) >= 2000)
        {
            oldTime = msTimer;
            broadcastMessage();
        }
#endif

        if (wrkMask)
        {
            // Commence operation
            switch (wrkMask)
            {
                case DO_READ:
                    GMLAN_RdMemByAddr();
                    break;
                case DO_RETURNID:
                    GMLAN_RdDataById();
                    break;
                case DO_HASHMD5:
                    GMLAN_HashMD5();
                    break;
                case DO_FORMAT:
                    GMLAN_Format();
                    break;
                case DO_WRITE:
                    GMLAN_TransferData();
                    break;
                case DO_LZWRITE:
                    GMLAN_TransferLzData();
                    break;
                default:
                    break;
            }

            // Make sure first response, if any, is sent before releasing control
            GMLAN_RespMech();

            // Work done. Indicate as such to release control
            wrkMask = 0;
        }

        // Keep sending response if a GoAhead has been received. If not, return to mainloop
        GMLAN_RespMech();
    }
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// Inside interrupt
// Remember: watchdog(s), tick tick..
// Do _NOT_ read flash from in here!!

// Check if requested address + length is within range
static uint32_t acceptedRegion(const uint32_t Address, const uint32_t Length)
{
    const uint32_t *ptr = flashRange;
    const uint32_t sets = *ptr++;

    // Catch overflow / 0-len
    if ((Address + Length) < Address || !Length)
        return 0;
#ifdef boundaryMask
    else if ((Length & boundaryMask) || (Address & boundaryMask))
        return 0;
#endif

    for (uint32_t i = 0; i < sets; i++)
    {
        if (Address >= ptr[0] && (Address + Length) <= ptr[1])
            return 1;
        ptr+=2;
    }

    return 0;
}

static uint32_t acceptedMask(const uint32_t mask)
{
    const uint32_t *ptr   = maskCombination;
    const uint32_t sets   = *ptr++;
    const uint32_t forbid = *ptr++;

    if ((mask & forbid) || !mask)
        return 0;

    for (uint32_t i = 0; i < sets; i++)
    {
        if ((mask & ptr[0]) && (mask & ptr[1]))
            return 0;
        ptr+=2;
    }

    return 1;
}

// 02 1A xx
static void REQ_RdDataByID(const uint8_t *ptr)
{
    if (*ptr != 2)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    else
    {
        respToID = ptr[2];
        wrkMask = DO_RETURNID;

        switch (ptr[2])
        {
            // Return loader ID
            case 0x90:
                opLength  = loaderID[0];
                opAddress = (uint32_t)&loaderID[1];
                break;
            // Inter-frame delay
            case 0x91:
                *tempZero = (canInterframe) ?
                    (canInterframe / (clockFreq / 1000000)) : 0;
                opLength  = 4;
                opAddress = (uint32_t)tempZero;
                break;
            // Processor ID
            case 0x92:
                *tempZero = processorID;
                opLength  = 4;
                opAddress = (uint32_t)tempZero;
                break;
            default:
                GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
                wrkMask = 0;
                break;
        }
    }
}

static void REQ_WrDataByID(const uint8_t *ptr)
{
    if (*ptr < 3)
    {
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
        return;
    }

    switch (ptr[2])
    {
        // Inter-frame delay (04 3B 91 nn nn)
        case 0x91:
            if (*ptr != 4)
            {
                GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
                return;
            }

            canInterframe = (ptr[3] << 8 | ptr[4]) * (clockFreq / 1000000);
            uint32_t tmp[1] = { 0x027B9100 };
            canSendFast(tmp);
            return;

        default:
            GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
            return;
    }
}

// 07 23 AA AA AA AA LL LL
static void REQ_RdMemByAddr(const uint8_t *ptr)
{
    uint32_t tmpAddr = ptr[2] << 24 | ptr[3] << 16 | ptr[4] << 8 | ptr[5];
    uint32_t tmpLen  = ptr[6] <<  8 | ptr[7];

    // We expect package to be 7 bytes long
    if (*ptr != 7)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    // 7 bytes are reserved for header and embedded checksum-16
    else if (tmpLen > (255 - 7)/* || !acceptedRegion(tmpAddr, tmpLen)*/)
        GMLAN_SendFail(ptr[1], RequestOutOfRange);
    else
    {
        opLength  = tmpLen;
        opAddress = tmpAddr;
        wrkMask = DO_READ;
    }
}

// 0a 31 service XX XX XX XX YY YY YY YY
static void REQ_StrtRoutById(const uint8_t *ptr)
{
    uint32_t param1 = ptr[3] << 24 | ptr[4] << 16 | ptr[5] << 8 | ptr[6];
    uint32_t param2 = ptr[7] << 24 | ptr[8] << 16 | ptr[9] << 8 | ptr[10];

    // Check length and if the SUB is supported
    if (*ptr != 10 || ptr[2] > 1)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    // It's okay to send the same request again as long as you request the same thing that is currently running
    else if ( (ptr[2] == 0 && (wrkMask & ~DO_HASHMD5)) || // Trying md5 while busy with something else
              (ptr[2] == 1 && (wrkMask & ~DO_FORMAT ))  ) // Trying erase while busy with something else
        GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
    else
    {
        // MD5
        if (ptr[2] == 0)
        {
            if (!acceptedRegion(param1, param2))
                GMLAN_SendFail(ptr[1], RequestOutOfRange);
            else if ((wrkMask & DO_HASHMD5) && (opAddress != param1 || opLength != param2))
                GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
            // else if (wrkMask & DO_HASHMD5)  ; // Already working on it, send an OK?
            // Problem scenario; Host missed reply, loader just completed the task but have yet to update the internal flags
            // This code sets the flag only for the main code to clear it once the exception has ended.
            // -No operation is started and no reply is given
            else
            {
                md5Has = 0;
                opAddress = param1;
                opLength = param2;
                wrkMask = DO_HASHMD5;
            }
        }

        // Erase
        else if (ptr[2] == 1 && param1 == ~param2)
        {
            if (!acceptedMask(param1))
                GMLAN_SendFail(ptr[1], RequestOutOfRange);
            else if ((wrkMask & DO_FORMAT) && opMask != param1)
                GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
            // else if (wrkMask & DO_FORMAT)  ; // Already working on it, send an OK?
            // Read above
            else
            {
                formatHas = 0;
                opMask = param1;
                wrkMask = DO_FORMAT;
            }
        }
        else
            GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    }
}

// 02 33 service
static void REQ_ReqRoutResById(const uint8_t *ptr)
{
    // Check length and if the SUB is supported
    if (*ptr != 2 || ptr[2] > 1)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    // Do not poll us while working on something else
    else if ( (ptr[2] == 0 && (wrkMask & ~DO_HASHMD5)) ||
              (ptr[2] == 1 && (wrkMask & ~DO_FORMAT ))  )
        GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
    else
    {
        if (ptr[2] == 0)
        {
            if (wrkMask & DO_HASHMD5)
                GMLAN_SendFail(ptr[1], BusyRepeatRequest);
            else if (md5Has)
            {
                uint8_t resp[19];
                resp[0] = 18;
                resp[1] = RequestRoutineResult + 0x40;
                resp[2] = 0;
                for (uint32_t i = 0; i < 16; i++)
                    resp[i+3] = md5Hash[i];
                GMLAN_PackageResponse(resp);
            }
            else
                GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
        }
        else if (ptr[2] == 1)
        {
            if (wrkMask & DO_FORMAT)
                GMLAN_SendFail(ptr[1], BusyRepeatRequest);
            else if (formatHas == 2)
            {
                GMLAN_SendFail(RequestRoutineResult, GeneralProgrammingFailure);
                formatHas = 0;
            }
            else if (formatHas == 1)
            {
                uint8_t resp[3] = { 2, RequestRoutineResult + 0x40, 1 };
                canSendFast(resp);
            }
            else
                GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
        }
    }
}

// xx 36 00 AA AA AA AA .. .. ..
static void REQ_TransferData(const uint8_t *ptr)
{
    uint32_t tmpAddr = ptr[3] << 24 | ptr[4] << 16 | ptr[5] << 8 | ptr[6];
    uint32_t len = *ptr - 6;
        
    if (*ptr < 7)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    else if (!acceptedRegion(tmpAddr, len))
        GMLAN_SendFail(ptr[1], RequestOutOfRange);
    else
    {
        for (uint32_t i = 0; i < len; i++)
            tempBuf[i] = ptr[i + 7];

        opAddress = tmpAddr;
        opLength  = len;
        wrkMask = DO_WRITE;
    }
}

// AA: 32-bit address, big endian
// EE: Extracted length. Must be at least 1, max whole flash
// DD: Data
// CS: Checksum-16 of the whole frame except REQ and PCI (include SUB)

// First frame: (PCI must be AT LEAST 14)
// xx [37] [00] [AA AA AA AA] [EE EE EE EE] DD DD .. [CS CS]

// Consecutive frames: (PCI must be at least 5)
// xx [37] [01] DD .. [CS CS]
// xx [37] [02] DD .. [CS CS]
// ..
// xx [37] [FF] DD .. [CS CS]
// xx [37] [01] DD .. [CS CS]
static void REQ_TransferLzData(const uint8_t *ptr)
{
    uint32_t frameLen = ptr[0];
    uint32_t frameStp = ptr[2];

    if ((frameLen <  5 &&  frameStp) || // Consecutive frame is too short
        (frameLen < 14 && !frameStp)  ) // First frame is too short
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    else
    {
        // Checksum the thing
        uint32_t rSum = ptr[frameLen - 1] << 8 | ptr[frameLen];
        for (uint8_t i = 0; i < (frameLen - 3); i++)
            rSum -= ptr[i + 2];
        if (rSum)
        {
            GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
            return;
        }

        // Consecutive frame
        // xx [37] [!00] DD .. [CS CS]
        if (frameStp)
        {
            if (frameStp != lzExpStep)
            {
                GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
                lzExpStep = 0;
                return;
            }

            lzExpStep++;
            if (!lzExpStep)
                lzExpStep = 1;
            ptr      += 3; // Do not store header
            frameLen -= 4; // Exclude header and checksum-16
        }

        // First frame
        // xx [37] [00] [AA AA AA AA] [CC CC] [EE EE] DD DD .. [CS CS]
        else
        {
                     lzAddress = ptr[3] << 24 | ptr[4] << 16 | ptr[5] << 8 | ptr[ 6];
            uint32_t lzExtLen  = ptr[7] << 24 | ptr[8] << 16 | ptr[9] << 8 | ptr[10];

            // Maximum window size is 4096 so we only really care about minimum length for the compressed data
            // It can easily handle anything you throw at it in terms of maximum length since it extracts on-the-fly
            if (!acceptedRegion(lzAddress, lzExtLen)) { // Check hard limits of flash and length
                GMLAN_SendFail(ptr[1], RequestOutOfRange);
                lzExpStep = 0;
                return;
            }

            lzResetParam(lzExtLen);
            ptr      += 11;  // Do not store header
            frameLen -= 12;  // Exclude header and checksum-16
            lzExpStep = 1;   // If more frames are to be received, expect this step to be present in the received frame
        }

        for (uint32_t i = 0; i < frameLen; i++)
            lzRecChBuf[lzParams.recWrTo++] = *ptr++;

        lzParams.inLen += frameLen;
        wrkMask  = DO_LZWRITE;
    }
}

static void REQ_ReturnToNormal(const uint8_t *ptr)
{
    if (*ptr != 1)
        GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
    else
    {
        uint16_t tmp[1] = { 0x0160 };
        canSendFast(tmp);
        keepAlive = 0;
    }
}

// Hack to help out with frames of size exactly 30
static uint32_t notGoAhead = 0;
static void *REQ_RecFrame(const uint8_t *ptr)
{
    static uint32_t stepper, multiFrame = 0;
    static uint8_t *bufPtr , reqBy[512];

    // Recover start of frame
    if (*ptr < 0x20 || *ptr > 0x2F)
        multiFrame = 0;

    if (multiFrame)
    {
        multiFrame--;
        if (*ptr++ == stepper++)
        {
            stepper &= 0x2f;
            for (uint32_t i = 0; i < 7; i++)
                *bufPtr++ = *ptr++;

            // Last frame
            if (!multiFrame)
                return (void *)&reqBy[0];
        }
        else
            multiFrame = 0;
    }
    // I have no idea what you are sending me!?
    else if (*ptr  > 7    && // More than 7 bytes
             *ptr != 0x10 && // No indication of it being a multi-frame message
             *ptr != 0x30  ) // This is not a "GoAhead"
        return 0;
    else
    {
        bufPtr = (uint8_t*)&reqBy[0];
        uint8_t reqMax  = 7;
        uint8_t framLen = 0;
        notGoAhead = 0;

        // Extended frame
        if (*ptr == 0x10)
        {
            framLen = *++ptr;
            reqMax--;
        }
        else if (*ptr != 0x30)
            framLen = *ptr;

        if (framLen > reqMax)
        {
            notGoAhead = 1;

            // How many extra frames?
            framLen -= reqMax;
            while ((++multiFrame * 7) < framLen)  ;

            stepper = 0x21;
            GMLAN_SendGoAhead();
        }

        for (uint32_t i = 0; i <= reqMax; i++)
            *bufPtr++ = *ptr++;

        if (!multiFrame)
            return (void *)&reqBy[0];
    }
    return 0;
}

void GMLAN_Arbiter(const void *dataptr)
{
    const uint8_t *ptr = REQ_RecFrame(dataptr);
    if (ptr)
    {
        if (*ptr != GoAhead)
        {
            // These must respond even while doing things
            // It is up to each function to determine what can and can not be done while doing other stuff
            switch (ptr[1])
            {
                case StartRoutineByIdentifier:
                    REQ_StrtRoutById(ptr);
                    return;
                case RequestRoutineResult:
                    REQ_ReqRoutResById(ptr);
                    return;
                default:
                    break;
            }
        }

        // Major tasks
        if (wrkMask)
            GMLAN_SendFail(ptr[1], condNotCorrReqSeqErr);
        else if (*ptr == GoAhead && !notGoAhead)
        {
            if (waitGoAhead == 2)
                waitGoAhead = 1;
        }
        else
        {
            // Invalidate old answers
            waitGoAhead = 0;
            respMsgs    = 0;

            switch (ptr[1])
            {
                case ReturnToNormalMode:
                    REQ_ReturnToNormal(ptr);
                    return;
                case ReadDataByIdentifier:
                    REQ_RdDataByID(ptr);
                    return;
                case WriteDataByIdentifier:
                    REQ_WrDataByID(ptr);
                    return;
                case ReadMemoryByAddress:
                    REQ_RdMemByAddr(ptr);
                    return;
                case TransferData:
                    REQ_TransferData(ptr);
                    return;
                case TransferLzData:
                    REQ_TransferLzData(ptr);
                    return;
                default:
                    GMLAN_SendFail(ptr[1], SubFuncNotSupInvForm);
                    return;
            }
        }
    }
}

#pragma GCC pop_options
