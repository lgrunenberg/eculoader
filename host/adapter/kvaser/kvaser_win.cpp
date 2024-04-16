#include <cstdio>
#include <string>
#include <list>
#include <iostream>

#include "kvaser.h"

using namespace std;
using namespace msgsys;
using namespace logger;

void kvaser::messageThread(kvaser *thisInstance) {}

kvaser::kvaser()
{
    canInitializeLibrary();
}

kvaser::~kvaser()
{
    this->close();
}

list <string> kvaser::adapterList()
{
    list<string> adapters;
    uint32_t card_channel = 0;
    int32_t num = 0;
    char name[100];

    canStatus stat = canGetNumberOfChannels(&num);

    for (int i = 0; (i < num) && (stat == canOK); i++)
    {
        name[0] = 0;
        stat = canGetChannelData(i, canCHANNELDATA_DEVDESCR_ASCII, name, sizeof(name));
        if (stat != canOK) continue;

        stat = canGetChannelData(i, canCHANNELDATA_CHAN_NO_ON_CARD, &card_channel, sizeof(card_channel));
        if (stat != canOK) continue;

        string strname = toString(name) + " P" + to_string(card_channel);
        adapters.push_back(strname);
    }

    return adapters;
}

bool kvaser::close()
{
    if (kvaserHandle != -1)
    {
        canBusOff((CanHandle)kvaserHandle);
        canClose((CanHandle)kvaserHandle);
        kvaserHandle = -1;
    }

    return true;
}

bool kvaser::CalcAcceptanceFilters(list<uint32_t> idList) 
{
    unsigned int code = ~0;
    unsigned int mask = 0;

    if (idList.size() > 0)
    {
        for (uint32_t canID : idList)
        {
            // log("Filter+ " + to_hex(canID));
            if (canID == 0)
            {
                log("Found illegal id");
                code = ~0;
                mask = 0;
                goto forbidden;
            }

            code &= canID;
            mask |= canID;
        }

        mask = ~(code ^ mask);
        // mask = (~mask & 0x7FF) | code;
    }

forbidden:

    return (canSetAcceptanceFilter(kvaserHandle, code, mask, 0) == canOK);
}

void kvasOnCall(CanHandle hnd, void* context, unsigned int evt)
{
    uint8_t msgData[128];
    message_t msg;
    unsigned int dlc, flags;
    unsigned long ts;
    long ID;

    if (evt & canNOTIFY_RX)
    {
        while (canRead(hnd, &ID, msgData, &dlc, &flags, &ts) == canOK)
        {
            if (dlc > 8) {
                log("Kvaser: unsup long msg");
                dlc = 8;
            }

            for (uint32_t i = 0; i < dlc; i++)
                msg.message[i] = msgData[i];

            for (uint32_t i = dlc; i < 8; i++)
                msg.message[i] = 0;

            msg.length = dlc;
            msg.timestamp = ts;
            msg.id = ID;
            messageReceive(&msg);
        }
    }
}

bool kvaser::m_open(channelData device, int chan)
{
    unsigned long bitPerSec = 0;
    unsigned int tseg1 = 0;
    unsigned int tseg2 = 0;
    unsigned int sjw   = 0;
    unsigned int samp  = 0;
    unsigned int syncm = 0;

    switch(device.bitrate)
    {
    case btr500k:
        bitPerSec = canBITRATE_500K;
        break;
    case btr400k:
        bitPerSec = 400000;
        tseg1 = 17 - 1;
        tseg2 =  3;
        samp = 3;
        sjw = 2;
        break;
    default:
        log("Unknown bitrate");
        return false;
    }

    kvaserHandle = (int)canOpenChannel(chan, 0);
    if (kvaserHandle < 0) return false;

    if (canSetBusParams(kvaserHandle, bitPerSec, tseg1, tseg2, sjw, samp, syncm) != canOK)
        return false;

    if (canBusOn(kvaserHandle) != canOK)
        return false;

    kvSetNotifyCallback(kvaserHandle, &kvasOnCall, 0, canNOTIFY_RX | canNOTIFY_ERROR | canNOTIFY_REMOVED);

    return CalcAcceptanceFilters(device.canIDs);
}

bool kvaser::open(channelData device)
{
    string   name = device.name;
    uint32_t card_channel = 0;
    int32_t  num = 0;
    char cname[100];

    if (name.length() == 0)
    {
        return false;
    }

    log("Trying to open " + name);

    canStatus stat = canGetNumberOfChannels(&num);

    for (int i = 0; (i < num) && (stat == canOK); i++)
    {
        cname[0] = 0;
        stat = canGetChannelData(i, canCHANNELDATA_DEVDESCR_ASCII, cname, sizeof(cname));
        if (stat != canOK) continue;

        stat = canGetChannelData(i, canCHANNELDATA_CHAN_NO_ON_CARD, &card_channel, sizeof(card_channel));
        if (stat != canOK) continue;

        string strname = toString(cname) + " P" + to_string(card_channel);

        if (strname == name)
        {
            log("Device located. Opening..");
            this->close();
            return this->m_open(device, i);
        }
    }

    return false;
}

bool kvaser::send(message_t *msg)
{
    if (kvaserHandle >= 0)
        return (canWrite(kvaserHandle, (long)msg->id, &msg->message, (unsigned int)msg->length, 0) == canOK);

    return false;
}
