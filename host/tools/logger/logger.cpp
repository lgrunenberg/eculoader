#include <sstream>
#include <mutex>

#include "logger.h"


#include <iostream>
#include <csignal>
#include <cstdlib>

using namespace std;
using namespace logger;

#warning "This needs rework!"

namespace logger
{
    static logger_t *m_parent = nullptr;
    static mutex logMtx;

    void loggerInstall(logger_t *parent)
    {
        m_parent = parent;
    }

    void log(logWho who, string message)
    {
        logMtx.lock();
        if (m_parent != nullptr)
            m_parent->log(who, message);
        logMtx.unlock();
    }

    void log(string message)
    {
        log(genericlog, message);
    }

    void progress(uint32_t prog)
    {
        logMtx.lock();
        if (m_parent != nullptr)
            m_parent->progress(prog);
        logMtx.unlock();
    }
}
