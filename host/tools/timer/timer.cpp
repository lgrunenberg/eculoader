#include "timer.h"

#include <condition_variable>
#include <mutex>
#include <cstdio>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace timer
{

void sleepMS(const uint32_t msTime)
{
    auto oldTime = system_clock::now();
    while (duration_cast<milliseconds>(system_clock::now() - oldTime).count() < msTime)
        ;
}

void sleepMicro(const uint32_t uTime)
{
    auto oldTime = system_clock::now();
    while (duration_cast<microseconds>(system_clock::now() - oldTime).count() < uTime)
        ;
}
}
