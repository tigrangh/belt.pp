#include "native.hpp"
#include "event_impl.hpp"

namespace beltpp
{
namespace native
{
#ifdef B_OS_WINDOWS
int get_connected_status(beltpp::detail::event_handler_impl* peh,
                         uint64_t id,
                         SOCKET socket_descritor,
                         int& error_code)
{
    auto& async_data = peh->m_poll_master.m_events.at(id);
    error_code = async_data->last_error;
    int res = (0 == error_code ? 0 : -1);

    if (0 == res)
    {
        int option = 0;
        int res2 = ::setsockopt(socket_descritor,
            SOL_SOCKET,
            SO_UPDATE_CONNECT_CONTEXT,
            NULL,
            0);

        if (res2 != NO_ERROR)
            throw std::runtime_error("setsockopt(): " + native::net_last_error());
    }

    return res;
}
#endif
#ifdef B_OS_WINDOWS
SOCKET accept(beltpp::detail::event_handler_impl* peh,
              uint64_t id,
              SOCKET socket_descriptor,
              int& res,
              int& error_code)
{
    auto& async_data = peh->m_poll_master.m_events.at(id);

    error_code = async_data->last_error;
    res = (0 == error_code ? 0 : -1);
    SOCKET result = INVALID_SOCKET;

    if (0 == res)
    {
        result = async_data->accept_socket;

        int res = ::setsockopt(result,
            SOL_SOCKET,
            SO_UPDATE_ACCEPT_CONTEXT,
            (char const*)(&socket_descriptor),
            sizeof(SOCKET));

        if (res != NO_ERROR)
            throw std::runtime_error("setsockopt(): " + native::net_last_error());
    }

    async_data->accept_socket = INVALID_SOCKET;
    async_data->init();
    return result;
}
#else
int accept(beltpp::detail::event_handler_impl*,
           uint64_t,
           int socket_descriptor,
           int& res,
           int& error_code)
{
    sockaddr_storage address;
    socklen_t size = sizeof(sockaddr_storage);
    res = 0;
    error_code = 0;
    return ::accept(socket_descriptor, reinterpret_cast<sockaddr*>(&address), &size);
}
#endif

#ifdef B_OS_WINDOWS
size_t recv(beltpp::detail::event_handler_impl* peh,
            uint64_t id,
            SOCKET,
            char* buf,
            size_t len,
            int& error_code)
{
    auto& async_data = peh->m_poll_master.m_events.at(id);
    if (0 != async_data->last_error)
    {
        error_code = async_data->last_error;
        return size_t(SOCKET_ERROR);
    }
    else
    {
        if (len > async_data->bytes_copied - async_data->bytes_offset)
            len = async_data->bytes_copied - async_data->bytes_offset;

        memcpy(buf, async_data->receive_buffer + async_data->bytes_offset, len);

        async_data->bytes_offset += len;
        if (async_data->bytes_copied == async_data->bytes_offset)
        {
            async_data->running = false;
            async_data->init();
        }
        else
            async_data->running = true;

        return len;
    }
}
#else
size_t recv(beltpp::detail::event_handler_impl*,
            uint64_t,
            int socket_descriptor,
            char* buf,
            size_t len,
            int& error_code)
{
    auto result = ::recv(socket_descriptor, reinterpret_cast<void*>(buf), len, 0);
    error_code = errno;

    return size_t(result);
}
#endif
#ifdef B_OS_WINDOWS
void connect(beltpp::detail::event_handler_impl* peh,
             uint64_t id,
             SOCKET /*fd*/,
             const struct sockaddr* addr,
             size_t len,
             beltpp::ip_address const& address)
{
    auto& async_data = peh->m_poll_master.m_events.at(id);
    async_data->connect(addr, len, address);
}
#endif
}
}
