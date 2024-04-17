#ifndef ADAPTER_H
#define ADAPTER_H

#include <cstdio>
#include <string>
#include <list>
#include "message.h"

class adapterDesc;

enum adaptertypes
{
    adapterCanUsb, // Lawicel CANUSB
    adapterKvaser, // Various Kvaser devices
};

enum adapterPorts
{
    portCAN,
    portKline
};

enum enBitrate
{
    btr500k,
    btr400k,
    btr300k,
    btr200k,
};

typedef struct {
    std::string name;
    std::list<uint32_t> canIDs;
    enBitrate bitrate;
} channelData;


// Adapter class "template". Used "internally"
class adapter_t
{
public:
    // explicit adapter_t();
    virtual ~adapter_t() = 0;
    virtual std::list <std::string> adapterList() = 0;
    virtual bool open(channelData&) = 0;
    virtual bool close() = 0;
    virtual bool send(msgsys::message_t*) = 0;
};

class adapter
{
public:
    explicit adapter();
    ~adapter();
	std::list <std::string> listAdapters(adaptertypes);
	// std::list <std::string> listAdapters(adaptertypes);
	bool open(channelData&);
	bool close();
	bool send(msgsys::message_t*);
protected:

private:
	bool setAdapter(adaptertypes);
    adapter_t *adapterContext = 0;
};


// Unique descriptor of an adapter
class adapterDesc
{
public:
	adaptertypes type;   // 
	std::string  name;   // Adapter name. ie USBcan II, canusb, combiadapter etc
	std::string  port;   // If an adapter has several ports of the same type, this one describes which one to use
	std::string  serial; // Unique identifier
};


#endif
