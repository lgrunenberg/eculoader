#ifndef GMLAN_H
#define GMLAN_H

#include "../../tools/tools.h"
#include "../../adapter/adapter.h"

typedef struct {
    uint32_t PMA;  // Address
    uint32_t NOB;  // Size
} range_t;

typedef struct {
    uint32_t DCID;       // Data Compatibility identifier
    uint32_t NOAR;       // Number Of Address Regions
    range_t  range[16];  // Start address, size
} module_t;

class gmlan : public virtual adapter
{
private:
    uint64_t testerID;
    uint64_t targetID;
public:

    void setTesterID(uint64_t ID);
    void setTargetID(uint64_t ID);
    uint64_t getTesterID();
    uint64_t getTargetID();

    uint32_t busyWaitTimeout = 6000;

/*  - Time between tester request (N_USData.ind) and the ECU
      response as follows: The ECU has to make sure that a USDT single
      frame response message, the first frame of a multi-frame response
      message, or the first UUDT response message is completed
      (successfully transmitted onto the CAN bus) within P2CE */
    uint32_t p2ce = 100; // (0 - 100 / 0 - 5000)

/*  - The timeout between the end of the tester request message (single frame or multi-frame request
      message) and the first received frame of either the USDT response message (single frame, or first frame
      of a multi-frame response), or the first UUDT message of the services $AA, $A9 (there may be subsequent
      UUDT messages transmitted by the ECU to transmit the requested data) AND 
    - The timeout between each response message from multiple nodes responding during functional
      communication. */
    uint32_t p2ct = 500; // (150 - xxxxxx  ms) 

    // Time between tester present messages
    uint32_t p3c = 3500; // (5000 nominal / 5100 max)

    uint8_t *sendRequest(uint8_t*);
    uint8_t *sendRequest(uint8_t*, uint16_t);

    void sendAck(); // 3x xx xx

    // 04
    bool clearDiagnosticInformation();

    // 10
    // sub 02 == disableAllDTCs
    // sub 03 == enableDTCsDuringDevCntrl
    // sub 04 == wakeUpLinks
    bool InitiateDiagnosticOperation(uint8_t mode);

    // 1a
    uint8_t *ReadDataByIdentifier(uint8_t id);

    // 20
    bool returnToNormal();

    // 23
    uint8_t *readMemoryByAddress_16_8 (uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    uint8_t *readMemoryByAddress_16_16(uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    uint8_t *readMemoryByAddress_24_8 (uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    uint8_t *readMemoryByAddress_24_16(uint32_t address, uint32_t len, uint32_t blockSize = 0x80);

    uint8_t *readMemoryByAddress_32_8 (uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    uint8_t *readMemoryByAddress_32_16(uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    uint8_t *readMemoryByAddress_32_24(uint32_t address, uint32_t len, uint32_t blockSize = 0x80);

    // 28
    bool disableNormalCommunication(int exdAddr = -1);
    bool disableNormalCommunicationNoResp(int exdAddr = -1);

    // 34
    bool requestDownload_16(uint32_t size, uint8_t fmt = 0);
    bool requestDownload_24(uint32_t size, uint8_t fmt = 0);
    bool requestDownload_32(uint32_t size, uint8_t fmt = 0);

    // 36
    bool transferData_24(const uint8_t *data, uint32_t address, uint32_t len, uint32_t blockSize = 0x80, bool execute = false);
    bool transferData_32(const uint8_t *data, uint32_t address, uint32_t len, uint32_t blockSize = 0x80, bool execute = false);

    // 3b
    bool WriteDataByIdentifier(const uint8_t *dat, uint8_t id, uint32_t len);

    // 3e
    // Note: Setting extended address makes it broadcast the message with id 101
    bool testerPresent(int exdAddr = -1);

    // a2
    bool ReportProgrammedState();

    // a5
    bool ProgrammingMode(uint8_t);

    // ae
    bool DeviceControl(const uint8_t*,uint8_t,uint8_t);
};


class gmConfig
{
public:
    // Checksum
    uint32_t CS = 0;

    // Module ID
    uint32_t MID = 0;

    // HFI  Header Format Identifier
    uint32_t lSWMI = 4;     // Size of SWMI field (4-16)
    bool     lDLS  = false; // Extend DLS to 3 bytes instead of 2
    bool     PMA   = false; // Product Memory Address. Enable NOAR, Product Memory Address, Memory Length field fields
    bool     lHFI  = false; // Extend HFI to two bytes
    bool     MPFH  = false; // (EXTENDED) Include address information for several calibration files
    bool     DCID  = false; // (EXTENDED) Include compatibility identifier(s)

    // Software Module Identifier (4 - 16)
    uint8_t  SWMI[16] = {0};
    // Design Level Suffix or Alpha Code
    uint8_t  DLS[3] = {0};

    // Number Of Additional Modules (MPFH must be set for this to be present)
    uint32_t NOAM = 0;

    // Base target and eventual additional targets.
    module_t target[17] = {0};

    uint32_t construct(uint8_t *buffer)
    {
        uint32_t hdrLen = 0;
        return hdrLen;
    }

    void defaults()
    {
        // Reset checksum
        this->CS = 0;

        // Reset Module ID
        this->MID = 0;

        this->lSWMI = 4;

        this->lDLS = false;
        this->PMA  = false;
        this->lHFI = false;
        this->MPFH = false;
        this->DCID = false;

        this->NOAM = 0;

        this->nTargets = 0;

        for (uint32_t trg = 0; trg < (sizeof(this->target) / sizeof(this->target[0])); trg++)
        {
            this->target[trg].DCID = 0;
            this->target[trg].NOAR = 0;

            for (uint32_t i = 0; i < (sizeof(this->target[0].range) / sizeof(this->target[0].range[0])); i++)
            {
                this->target[trg].range[i].NOB = 0;
                this->target[trg].range[i].PMA = 0;
            }
        }

        for (uint32_t i = 0; i < sizeof(this->SWMI); i++)
            this->SWMI[i] = 0;
        for (uint32_t i = 0; i < sizeof(this->DLS); i++)
            this->DLS[i] = 0;
    }

    explicit gmConfig()
    {
        this->defaults();
    }
private:
    uint32_t nTargets = 0;
};

#endif
