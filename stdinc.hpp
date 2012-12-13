#ifdef _MSC_VER
// building on VC++
#define _SCL_SECURE_NO_WARNINGS // Disable VC++ warnings for correct (but "unsafe") C++ code
#include <windows.h>
#include <shlobj.h>
typedef unsigned char uint8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#elif __CYGWIN__
#define  _WIN32_WINNT   0x0600
// building on GCC with Cygwin and MinGW
#include <windows.h>
#include <shlobj.h>
typedef unsigned char uint8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#endif

#include <vector>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/unordered_map.hpp>
#include <boost/exception/all.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rc4.h>
