#pragma once

#include "global.hpp"
#include "socket.hpp"

#include <belt.pp/scope_helper.hpp>

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

#ifdef B_OS_WINDOWS
inline string win_last_error(int code) noexcept
{
    LPSTR msgbuf = nullptr;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msgbuf,
        0,
        NULL);

    string native_error;
    if (msgbuf)
    {
        native_error = msgbuf;
        LocalFree(msgbuf);
    }


    return native_error;
}
#endif


inline string net_error(int code) noexcept
{
#ifdef B_OS_WINDOWS
    return win_last_error(code);
#else
    return strerror(code);
#endif
}

inline string net_last_error() noexcept
{
#ifdef B_OS_WINDOWS
    return win_last_error(WSAGetLastError());
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
        throw std::runtime_error("wsacleanup(): " + net_last_error());
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

inline bool check_connection_refused(int res, int error_code)
{
#ifdef B_OS_WINDOWS
    return -1 == res && error_code == ERROR_CONNECTION_REFUSED;
#else
    return -1 == res && error_code == ECONNREFUSED;
#endif
}

inline bool check_connected_error(int res, int)
{
#ifdef B_OS_WINDOWS
    return -1 == res;
#else
    return -1 == res;
#endif
}

inline bool check_accepted_skip(int res, int error_code)
{
#ifdef B_OS_WINDOWS
    return -1 == res && ERROR_NETNAME_DELETED == error_code;
#else
    return false;
#endif
}

inline bool check_recv_block(int res, int error_code)
{
#ifdef B_OS_WINDOWS
    return false;
    //return res == SOCKET_ERROR && error_code == WSAEWOULDBLOCK;
#else
    return -1 == res && (error_code == EWOULDBLOCK || error_code == EAGAIN);
#endif
}

inline bool check_recv_connect(int res, int error_code)
{
#ifdef B_OS_WINDOWS
    return res == SOCKET_ERROR && 
                    (error_code == WSAECONNRESET ||
                     error_code == WSAECONNABORTED ||
                     error_code == ERROR_NETNAME_DELETED ||
                     error_code == ERROR_CONNECTION_ABORTED);
#else
    return -1 == res && error_code == ECONNRESET;
#endif
}

inline bool check_recv_fail(int res)
{
#ifdef B_OS_WINDOWS
    return res == SOCKET_ERROR;
#else
    return -1 == res;
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
inline SOCKET socket(int af, int type, int protocol)
{
    return ::WSASocket(af, type, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
}
#else
inline int socket(int af, int type, int protocol)
{
    return ::socket(af, type, protocol);
}
#endif
#ifdef B_OS_WINDOWS
int get_connected_status(beltpp::detail::event_handler_impl* peh, uint64_t id, SOCKET, int& error_code);
#else
inline int get_connected_status(beltpp::detail::event_handler_impl*, uint64_t, int socket_descriptor, int& error_code)
{
    int so_error;
    socklen_t size = sizeof(so_error);

    if (-1 == getsockopt(socket_descriptor,
                         SOL_SOCKET, SO_ERROR,
                         native::sockopttype(&so_error), &size))
    {
        error_code = so_error;
        return -1;
    }
    else
    {
        if (0 == so_error)
        {
            error_code = so_error;
            return 0;
        }
        else
        {
            error_code = so_error;
            return -1;
        }
    }
}
#endif
#ifdef B_OS_WINDOWS
SOCKET accept(beltpp::detail::event_handler_impl* peh,
              uint64_t id,
              SOCKET,
              int& res,
              int& error_code);
#else
int accept(beltpp::detail::event_handler_impl*,
           uint64_t,
           int socket_descriptor,
           int& res,
           int& error_code);
#endif
#ifdef B_OS_WINDOWS
int recv(beltpp::detail::event_handler_impl* peh,
         uint64_t id,
         SOCKET,
         char* buf,
         int len,
         int& error_code);
#else
int recv(beltpp::detail::event_handler_impl*,
         uint64_t,
         int socket_descriptor,
         char* buf,
         int len,
         int& error_code);
#endif

#ifdef B_OS_WINDOWS
inline
size_t send(SOCKET socket_descriptor, char const* buf, size_t size)
{
    int res = ::send(socket_descriptor, buf, (int)size, 0);
    if (-1 == res &&
        WSAEWOULDBLOCK == ::WSAGetLastError())
        res = 0;

    return size_t(res);
}
#else

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
# ifdef SO_NOSIGPIPE
#  define BELT_USE_SO_NOSIGPIPE
# else
#  error "That's a problem, cannot block SIGPIPE..."
# endif
#endif

inline
size_t send(int socket_descriptor, char const* buf, size_t size)
{
    ssize_t res = ::send(socket_descriptor, reinterpret_cast<void const*>(buf), size, MSG_NOSIGNAL);
    if (-1 == res &&
        (
            EWOULDBLOCK == errno ||
            EAGAIN == errno
        ))
        res = 0;

    return size_t(res);
}
#endif
#ifdef B_OS_WINDOWS
void connect(beltpp::detail::event_handler_impl* peh,
             uint64_t id,
             SOCKET fd,
             const struct sockaddr* addr,
             size_t len,
             beltpp::ip_address const& address);
#else
inline void connect(beltpp::detail::event_handler_impl* /*peh*/,
                    uint64_t /*id*/,
                    int fd,
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
