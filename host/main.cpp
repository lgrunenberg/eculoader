#include <iostream>
#include <csignal>
#include <cstdlib>

#include "tools/tools.h"

#include "ecu/ecu.h"

using namespace logger;
using namespace std;


class parentlogger_t : logger_t {
public:
    void log(logWho who, string message)
    {
        string loggedby;

        switch (who)
        {
            default:           loggedby = "Unknown       : "; break;
        }

        cout << loggedby + message + '\n';
    }
};

parentlogger_t parentlogger;

// Rework..
class ecuSpace : public e39
{};

ecuSpace ecu;

static void exiting()
{
    ecu.close();
    ecu.~ecuSpace();
    log("Going down");
    exit(1);
}

static void prepMain()
{
    srand(time(NULL));

    auto lam = [] (int i)
    {
        exiting();
        exit(1);
    };

    signal(SIGINT, lam);
    signal(SIGABRT, lam);
    signal(SIGTERM, lam);

    loggerInstall((logger_t*)&parentlogger);
}

// mpc5566/out/mainloader.bin

int main()
{
    // adapterKvaser, adapterCanUsb
    string adapterList = "";
    list <string> adapters = ecu.listAdapters( adapterKvaser );

    prepMain();

    printf("Test\n");

    for (string adapt : adapters)
    {
        adapterList += adapt + " ";
    }

    log("Adapter list: " + adapterList);

    if (!adapters.size())
    {
        log("No adapters");
        return 1;
    }

    channelData chDat;
    chDat.name = adapters.front();
    chDat.bitrate = btr500k;
    chDat.canIDs = { 0x7e0, 0x7e8, 0x101, 0x011 };

    if(! ecu.open( chDat ) )
    {
        log("Could not open");
        return 1;
    }




    ecu.e39::dump();













    return 0;

}