#include "socket.hpp"

#include "native.hpp"
#include "event_impl.hpp"

#include <belt.pp/scope_helper.hpp>
#include <belt.pp/queue.hpp>
#include <belt.pp/packet.hpp>
#include <belt.pp/event.hpp>

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

#include <cstring>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <mutex>
#include <memory>
#include <list>
#include <utility>
//
using std::string;
using std::vector;
using std::pair;
using beltpp::on_failure;
using ptr_addrinfo = std::unique_ptr<addrinfo, void(*)(addrinfo*)>;

using sockets = vector<pair<beltpp::native::socket_handle, addrinfo*>>;
using peer_ids = beltpp::socket::peer_ids;
using packets = beltpp::socket::packets;

namespace beltpp
{
/*
 * internals
 */
namespace detail
{

class channel
{
public:
    enum class type {listening, streaming};
    //enum class status {idle, need_scan, need_read};

    channel(size_t attempts = 0,
            native::socket_handle&& socket_descriptor = native::socket_handle(),
            type e_type = type::listening,
            ip_address socket_bundle = ip_address(),
            uint64_t eh_id = 0,
            native::sync_result const& sync_result = native::sync_result())
        : m_closed(false)
        , m_socket_descriptor(std::move(socket_descriptor))
        , m_type(e_type)
        , m_attempts(attempts)
        , m_eh_id(eh_id)
        , m_socket_bundle(socket_bundle)
        , m_sync_result(sync_result)
    {}

    bool m_closed;
    native::socket_handle m_socket_descriptor;
    type m_type;
    size_t m_attempts;
    uint64_t m_eh_id;
    ip_address m_socket_bundle;
    beltpp::queue<char> m_stream;
    session_special_data m_special_data;
    native::sync_result m_sync_result;
};
using channels = beltpp::queue<channel>;

class socket_internals
{
public:
    socket_internals(beltpp::event_handler& eh,
                     detail::fptr_message_loader fmessage_loader,
                     beltpp::void_unique_ptr&& putl)
        : m_peh(&eh)
        , m_fmessage_loader(fmessage_loader)
        , m_putl(std::move(putl))
    {
        if (0 == counter)
        {
            native::startup();
        }
        ++counter;
    }

    ~socket_internals()
    {
        --counter;
        if (0 == counter)
        {
            native::shutdown();
        }
    }

    beltpp::event_handler* m_peh;
    detail::fptr_message_loader m_fmessage_loader;
    beltpp::void_unique_ptr m_putl;
    std::list<channels> m_lst_channels;
    std::mutex m_mutex;
    static size_t counter;
};
size_t socket_internals::counter = 0;

string construct_peer_id(uint64_t id, ip_address const& socket_bundle);
uint64_t parse_peer_id(string const& peer_id);
string dump(sockaddr& address);
void getaddressinfo(ptr_addrinfo& servinfo,
                    ip_address::e_type type,
                    string const& str_address,
                    unsigned short port,
                    bool hold_family_mismatch_exception = false);
void getaddressinfo(ptr_addrinfo& servinfo,
                    int ai_family,
                    string const& str_address,
                    unsigned short port,
                    bool hold_family_mismatch_exception = false);
sockets socket(addrinfo* servinfo,
               bool bind,
               bool reuse,
               bool non_blocking);
beltpp::socket::peer_id add_channel(beltpp::socket& self,
                                    detail::socket_internals* pimpl,
                                    native::socket_handle&& socket_descriptor,
                                    detail::channel::type e_type,
                                    size_t attempts,
                                    const struct sockaddr* addr = nullptr,
                                    size_t len = 0,
                                    ip_address* paddress = nullptr);

detail::channel& get_channel(detail::socket_internals* pimpl,
                             uint64_t id);
bool get_channel_safe(detail::channel* &pchannel,
    detail::socket_internals* pimpl,
    uint64_t id);
void delete_channel(detail::socket_internals* pimpl,
                    uint64_t current_id);
ip_address get_socket_bundle(native::socket_handle const& socket_descriptor);

}
/*
 * socket
 */
socket::socket(event_handler& eh,
               detail::fptr_message_loader _fmessage_loader,
               beltpp::void_unique_ptr&& putl)
    : isocket(eh)
    , m_pimpl(new detail::socket_internals(eh,
                                           _fmessage_loader,
                                           std::move(putl)))
{
}

socket::socket(socket&&) = default;

socket::~socket()
{
    if (m_pimpl)    //  check if the socket has been moved out from
    {
        for (auto& channels_item : m_pimpl->m_lst_channels)
        for (auto& channel_data : channels_item)
        {
            if (channel_data.m_closed)
                continue;
            if (0 != channel_data.m_socket_descriptor.reset())
            {
                assert(false);
                std::terminate();
            }
        }
    }
}

peer_ids socket::listen(ip_address const& address, int backlog/* = 100*/)
{
    peer_ids peers;
    string error_message;

    ptr_addrinfo ptr_servinfo(nullptr, [](addrinfo*){});

    detail::getaddressinfo(ptr_servinfo,
                           address.ip_type,
                           address.local.address,
                           address.local.port);

    sockets arr_sockets = detail::socket(ptr_servinfo.get(),
                                         true,    //  bind
                                         true,    //  reuse
                                         true);   //  non blocking

    for (auto& socket_info : arr_sockets)
    {
        native::socket_handle socket_descriptor = std::move(socket_info.first);
        addrinfo* pinfo = socket_info.second;

        assert(pinfo);
        string str_temp_address = detail::dump(*pinfo->ai_addr);

        {
            int res = ::listen(socket_descriptor.handle, backlog);
            if (-1 == res)
            {
                string listen_error = native::net_last_error();
                if (false == error_message.empty())
                    error_message += "; ";
                error_message += str_temp_address +
                        " - " +
                        listen_error;
                continue;
            }
        }

        peers.emplace_back(
                    detail::add_channel(*this,
                                        m_pimpl.get(),
                                        std::move(socket_descriptor),
                                        detail::channel::type::listening,
                                        0));
    }

    if (peers.empty() &&
        false == error_message.empty())
        throw std::runtime_error("listen(): " + error_message);

    return peers;
}

peer_ids socket::open(ip_address address, size_t attempts/* = 0*/)
{
    peer_ids peers;

    if (address.remote.empty())
    {
        address.remote = address.local;
        address.local = ip_destination();
    }

    ptr_addrinfo ptr_localinfo(nullptr, [](addrinfo*){});

    bool bind = true;// (false == address.local.empty());
    //  setting bind to true always, for connectex on windows
    //  does not seem to have other side effects
    //
    //  even if address.local is not set, the socket will
    //  be bound to an arbitraty port

    detail::getaddressinfo(ptr_localinfo,
                           address.ip_type,
                           address.local.address,
                           address.local.port);

    if (attempts == 0)
        attempts = 1;

    sockets arr_sockets = detail::socket(ptr_localinfo.get(),
                                         bind,    //  bind, always true
                                         true,    //  reuse
                                         true);   //  non blocking

    for (auto& socket_info : arr_sockets)
    {
        native::socket_handle socket_descriptor = std::move(socket_info.first);
        addrinfo* pinfo = socket_info.second;

        string str_temp_address = detail::dump(*pinfo->ai_addr);
        str_temp_address += "\t";

        bool hold_exception = false;
        if (address.ip_type == ip_address::e_type::any)
            hold_exception = true;

        ptr_addrinfo ptr_remoteinfo(nullptr, [](addrinfo*){});

        detail::getaddressinfo(ptr_remoteinfo,
                               pinfo->ai_family,
                               address.remote.address,
                               address.remote.port,
                               hold_exception);

        if (nullptr == ptr_remoteinfo)
            continue;
        str_temp_address += detail::dump(*ptr_remoteinfo->ai_addr);

        peers.emplace_back(
                    detail::add_channel(*this,
                                        m_pimpl.get(),
                                        std::move(socket_descriptor),
                                        detail::channel::type::streaming,
                                        attempts,
                                        ptr_remoteinfo->ai_addr,
                                        ptr_remoteinfo->ai_addrlen,
                                        &address));
    }

    return peers;
}

#ifdef B_OS_WINDOWS

void set_nonblocking(SOCKET socket_descriptor, bool option)
{
    u_long mode = 0; // disable nonblocking
    if (option)
        mode = 1;

    int res = ioctlsocket(socket_descriptor, FIONBIO, &mode);

    if (res != NO_ERROR)
        throw std::runtime_error("ioctlsocket failed with error: " + res);
}

#else

void set_nonblocking(int socket_descriptor, bool option)
{
    //  seems the initial state of socket is not
    //  the same across OSes
    //  non blocking listen accepts non blocking connections
    //  on macos, while on linux need to set non blocking manually
    //  so will force desired option always
    int flags = ::fcntl(socket_descriptor, F_GETFL, 0);
    if (false == option)
    {
        //  assert(flags & O_NONBLOCK);
        flags = flags ^ O_NONBLOCK;
    }
    else
    {
        //  assert(!(flags & O_NONBLOCK));
        flags = flags | O_NONBLOCK;
    }
    int res = ::fcntl(socket_descriptor, F_SETFL, flags);
    if (-1 == res)
    {
        string fcntl_error = native::net_last_error();
        throw std::runtime_error("setsockopt():F_SETFL " + fcntl_error);
    }
}

#endif

void socket::prepare_wait()
{
}

packets socket::receive(peer_id& peer)
{
    packets result;

    peer = peer_id();

    std::unordered_set<uint64_t> set_ids = m_pimpl->m_peh->waited(*this);

    for (uint64_t current_id : set_ids)
    {
        detail::channel* pchannel = nullptr;
        if (false == detail::get_channel_safe(pchannel, m_pimpl.get(), current_id))
            continue;
        detail::channel& current_channel = *pchannel;

        m_pimpl->m_peh->m_pimpl->reset(current_channel.m_eh_id);

        if (current_channel.m_attempts)
        {
            --current_channel.m_attempts;
            auto const& socket_descriptor = current_channel.m_socket_descriptor;

            e_three_state_result connect_result = e_three_state_result::success;
            string connect_error;
            
            int error_code = 0;
            bool res = false;
            if (current_channel.m_sync_result.is_set)
            {
                error_code = current_channel.m_sync_result.error_code;
                res = current_channel.m_sync_result.succeeded;
                current_channel.m_sync_result = native::sync_result();
            }
            else
            {
                res = native::get_connected_status(m_pimpl->m_peh->m_pimpl.get(),
                                                   current_channel.m_eh_id,
                                                   socket_descriptor.handle,
                                                   error_code);
            }

            if (native::check_connection_refused(res, error_code))
            {
                connect_result = e_three_state_result::attempt;
                connect_error = native::net_error(error_code);
            }
            else if (native::check_connected_error(res, error_code))
            {
                connect_result = e_three_state_result::error;
                connect_error = native::net_error(error_code);
            }
            else
                connect_result = e_three_state_result::success;

            if (connect_result == e_three_state_result::success)
            {
                m_pimpl->m_peh->m_pimpl->remove(socket_descriptor.handle,
                                                current_channel.m_eh_id,
                                                false,   //  already_closed
                                                event_handler::task::connect,
                                                true);

                current_channel.m_eh_id =
                        m_pimpl->m_peh->m_pimpl->add(*this,
                                                     socket_descriptor.handle,
                                                     current_id,
                                                     event_handler::task::receive,
                                                     true,
                                                     current_channel.m_eh_id);

                current_channel.m_attempts = 0;

                result.emplace_back(beltpp::isocket_join());

                peer = detail::construct_peer_id(current_id,
                                                 current_channel.m_socket_bundle);

                break;
            }
            else if (connect_result == e_three_state_result::attempt)
            {
                auto attempts = current_channel.m_attempts;
                auto local_peer = detail::construct_peer_id(current_id,
                                                            current_channel.m_socket_bundle);

                detail::delete_channel(m_pimpl.get(), current_id);

                if (attempts)
                    open(current_channel.m_socket_bundle, attempts);
                else
                {
                    result.emplace_back(beltpp::isocket_open_refused(connect_error, current_channel.m_socket_bundle));
                    peer = local_peer;
                    break;
                }
            }
            else if (connect_result == e_three_state_result::error)
            {
                detail::delete_channel(m_pimpl.get(), current_id);

                result.emplace_back(beltpp::isocket_open_error(connect_error, current_channel.m_socket_bundle));
                peer = detail::construct_peer_id(current_id,
                                                 current_channel.m_socket_bundle);
                break;
            }
        }
        else if (current_channel.m_type == detail::channel::type::listening)
        {
            int error_code = 0;
            bool res = 0;

            native::socket_handle joined_socket_descriptor(
                    native::accept(m_pimpl->m_peh->m_pimpl.get(),
                                   current_channel.m_eh_id,
                                   current_channel.m_socket_descriptor.handle,
                                   res,
                                   error_code)
                        );

            if (native::check_accepted_skip(res, error_code))
            {
                assert(joined_socket_descriptor.is_invalid());
                continue;
            }
            else if (native::check_connected_error(res, error_code))
            {
                assert(joined_socket_descriptor.is_invalid());
                string accept_error = native::net_error(error_code);
                throw std::runtime_error("accept(): " + accept_error);
            }
            else if (joined_socket_descriptor.is_invalid())
            {
                string accept_error = native::net_last_error();
                throw std::runtime_error("accept(): " + accept_error);
            }

            set_nonblocking(joined_socket_descriptor.handle, true);

            ip_address socket_bundle_temp = detail::get_socket_bundle(joined_socket_descriptor);
            assert(false == socket_bundle_temp.remote.empty());

            peer = detail::add_channel(*this,
                                       m_pimpl.get(),
                                       std::move(joined_socket_descriptor),
                                       detail::channel::type::streaming,
                                       0);

            result.emplace_back(beltpp::isocket_join());
            break;
        }
        else// if(current_channel.m_type == detail::channel::type::streaming)
        {
            current_channel.m_stream.reserve();
            char* p_buffer = nullptr;
            size_t size_buffer = 0;
            current_channel.m_stream.get_free_range(p_buffer, size_buffer);

            auto const& socket_descriptor = current_channel.m_socket_descriptor;

            int error_code = 0;
            size_t res = native::recv(m_pimpl->m_peh->m_pimpl.get(),
                                      current_channel.m_eh_id,
                                      socket_descriptor.handle,
                                      p_buffer,
                                      size_buffer,
                                      error_code);

            if (native::check_recv_block(res, error_code))
                continue;

            if (native::check_recv_connect(res, error_code))
                res = 0;

            if (native::check_recv_fail(res))
            {
                string recv_error = native::net_error(error_code);
                throw std::runtime_error("recv(): " +
                                         recv_error);
            }
            else if (0 == res)
            {
                peer = detail::construct_peer_id(current_id,
                                                 current_channel.m_socket_bundle);

                detail::delete_channel(m_pimpl.get(), current_id);

                result.emplace_back(beltpp::isocket_drop());
                break;
            }
            else
            {
                //  actually the stream here already has all new
                //  data read from socket. only need to push, to
                //  let the container know about it
                for (size_t index = 0; index < res; ++index)
                    current_channel.m_stream.push(p_buffer[index]);

                while (true &&
                       false == current_channel.m_stream.empty())
                {
                    auto const& stm = current_channel.m_stream;
                    beltpp::iterator_wrapper<char const> it_begin = stm.cbegin();
                    auto pmsgall = m_pimpl->m_fmessage_loader(it_begin,
                                                              stm.cend(),
                                                              current_channel.m_special_data,
                                                              m_pimpl->m_putl.get());

                    string buf;
                    if (pmsgall.rtt == size_t(-2))
                        buf.assign(stm.cbegin(), stm.cend());

                    while (false == current_channel.m_stream.empty() &&
                           beltpp::iterator_wrapper<char const>(stm.cbegin()) !=
                           it_begin)
                        current_channel.m_stream.pop();

                    if (pmsgall.rtt == size_t(-2))
                        result.emplace_back(beltpp::isocket_protocol_error(buf));
                    else if (pmsgall.pmsg)
                    {
                        packet pack;
                        pack.set(pmsgall.rtt,
                                std::move(pmsgall.pmsg),
                                pmsgall.fsaver);

                        result.emplace_back(std::move(pack));
                    }
                    else
                        break;
                }
               
                if (false == result.empty())
                {
                    peer = detail::construct_peer_id(current_id,
                                                     current_channel.m_socket_bundle);
                    break;
                }
            }
        }
    }

    return result;
}

void socket::send(peer_id const& peer, packet&& pack)
{
    uint64_t current_id = detail::parse_peer_id(peer);
    if (pack.type() == beltpp::isocket_drop::rtt)
    {
        detail::delete_channel(m_pimpl.get(), current_id);
    }
    else
    {
        detail::channel& current_channel =
                detail::get_channel(m_pimpl.get(),
                                    current_id);

        if (current_channel.m_type != detail::channel::type::streaming)
            throw std::runtime_error("send message on non streaming channel");
        {
            auto lambda_send = [](native::socket_handle const& sd,
                                  string const& ms)
            {
                size_t sent = 0;
                size_t length = ms.size();

                while (sent < length)
                {
                    size_t res = native::send(sd.handle,
                                              &ms[sent],
                                              ms.size() - sent);
                    //  when sending to socket closed by the peer
                    //  we have res = -1 and errno set to EPIPE

                    if (size_t(-1) == res)
                    {
                        string send_error = native::net_last_error();
                        throw std::runtime_error("send(): " +
                                                 send_error);
                    }
                    else
                        sent += res;
                }
            };

            auto const& socket_descriptor = current_channel.m_socket_descriptor;

            auto& sp_data = current_channel.m_special_data;
            auto fp = sp_data.session_specal_handler;

            if (fp)
            {
                string ms = fp(sp_data, pack);
                lambda_send(socket_descriptor, ms);
                assert(nullptr == sp_data.session_specal_handler);
            }
            else
            {
                string message_stream = pack.to_string();
                if (message_stream.empty())
                    throw std::runtime_error("send empty message");
                lambda_send(socket_descriptor, message_stream);
            }
        }
    }
}

void socket::timer_action()
{
}

ip_address socket::info(peer_id const& peer) const
{
    uint64_t current_id = detail::parse_peer_id(peer);
    detail::channel& current_channel =
            detail::get_channel(m_pimpl.get(),
                                current_id);

    return current_channel.m_socket_bundle;
}

string socket::dump() const
{
    string result;

    for (auto const& channels_item : m_pimpl->m_lst_channels)
    {
        auto id = channels_item.begin_index();
        for (auto const& channel_data : channels_item)
        {
            if (channel_data.m_closed)
                continue;

            result += detail::construct_peer_id(id,
                                                channel_data.m_socket_bundle);
            result += "\n";
            ++id;
        }
    }

    return result;
}

namespace detail
{
ip_address dumpaddr(sockaddr &address);
//string dump(beltpp::address const& addr);

ip_address get_socket_bundle(native::socket_handle const& socket_descriptor)
{
    ip_address result;

    sockaddr_storage address;
    socklen_t size = sizeof(sockaddr_storage);
    memset(&address, 0, size);
    {
        int res = ::getsockname(socket_descriptor.handle,
                                reinterpret_cast<sockaddr*>(&address),
                                &size);
        if (-1 == res)
        {
            string gsn_error = native::net_last_error();
            throw std::runtime_error("getsockname(): " + gsn_error);
        }
    }
    result = detail::dumpaddr(reinterpret_cast<sockaddr&>(address));

    {
        int res = ::getpeername(socket_descriptor.handle,
                                reinterpret_cast<sockaddr*>(&address),
                                &size);
        if (0 == res)
            result.remote = detail::dumpaddr(reinterpret_cast<sockaddr&>(address)).local;
        else
        {
            int errorno = errno;
            string gpn_error = native::net_last_error();
            //
            ++errorno;
        }
    }

    return result;
}

string construct_peer_id(uint64_t id, ip_address const& socket_bundle)
{
    return std::to_string(id) + "<=>" + socket_bundle.to_string();
}

uint64_t parse_peer_id(string const& peer_id)
{
    size_t pos = peer_id.find("<=>");
    if (string::npos == pos)
        throw std::runtime_error("parse_peer_id");

    size_t parse_end;
    uint64_t id = beltpp::stoui64(peer_id, parse_end);

    if (parse_end != pos)
        throw std::runtime_error("parse_peer_id");

    return id;
}

ip_address dumpaddr(sockaddr &address)
{
    ip_address addr;

    if (address.sa_family == AF_INET)
    {
        sockaddr_in* addressv4 = reinterpret_cast<sockaddr_in*>(&address);
        char ipstr[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(addressv4->sin_addr), ipstr, INET_ADDRSTRLEN);

        addr.local.port = ntohs(addressv4->sin_port);
        addr.local.address = ipstr;
        addr.ip_type = ip_address::e_type::ipv4;
    }
    else if (address.sa_family == AF_INET6)
    {
        sockaddr_in6* addressv6 = reinterpret_cast<sockaddr_in6*>(&address);
        char ipstr[INET6_ADDRSTRLEN];

        inet_ntop(AF_INET6, &(addressv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);

        addr.local.port = ntohs(addressv6->sin6_port);
        addr.local.address = ipstr;
        addr.ip_type = ip_address::e_type::ipv6;
    }
    else
    {
        assert(false);
        throw std::runtime_error("beltpp::address dump(sockaddr &address) "
                                 "not implemented");
    }

    return addr;
}

string dump(sockaddr& address)
{
    ip_address addr = dumpaddr(address);
    return addr.to_string();
}

void getaddressinfo(ptr_addrinfo& servinfo,
                    ip_address::e_type type,
                    string const& str_address,
                    unsigned short port,
                    bool hold_family_mismatch_exception/* = false*/)
{
    int ai_family;
    switch (type)
    {
    case ip_address::e_type::any:
        ai_family = AF_UNSPEC;
        break;
    case ip_address::e_type::ipv4:
        ai_family = AF_INET;
        break;
//    case ip_address::e_type::ipv6:
    default:
        ai_family = AF_INET6;
    }

    getaddressinfo(servinfo,
                   ai_family,
                   str_address,
                   port,
                   hold_family_mismatch_exception);
}

bool getaddressinfo_is_family_mismatch_error(int rv)
{
#ifdef B_OS_MACOS
    return (rv == EAI_ADDRFAMILY) || (rv == EAI_NONAME);
#elif defined B_OS_WINDOWS
    return (rv == WSANO_DATA) || (rv == WSAHOST_NOT_FOUND);
#else
    return (rv == EAI_ADDRFAMILY) || (rv == EAI_NONAME);
#endif
}

void getaddressinfo(ptr_addrinfo& ptr_servinfo,
                    int ai_family,
                    string const& str_address,
                    unsigned short port,
                    bool hold_family_mismatch_exception/* = false*/)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = ai_family;

    char const* sz_address = nullptr;
    if (str_address.empty())
        hints.ai_flags = AI_PASSIVE; // use localhost
    else
        sz_address = str_address.c_str();

    addrinfo* pservinfo = nullptr;
    int gai_rv = ::getaddrinfo(sz_address,
                               std::to_string(port).c_str(),
                               &hints,
                               &pservinfo);
    //
    //  throw any error, except EAI_ADDRFAMILY when
    //  hold_family_mismatch_exception is true
    if (gai_rv &&
        (false == getaddressinfo_is_family_mismatch_error(gai_rv) ||
         false == hold_family_mismatch_exception))
        throw std::runtime_error(native::gai_error(gai_rv));

    if (gai_rv) // in case of error that was not thrown
        ptr_servinfo = ptr_addrinfo(nullptr, [](addrinfo*){});
    else
    {
        assert(pservinfo);
        if (nullptr == pservinfo)
            throw std::runtime_error("assert(pservinfo);");
        ptr_servinfo = ptr_addrinfo(pservinfo, &freeaddrinfo);
    }
}

sockets socket(addrinfo* servinfo,
               bool bind,
               bool reuse,
               bool non_blocking)
{
    sockets result;

    assert(servinfo);
    if (nullptr == servinfo)
        throw std::runtime_error("assert(servinfo);");

    for (addrinfo* p = servinfo; p != nullptr; p = p->ai_next)
    {
        native::socket_handle socket_descriptor(native::socket(p->ai_family,
                                                               p->ai_socktype,
                                                               p->ai_protocol));

        if (socket_descriptor.is_invalid())
        {
            string socket_error = native::net_last_error();
            throw std::runtime_error("socket(): " + socket_error);
        }

        string str_temp_address = detail::dump(*p->ai_addr);

#ifdef BELT_USE_SO_NOSIGPIPE
        {
            int yes = 1;
            int res = ::setsockopt(socket_descriptor.handle,
                                   SOL_SOCKET,
                                   SO_NOSIGPIPE,
                                   &yes,
                                   sizeof(int));
            if (-1 == res)
            {
                string setsockopt_error = native::net_last_error();
                throw std::runtime_error("setsockopt(): " + setsockopt_error);
            }
        }
#endif

#ifndef B_OS_WINDOWS
        {   //  reuse address always makes sense for linux
            int yes = 1;
            int res = ::setsockopt(socket_descriptor.handle,
                                   SOL_SOCKET,
                                   SO_REUSEADDR,
                                   native::sockopttype(&yes),
                                   sizeof(int));
            if (-1 == res)
            {
                string setsockopt_error = native::net_last_error();
                throw std::runtime_error("setsockopt(): " + setsockopt_error);
            }
        }
#endif

        if (reuse)
        {
#ifdef B_OS_WINDOWS
            int yes = 1;
            int res = ::setsockopt(socket_descriptor.handle,
                SOL_SOCKET,
                SO_REUSEADDR,
                native::sockopttype(&yes),
                sizeof(int));

            if (res != NO_ERROR)
                throw std::runtime_error("setsockopt(): " + native::net_last_error());
#else
            int yes = 1;
            int res = ::setsockopt(socket_descriptor.handle,
                                   SOL_SOCKET,
                                   SO_REUSEPORT,
                                   native::sockopttype(&yes),
                                   sizeof(int));
            if (-1 == res)
            {
                string setsockopt_error = native::net_last_error();
                throw std::runtime_error("setsockopt(): " + setsockopt_error);
            }
#endif
        }
        else
        {
#ifdef B_OS_WINDOWS
            int yes = 1;
            int res = ::setsockopt(socket_descriptor.handle,
                SOL_SOCKET,
                SO_EXCLUSIVEADDRUSE,
                native::sockopttype(&yes),
                sizeof(int));

            if (res != NO_ERROR)
                throw std::runtime_error("setsockopt(): " + native::net_last_error());
#endif
        }

        if (non_blocking)
            set_nonblocking(socket_descriptor.handle, true);

        if (bind)
        {
            int res = native::bind(socket_descriptor.handle,
                                   p->ai_addr,
                                   p->ai_addrlen);
            if (-1 == res)
            {
                string bind_error = native::net_last_error();
                throw std::runtime_error("bind(): " + str_temp_address + ", " + bind_error);
            }
        }

        result.push_back(std::make_pair(std::move(socket_descriptor), p));
    }

    return result;
}

beltpp::socket::peer_id add_channel(beltpp::socket& self,
                                    detail::socket_internals* pimpl,
                                    native::socket_handle&& socket_descriptor,
                                    detail::channel::type e_type,
                                    size_t attempts,
                                    const struct sockaddr* addr/* = nullptr*/,
                                    size_t len/* = 0*/,
                                    ip_address* paddress/* = nullptr*/)
{
    std::lock_guard<std::mutex> lock(pimpl->m_mutex);

    auto& lst_channels = pimpl->m_lst_channels;
    if (lst_channels.empty())
        lst_channels.emplace_back(channels(16));
    auto it_channels = lst_channels.end();
    --it_channels;
    if (it_channels->full())
    {
        lst_channels.emplace_back(channels(it_channels->size() * 2,
                                           it_channels->end_index()));
        it_channels = lst_channels.end();
        --it_channels;
    }

    uint64_t id = it_channels->end_index();

    event_handler::task action = event_handler::task::accept;

    if (e_type != detail::channel::type::listening)
    {
        if (attempts == 0)
            action = event_handler::task::receive;
        else
            action = event_handler::task::connect;
    }

    auto native_handle = socket_descriptor.handle;
    uint64_t eh_id = pimpl->m_peh->m_pimpl->add(self,
                                                native_handle,
                                                id,
                                                action);

    beltpp::on_failure scope_guard(
    [pimpl, native_handle, eh_id, action]
    {
        pimpl->m_peh->m_pimpl->remove(native_handle,
                                      eh_id,
                                      false,  //  already_closed
                                      action);
    });

    ip_address socket_bundle;

    native::sync_result connect_result;

    if (action == event_handler::task::connect)
    {

        native::connect(pimpl->m_peh->m_pimpl.get(),
                        eh_id,
                        socket_descriptor.handle,
                        addr,
                        len,
                        *paddress,
                        connect_result);

        if (connect_result.is_set)
            pimpl->m_peh->m_pimpl->set_sync_result(eh_id);

        socket_bundle = detail::get_socket_bundle(socket_descriptor);

        if (socket_bundle.remote.empty())
            socket_bundle.remote = paddress->remote;

        assert(false == socket_bundle.remote.empty());
    }
    else
        socket_bundle = detail::get_socket_bundle(socket_descriptor);

    it_channels->push(detail::channel(attempts,
                                      std::move(socket_descriptor),
                                      e_type,
                                      socket_bundle,
                                      eh_id,
                                      connect_result));

    beltpp::socket::peer_id current_peer =
        detail::construct_peer_id(id, socket_bundle);

    scope_guard.dismiss();

    return current_peer;
}

detail::channel& get_channel(detail::socket_internals* pimpl, uint64_t current_id)
{
    std::lock_guard<std::mutex> lock(pimpl->m_mutex);

    for (auto& channels_item : pimpl->m_lst_channels)
    {
        if (channels_item.valid_index(current_id))
            return channels_item[current_id];
    }

    throw std::runtime_error("get_channel");
}

bool get_channel_safe(detail::channel* &pchannel,
                      detail::socket_internals* pimpl,
                      uint64_t current_id)
{
    bool code = false;
    std::lock_guard<std::mutex> lock(pimpl->m_mutex);

    for (auto& channels_item : pimpl->m_lst_channels)
    {
        if (channels_item.valid_index(current_id) &&
            channels_item[current_id].m_closed == false)
        {
            pchannel = &channels_item[current_id];
            code = true;
            break;
        }
    }

    return code;
}

void delete_channel(detail::socket_internals* pimpl, uint64_t current_id)
{
    std::lock_guard<std::mutex> lock(pimpl->m_mutex);

    auto it_channels = pimpl->m_lst_channels.begin();
    for (; it_channels != pimpl->m_lst_channels.end(); ++it_channels)
    {
        if (it_channels->valid_index(current_id))
        {
            detail::channel& current_channel =
                    it_channels->operator [](current_id);

            if (current_channel.m_closed)
            {
                assert(false);
                throw std::runtime_error("delete_channel");
            }

            bool close_succeeded = true;
            if (0 != current_channel.m_socket_descriptor.reset())
            {
                close_succeeded = false;
                assert(false);
            }

            current_channel.m_closed = true;

            event_handler::task action = event_handler::task::accept;
            if (current_channel.m_type == detail::channel::type::streaming)
                action = event_handler::task::receive;

            pimpl->m_peh->m_pimpl->remove(current_channel.m_socket_descriptor.handle,
                                          current_channel.m_eh_id,
                                          true,   //  already_closed
                                          action);

            while (false == it_channels->empty())
            {
                if (false == it_channels->front().m_closed)
                    break;
                it_channels->pop();
            }

            auto it_channels_check = it_channels;
            ++it_channels_check;
            if (it_channels->empty() &&
                it_channels_check != pimpl->m_lst_channels.end())
                pimpl->m_lst_channels.erase(it_channels);

            if (false == close_succeeded)
                throw std::runtime_error("::close unsuccessful");
            return;
        }
    }

    //  this will terminate
    //      not sure why this is not noexcept considering the above comment
    assert(false);
    throw std::runtime_error("delete_channel");
}

}// end detail

}// end beltpp
