#pragma once


#ifndef B_OS_WINDOWS
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//  sys/types seems to be unnecessary on modern systems
#include <sys/types.h>
#include <fcntl.h>

//  close the file descriptor
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <cerrno>
#include <cstring>
#include <string>

using std::string;

namespace beltpp
{
namespace native
{
class sk_handle
{
public:
    sk_handle() :handle(0) {}

#ifdef B_OS_WINDOWS
    SOCKET handle;
#else
    int handle;
#endif
};

inline int close(sk_handle const& socket_descriptor) noexcept
{
#ifdef B_OS_WINDOWS
    return closesocket(socket_descriptor.handle);
#else
    return ::close(socket_descriptor.handle);
#endif
}

inline bool is_invalid(sk_handle const& socket_descriptor) noexcept
{
    return socket_descriptor.handle == -1;
}

inline void startup() noexcept
{

}

inline void shutdown() noexcept
{

}
inline string last_error() noexcept
{
#ifdef B_OS_WINDOWS
    return "TODO:";
#else
    return strerror(errno);
#endif
}

inline char const* gai_error(int ecode)
{
#ifdef B_OS_WINDOWS
    return gai_strerrorA(ecode);
#else
    return gai_strerror(ecode);
#endif
}

#ifdef B_OS_WINDOWS
inline char* sockopttype(int* param)
{
    return reinterpret_cast<char*>(param);
}
#else
inline int* sockopttype(int* param)
{
    return param;
}
#endif
}
}
