#ifndef GMLAN_H
#define GMLAN_H

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

#endif
