#include <cstdio>
#include <string>
#include <list>
#include <stdio.h>
#include <string.h>
#include <usb.h>
#include <iostream>

#include "kvaser.h"

using namespace std;
using namespace msgsys;
using namespace logger;

// 0bfd:0004 Kvaser AB USBcan II

#define VENDOR_ID  0x0bfd
#define PRODUCT_ID 0x0004
/*
static void kvaser::bahs(usb_device *dev)
{

}
*/

std::list <std::string> kvaser::findUSBCANII()
{
    struct usb_device *dev;
    struct usb_bus *bus;
    list <string> adapters;

    uint usbcan2cnt = 0;

    for (kvaserDesc_t *desc : kvaserList)
        delete desc;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    for (dev = bus->devices;     dev; dev = dev->next)
    {
        if (dev->descriptor.idVendor  == 0x0bfd) // KVASER
        {
            // kvaserDesc_t *desc = new kvaserDesc_t;
            usb_dev_handle *handle;

            switch (dev->descriptor.idProduct)
            {
            case 0x0004: // USBcan II
                adapters.push_back(to_string(++usbcan2cnt) + ": USBcan II (p1)");
                adapters.push_back(to_string(  usbcan2cnt) + ": USBcan II (p2)");
                break;
            default:
                continue;
            }


/*
            if (!(handle = usb_open(dev)))
            {
                log("Could not open");
                continue;
            }

            usb_get_string_simple(handle, dev->descriptor.iManufacturer, VID, 256);
            usb_get_string_simple(handle, dev->descriptor.iProduct     , PID, 256);
            
            // log(toString(VID));
            // log(toString(PID));
            adapters.push_back(toString(PID));


            usb_close(handle);*/
        }
    }

    return adapters;
}



kvaser::~kvaser()
{
}

list <string> kvaser::adapterList()
{
    usb_init();
    usb_find_busses();
    usb_find_devices();
    return findUSBCANII();
}

bool kvaser::open(std::string)
{
    log("Trying to open");
    return false;
}

bool kvaser::close()
{
    return true;
}

bool kvaser::send(message_t*)
{
    return false;
}
