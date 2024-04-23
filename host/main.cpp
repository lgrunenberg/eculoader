#include <iostream>
#include <csignal>
#include <cstdlib>

#include "tools/tools.h"

#include "ecu/ecu.h"

using namespace logger;
using namespace std;


class parentlogger_t : public logger_t {
public:
    void log(const logWho & who, const string & message)
    {
        string loggedby;

        switch (who)
        {
            case filemanager:  loggedby = "Filemanager   : "; break;
            case e39log:       loggedby = "E39           : "; break;
            case gmlanlog:     loggedby = "GMLAN         : "; break;
            case adapterlog:   loggedby = "Adapter       : "; break;
            default:           loggedby = "- - -         : "; break;
        }

        cout << loggedby + message + '\n';
    }

    void progress(uint32_t prog)
    {
        static uint32_t oldProg = 100;

        if ( prog > 100 )
            prog = 100;
        
        if ( prog < oldProg || prog >= (oldProg + 5) || (oldProg < prog && prog == 100))
        {
            oldProg = prog;
            cout << to_string( prog ) << "%" "\n";
        }
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

    loggerInstall( &parentlogger );
}

int main()
{
    prepMain();

    log("Test");

    // adapterKvaser, adapterCanUsb
    string adapterList = "";
    list <string> adapters = ecu.listAdapters( adapterKvaser );

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
    chDat.canIDs = { 0x7e0, 0x7e8, 0x101, 0x011, 0x002, 0x003 };

    if(! ecu.open( chDat ) )
    {
        log("Could not open");
        return 1;
    }

    ecu.e39::dump();

    return 0;

}