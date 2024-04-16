#ifndef CANUSB_H
#define CANUSB_H

#include <thread> 
#include "../adapter.h"
#include "../../tools/tools.h"

extern "C"
{
#if defined (_WIN32)
#include "../../libs/ftd2xx_win.h"
#else
#include "../../libs/ftd2xx_nix.h"
#endif
}


class canusb : public adapter_t
{
public:
    explicit canusb();
	~canusb();

	std::list <std::string> adapterList();
	bool                    open(channelData);
	bool                    close();
    bool                    send(msgsys::message_t*);

private:
	FT_HANDLE                canusbContext = nullptr;

	bool                     loadLibrary();
    bool                     unloadLibrary();
    std::list <std::string>  findCANUSB();
	void                    *m_open(std::string);
	static void              messageThread(canusb*);

	volatile bool            runThread;

	std::thread             *messageThreadPtr = nullptr;


    bool closeChannel();


    bool openChannel(FT_HANDLE, int);
    bool CalcAcceptanceFilters(FT_HANDLE, std::list <uint32_t>);
};

#endif
