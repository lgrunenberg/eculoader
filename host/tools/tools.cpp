#include "tools.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace logger;

string toString(const char* a) 
{
    string s = "";
    while (*a)
        s += *a++;

    return s; 
}

u8 asc2u8(const char *ptr)
{
    u8 tmp1, tmp2;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr)   > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    return (u8)( (tmp1 << 4) | tmp2 );
}

u16 asc2u16(const char *ptr)
{
    u8  tmp1, tmp2, tmp3, tmp4;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr)   > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    return (u16)( (tmp1 << 12) | (tmp2 << 8) |
                  (tmp3 <<  4) | (tmp4) );
}

u32 asc2u24(const char *ptr)
{
    u8 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr++) > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    tmp5 = ((tmp5 = (u8)*ptr++) > 64) ? (u8)(tmp5 + 201) : (u8)(tmp5 + 208);
    tmp6 = ((tmp6 = (u8)*ptr)   > 64) ? (u8)(tmp6 + 201) : (u8)(tmp6 + 208);
    return (u32)( (tmp1 << 20) | (tmp2 << 16) |
                  (tmp3 << 12) | (tmp4 <<  8) |
                  (tmp5 <<  4) | (tmp6) );
}

u32 asc2u32(const char *ptr)
{
    u8 tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
    tmp1 = ((tmp1 = (u8)*ptr++) > 64) ? (u8)(tmp1 + 201) : (u8)(tmp1 + 208);
    tmp2 = ((tmp2 = (u8)*ptr++) > 64) ? (u8)(tmp2 + 201) : (u8)(tmp2 + 208);
    tmp3 = ((tmp3 = (u8)*ptr++) > 64) ? (u8)(tmp3 + 201) : (u8)(tmp3 + 208);
    tmp4 = ((tmp4 = (u8)*ptr++) > 64) ? (u8)(tmp4 + 201) : (u8)(tmp4 + 208);
    tmp5 = ((tmp5 = (u8)*ptr++) > 64) ? (u8)(tmp5 + 201) : (u8)(tmp5 + 208);
    tmp6 = ((tmp6 = (u8)*ptr++) > 64) ? (u8)(tmp6 + 201) : (u8)(tmp6 + 208);
    tmp7 = ((tmp7 = (u8)*ptr++) > 64) ? (u8)(tmp7 + 201) : (u8)(tmp7 + 208);
    tmp8 = ((tmp8 = (u8)*ptr)   > 64) ? (u8)(tmp8 + 201) : (u8)(tmp8 + 208);
    return (u32)( (tmp1 << 28) | (tmp2 << 24) |
                  (tmp3 << 20) | (tmp4 << 16) |
                  (tmp5 << 12) | (tmp6 <<  8) |
                  (tmp7 <<  4) | (tmp8) );
}


#warning "Move this to a file class and make things neat"

size_t openRead( const char *fName, FILE **fp ) {

    if ( (*fp = fopen( fName, "rb" )) == nullptr ) {
        log("Error: openRead() - Could not get file handle");
        return 0;
    }

    fseek(*fp, 0L, SEEK_END);
    long fileSize = ftell(*fp);
    rewind(*fp);

    if ( fileSize < 0 ) {
        log("Error: openRead() - Unable to determine file size");
        fclose( *fp );
        return 0;
    }

    if ( fileSize == 0 ) {
        log("Error: openRead() - There's no data to read");
        fclose( *fp );
        return 0;
    }

    return (size_t)fileSize;
}

bool openWrite( const char *fName, FILE **fp ) {

    if ( (*fp = fopen( fName, "wb" )) == nullptr ) {
        log("Error: openWrite() - Could not get file handle");
        return false;
    }

    rewind( *fp );
    return true;
}
