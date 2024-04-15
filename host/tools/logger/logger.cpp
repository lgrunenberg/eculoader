#include <sstream>
#include <mutex>

#include "logger.h"

using namespace std;

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
        if (m_parent)
            m_parent->log(who, message);
        logMtx.unlock();
    }

    void log(string message)
    {
        log(genericlog, message);
    }
}
