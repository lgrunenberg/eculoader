#ifndef TOOLS_H
#define TOOLS_H

#include <cstdio>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "logger/logger.h"
#include "timer/timer.h"
#include "checksum/checksum.h"
#include "typedef.h"

extern std::string toString(const char* a);

extern u8  asc2u8(const char *ptr);
extern u16 asc2u16(const char *ptr);
extern u32 asc2u24(const char *ptr);
extern u32 asc2u32(const char *ptr);

extern size_t openRead( const char *fName, FILE **fp );
extern bool openWrite( const char *fName, FILE **fp );

// Because, F*CK C++..
// They can not hande 1-byte integers for some weird reason!
template< typename T >
std::string to_hex( T i )
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(sizeof(T)*2) << i;
    return stream.str();
}

template< typename T >
std::string to_hex( T i, uint32_t size)
{
    std::stringstream stream;
    if (size) stream << std::hex << std::setfill ('0') << std::setw(size*2) << i;
    else      stream << std::hex << i;
    return stream.str();
}

template< typename T >
std::string to_hex24( T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(6) << i;
    return stream.str();
}

template< typename T >
std::string to_hex12( T i)
{
    std::stringstream stream;
    stream << std::hex << std::setfill ('0') << std::setw(3) << i;
    return stream.str();
}

template <typename T>
std::string to_string_fcount(const T a_value, const int32_t n = 2)
{
    std::ostringstream out;
    out << std::setfill('0') << std::setw(n) << a_value;
    return out.str();
}

#endif