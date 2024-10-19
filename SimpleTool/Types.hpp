#ifndef SIM_TYPES_HPP_
#define SIM_TYPES_HPP_
#include <string>
#include <vector>
#include <list>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #ifndef OS_WINDOWS
        #define OS_WINDOWS
    #endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #ifndef OS_LINUX
        #define OS_LINUX
    #endif
#else
    #error "不支持的平台"
#endif

namespace sim
{
#ifdef OS_WINDOWS
    typedef signed char         Int8;
    typedef signed short        Int16;
    typedef signed int          Int32;
    typedef signed __int64      Int64;
    typedef unsigned char       UInt8;
    typedef unsigned short      UInt16;
    typedef unsigned int        UInt32;
    typedef unsigned __int64    UInt64;
#elif defined(OS_LINUX)
    typedef signed char         Int8;
    typedef signed short        Int16;
    typedef signed int          Int32;
    typedef long long      Int64;
    typedef unsigned char       UInt8;
    typedef unsigned short      UInt16;
    typedef unsigned int        UInt32;
    typedef unsigned long long    UInt64;
#endif // OS_LINUX

    typedef std::string     String;
    #define tVector         std::vector
    #define tList           std::list
}
#endif