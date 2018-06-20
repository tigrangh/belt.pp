#pragma once

#include "socket.hpp"

#ifndef B_OS_WINDOWS
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//  sys/types seems to be unnecessary on modern systems
#include <sys/types.h>
#include <fcntl.h>

//  close the file descriptor
#include <unistd.h>

#include <cerrno>
#include <cstring>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

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
#ifdef B_OS_WINDOWS
    return socket_descriptor.handle == INVALID_SOCKET;
#else
    return socket_descriptor.handle == -1;
#endif
}

inline string last_error() noexcept
{
#ifdef B_OS_WINDOWS
    return std::to_string(WSAGetLastError());
#else
    return strerror(errno);
#endif
}

inline void startup()
{
#ifdef B_OS_WINDOWS
    WSADATA wsaData;
    
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    if (err != 0)
        throw std::runtime_error("WSAStartup failed with error: " + err);
#endif
}

inline void shutdown()
{
#ifdef B_OS_WINDOWS
    if (WSACleanup())
        throw std::runtime_error(last_error());
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

#ifdef B_OS_WINDOWS
inline void connect(SOCKET fd,
                    const struct sockaddr FAR * addr,
                    size_t len,
                    beltpp::ip_address const& address)
{
    int res = ::connect(fd, addr, int(len));

    if (SOCKET_ERROR != res || WSAGetLastError() != WSAEWOULDBLOCK)
    {
        char msgbuf[256];
        msgbuf[0] = '\0';

        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       WSAGetLastError(),
                       MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                       msgbuf,
                       sizeof (msgbuf),
                       NULL);

        string native_error(msgbuf);
        string connect_error = "connect(";
        connect_error += address.to_string();
        connect_error += "): ";
        connect_error += native_error;
        throw std::runtime_error(connect_error);
    }
}
#else
inline void connect(int fd,
                    const struct sockaddr* addr,
                    socklen_t len,
                    beltpp::ip_address const& address)
{
    int res = ::connect(fd, addr, len);

    if (-1 != res || errno != EINPROGRESS)
    {
        string native_error = ::strerror(errno);
        string connect_error = "connect(";
        connect_error += address.to_string();
        connect_error += "): ";
        connect_error += native_error;
        throw std::runtime_error(connect_error);
    }
}
#endif
}
}
