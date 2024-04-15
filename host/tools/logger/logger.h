#include <cstdio>
#include <string>

namespace logger
{
    // For future in case you want to push the log to a particular file or parent
    enum logWho {
        genericlog    ,
        filemanager   ,
        e39log        ,
    };

    class logger_t {
    public:
        virtual void log(logWho who, std::string message) = 0;
    };

    void loggerInstall (logger_t    *parent);
    void log           (logWho       who, std::string message);
    void log           (std::string  message);
}
