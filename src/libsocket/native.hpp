#pragma once

#include "global.hpp"
#include "socket.hpp"

#include <belt.pp/scope_helper.hpp>
#include <belt.pp/queue.hpp>

#ifdef USE_BOOST_SOCKET
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#endif

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

class sync_result
{
public:
    bool is_set = false;
    bool succeeded = false;
    int error_code = 0;
};

class socket_handle
{
public:
#ifdef B_OS_WINDOWS
using handle_type = SOCKET;
static handle_type const invalid_value = INVALID_SOCKET;
#else
using handle_type = int;
static handle_type const invalid_value = -1;
#endif

    socket_handle()
        : handle(invalid_value)
    {}
    socket_handle(handle_type handle_)
        : handle(handle_)
    {}
    socket_handle(socket_handle const&) = delete;
    socket_handle(socket_handle&& other)
    {
        handle = other.handle;
        other.handle = invalid_value;
    }

    socket_handle& operator = (socket_handle const&) = delete;
    socket_handle& operator = (socket_handle&& other)
    {
        handle = other.handle;
        other.handle = invalid_value;
        return *this;
    }

    ~socket_handle()
    {
        reset();
    }

    bool is_invalid() const noexcept
    {
        return handle == invalid_value;
    }
    int reset() noexcept
    {
        if (handle != invalid_value)
        {
            auto copy = handle;
            handle = invalid_value;
            #ifdef B_OS_WINDOWS
            return closesocket(copy);
            #else
            return ::close(copy);
            #endif
        }
        return 0;
    }

    handle_type handle;
};

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

inline bool check_connection_refused(bool res, int error_code)
{
#ifdef B_OS_WINDOWS
    return false == res && error_code == ERROR_CONNECTION_REFUSED;
#else
    return false == res && error_code == ECONNREFUSED;
#endif
}

inline bool check_connected_error(bool res, int)
{
#ifdef B_OS_WINDOWS
    return false == res;
#else
    return false == res;
#endif
}

inline bool check_accepted_skip(bool res, int error_code)
{
#ifdef B_OS_WINDOWS
    return false == res && ERROR_NETNAME_DELETED == error_code;
#else
    B_UNUSED(res);
    B_UNUSED(error_code);
    return false;
#endif
}

inline bool check_recv_block(size_t res, int error_code)
{
#ifdef B_OS_WINDOWS
    return res == size_t(SOCKET_ERROR) && error_code == WSAEWOULDBLOCK;
#else
    return size_t(-1) == res && (error_code == EWOULDBLOCK || error_code == EAGAIN);
#endif
}

inline bool check_recv_connect(size_t res, int error_code)
{
#ifdef B_OS_WINDOWS
    return res == size_t(SOCKET_ERROR) &&
            (error_code == WSAECONNRESET ||
             error_code == WSAECONNABORTED ||
             error_code == ERROR_NETNAME_DELETED ||
             error_code == ERROR_CONNECTION_ABORTED);
#else
    return size_t(-1) == res && error_code == ECONNRESET;
#endif
}

inline bool check_recv_fail(size_t res)
{
#ifdef B_OS_WINDOWS
    return res == size_t(SOCKET_ERROR);
#else
    return size_t(-1) == res;
#endif
}

inline bool check_send_dropped(size_t res, int error_code)
{
#ifdef B_OS_WINDOWS
    return (size_t(-1) == res && error_code == WSAECONNRESET);
#else
    return (size_t(-1) == res && error_code == EPIPE);
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
bool get_connected_status(beltpp::detail::event_handler_impl* peh,
                          uint64_t id,
                          SOCKET,
                          int& error_code);
#else
inline bool get_connected_status(beltpp::detail::event_handler_impl*,
                                 uint64_t,
                                 int socket_descriptor,
                                 int& error_code)
{
    int so_error;
    socklen_t size = sizeof(so_error);

    if (-1 == getsockopt(socket_descriptor,
                         SOL_SOCKET, SO_ERROR,
                         native::sockopttype(&so_error), &size))
    {
        error_code = so_error;
        return false;
    }
    else
    {
        if (0 == so_error)
        {
            error_code = so_error;
            return true;
        }
        else
        {
            error_code = so_error;
            return false;
        }
    }
}
#endif
#ifdef B_OS_WINDOWS
SOCKET accept(beltpp::detail::event_handler_impl* peh,
              uint64_t id,
              SOCKET,
              bool& res,
              int& error_code);
#else
int accept(beltpp::detail::event_handler_impl*,
           uint64_t,
           int socket_descriptor,
           bool& res,
           int& error_code);
#endif
#ifdef B_OS_WINDOWS
size_t recv(beltpp::detail::event_handler_impl* peh,
            uint64_t id,
            SOCKET,
            char* buf,
            size_t len,
            int& error_code);
#else
size_t recv(beltpp::detail::event_handler_impl*,
            uint64_t,
            int socket_descriptor,
            char* buf,
            size_t len,
            int& error_code);
#endif

#ifdef B_OS_WINDOWS
void async_send(SOCKET /*socket_descriptor*/,
                beltpp::detail::event_handler_impl* /*peh*/,
                ievent_item& /*ev_it*/,
                uint64_t /*ev_id*/,
                uint64_t /*eh_id*/,
                beltpp::queue<char>& /*send_stream*/,
                std::string const& /*message*/);

/*size_t send(SOCKET socket_descriptor,
            char const* buf,
            size_t size);*/

size_t send(SOCKET /*socket_descriptor*/,
            beltpp::detail::event_handler_impl* /*peh*/,
            ievent_item& /*ev_it*/,
            uint64_t /*ev_id*/,
            uint64_t /*eh_id*/,
            beltpp::queue<char>& /*send_stream*/,
            int& error_code);
#else

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
# ifdef SO_NOSIGPIPE
#  define BELT_USE_SO_NOSIGPIPE
# else
#  error "That's a problem, cannot block SIGPIPE..."
# endif
#endif

void async_send(int socket_descriptor,
                beltpp::detail::event_handler_impl* peh,
                ievent_item& ev_it,
                uint64_t ev_id,
                uint64_t eh_id,
                beltpp::queue<char>& send_stream,
                std::string const& message);

size_t send(int socket_descriptor,
            beltpp::detail::event_handler_impl* peh,
            ievent_item& ev_it,
            uint64_t ev_id,
            uint64_t eh_id,
            beltpp::queue<char>& send_stream,
            int& error_code);
#endif

#ifdef B_OS_WINDOWS
void connect(beltpp::detail::event_handler_impl* peh,
             uint64_t id,
             SOCKET fd,
             const struct sockaddr* addr,
             size_t len,
             beltpp::ip_address const& address,
             sync_result& result);
#else
inline void connect(beltpp::detail::event_handler_impl* /*peh*/,
                    uint64_t /*id*/,
                    int fd,
                    const struct sockaddr* addr,
                    size_t len,
                    beltpp::ip_address const& /*address*/,
                    sync_result& result)
{
    int res = ::connect(fd, addr, socklen_t(len));

    if (-1 != res || errno != EINPROGRESS)
    {
        result.is_set = true;
        result.succeeded = (0 == res);
        if (false == result.succeeded)
            result.error_code = errno;
    }
}
#endif
#ifdef B_OS_WINDOWS
inline int bind(SOCKET socket_descriptor, const struct sockaddr* addr, size_t namelen)
{
    return ::bind(socket_descriptor, addr, int(namelen));
}
#else
inline int bind(int socket_descriptor, const struct sockaddr* addr, socklen_t namelen)
{
    return ::bind(socket_descriptor, addr, namelen);
}
#endif
}
}
