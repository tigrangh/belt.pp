#include "socket.hpp"

#include <scope_helper.hpp>
#include <queue.hpp>
#include <message.hpp>

#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//  sys/types seems to be unnecessary on modern systems
#include <sys/types.h>

//  close the file descriptor
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <unordered_map>
#include <unordered_set>
#include <cassert>
#include <utility>
#include <mutex>
#include <memory>
#include <list>
//
using string = std::string;
using sockets = std::vector<std::pair<int, addrinfo*>>;
using peer_ids = beltpp::socket::peer_ids;
using messages = beltpp::socket::messages;

namespace beltpp
{
address::address(std::string const& aaddress, unsigned short port)
    : m_address(aaddress)
    , m_port(port)
{}
bool address::empty() const noexcept
{
    return (0 == m_port) && m_address.empty();
}
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

    channel(int socket_descriptor = 0,
            type e_type = type::listening)
        : m_closed(false)
        , m_socket_descriptor(socket_descriptor)
        , m_type(e_type)
    {}

    bool m_closed;
    int m_socket_descriptor;
    type m_type;
    beltpp::queue<char> m_stream;
};
using channels = beltpp::queue<channel>;

class poll_master
{
public:
    poll_master()
    {
        m_fd = epoll_create(1);
        if (-1 == m_fd)
        {
            string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_create(): " +
                                     epoll_error);
        }

        m_event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    }

    ~poll_master() {}

    void add(int socket_descriptor, uint64_t id)
    {
        m_event.data.fd = socket_descriptor;
        m_event.data.u64 = id;

        m_arr_event.resize(m_arr_event.size() + 1);

        int res = epoll_ctl(m_fd, EPOLL_CTL_ADD, socket_descriptor, &m_event);
        if (-1 == res)
        {
            m_arr_event.resize(m_arr_event.size() - 1);
            string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_ctl(): " +
                                     epoll_error);
        }
    }

    void remove(int socket_descriptor, uint64_t id)
    {
        m_event.data.fd = socket_descriptor;
        m_event.data.u64 = id;

        int res = epoll_ctl(m_fd, EPOLL_CTL_DEL, socket_descriptor, &m_event);
        if (-1 == res)
        {
            string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_ctl(): " +
                                     epoll_error);
        }

        m_arr_event.resize(m_arr_event.size() - 1);
    }

    std::unordered_set<uint64_t> wait()
    {
        std::unordered_set<uint64_t> set_ids;
        //while (true)
        {
            int count = -1;
            do
            {
                count = epoll_wait(m_fd,
                                   &m_arr_event[0],
                                   m_arr_event.size(),
                                   -1);
            } while (-1 == count && errno == EINTR);

            if (-1 == count)
            {
                string epoll_error = strerror(errno);
                throw std::runtime_error("epoll_wait(): " +
                                         epoll_error);
            }

            // for each ready socket
            for(int i = 0; i < count; ++i)
            {
                uint64_t id = m_arr_event[i].data.u64;
                set_ids.insert(id);
            }

            //break;
        }

        return set_ids;
    }

    int m_fd;
    epoll_event m_event;
    std::vector<epoll_event> m_arr_event;
};

class socket_internals
{
public:
    socket_internals(size_t rtt_join,
                     size_t rtt_drop,
                     detail::fptr_creator fcreator_join,
                     detail::fptr_creator fcreator_drop,
                     detail::fptr_saver fsaver_join,
                     detail::fptr_saver fsaver_drop,
                     detail::fptr_message_loader fmessage_loader)
        : m_rtt_join(rtt_join)
        , m_rtt_drop(rtt_drop)
        , m_fcreator_join(fcreator_join)
        , m_fcreator_drop(fcreator_drop)
        , m_fsaver_join(fsaver_join)
        , m_fsaver_drop(fsaver_drop)
        , m_fmessage_loader(fmessage_loader)
    {}

    size_t m_rtt_join;
    size_t m_rtt_drop;
    detail::fptr_creator m_fcreator_join;
    detail::fptr_creator m_fcreator_drop;
    detail::fptr_saver m_fsaver_join;
    detail::fptr_saver m_fsaver_drop;
    detail::fptr_message_loader m_fmessage_loader;
    std::list<channels> m_lst_channels;
    poll_master m_poll_master;
    std::mutex m_mutex;
};

string construct_peer_id(uint64_t id, int socket_descriptor);
uint64_t parse_peer_id(string const& peer_id);
string dump(sockaddr& address);
void getaddressinfo(addrinfo* &servinfo,
                    beltpp::socket::socketv version,
                    string const& str_address,
                    unsigned short port,
                    beltpp::scope_helper& servinfo_cleaner);
void getaddressinfo(addrinfo* &servinfo,
                    int ai_family,
                    string const& str_address,
                    unsigned short port,
                    beltpp::scope_helper& servinfo_cleaner);
sockets socket(addrinfo* servinfo,
               bool bind,
               bool reuse);
beltpp::socket::peer_id add_channel(detail::socket_internals* pimpl,
                                    int socket_descriptor,
                                    detail::channel::type e_type);
detail::channel& get_channel(detail::socket_internals* pimpl,
                             uint64_t id);
void delete_channel(detail::socket_internals* pimpl,
                    uint64_t current_id);
beltpp::socket::peer_info info(int socket_descriptor);

}
/*
 * socket
 */
socket::socket(size_t _rtt_join,
               size_t _rtt_drop,
               detail::fptr_creator _fcreator_join,
               detail::fptr_creator _fcreator_drop,
               detail::fptr_saver _fsaver_join,
               detail::fptr_saver _fsaver_drop,
               detail::fptr_message_loader _fmessage_loader)
    : isocket()
    , m_pimpl(new detail::socket_internals(_rtt_join,
                                           _rtt_drop,
                                           _fcreator_join,
                                           _fcreator_drop,
                                           _fsaver_join,
                                           _fsaver_drop,
                                           _fmessage_loader))
{

}

socket::socket(socket&&) = default;

socket::~socket()
{
    for (auto const& channels_item : m_pimpl->m_lst_channels)
    for (auto const& channel_data : channels_item)
    {
        ::close(channel_data.m_socket_descriptor);
    }
}

peer_ids socket::listen(address const& local_address,
                        socketv version/* = socketv::any*/,
                        int backlog/* = 100*/)
{
    socket::peer_ids peers;
    string error_message;

    addrinfo* servinfo = nullptr;
    beltpp::scope_helper servinfo_cleaner;

    detail::getaddressinfo(servinfo,
                           version,
                           local_address.m_address,
                           local_address.m_port,
                           servinfo_cleaner);

    sockets arr_sockets = detail::socket(servinfo,
                                         true,    //  bind
                                         true);   //  reuse

    for (auto const& socket_info : arr_sockets)
    {
        assert(socket_info.second);
        string str_temp_address = detail::dump(*socket_info.second->ai_addr);

        {
            int res = ::listen(socket_info.first, backlog);
            if (-1 == res)
            {
                string listen_error = strerror(errno);
                ::close(socket_info.first);
                if (false == error_message.empty())
                    error_message += "; ";
                error_message += str_temp_address +
                        " - " +
                        listen_error;
                continue;
            }
        }

        peers.emplace_back(
                    detail::add_channel(m_pimpl.get(),
                                        socket_info.first,
                                        detail::channel::type::listening));
    }

    if (peers.empty() &&
        false == error_message.empty())
        throw std::runtime_error("listen(): " + error_message);

    return peers;
}

peer_ids socket::open(address const& local_address,
                      address const& remote_address,
                      socketv version/* = socketv::any*/)
{
    socket::peer_ids peers;
    string error_message;

    addrinfo* localinfo = nullptr;
    beltpp::scope_helper localinfo_cleaner;

    bool bind = (0 != local_address.m_port);

    detail::getaddressinfo(localinfo,
                           version,
                           local_address.m_address,
                           local_address.m_port,
                           localinfo_cleaner);

    sockets arr_sockets = detail::socket(localinfo,
                                         bind,    //  bind
                                         true);   //  reuse

    for (auto const& socket_info : arr_sockets)
    {
        string str_temp_address = detail::dump(*socket_info.second->ai_addr);
        str_temp_address += "\t";

        addrinfo* remoteinfo = nullptr;
        beltpp::scope_helper remoteinfo_cleaner;
        detail::getaddressinfo(remoteinfo,
                               socket_info.second->ai_family,
                               remote_address.m_address,
                               remote_address.m_port,
                               remoteinfo_cleaner);

        assert(remoteinfo);
        str_temp_address += detail::dump(*remoteinfo->ai_addr);

        {
            int res = ::connect(socket_info.first,
                                remoteinfo->ai_addr,
                                remoteinfo->ai_addrlen);
            if (-1 == res)
            {
                string connect_error = strerror(errno);
                ::close(socket_info.first);

                if (errno == ECONNREFUSED)
                    continue;

                if (false == error_message.empty())
                    error_message += "; ";
                error_message += str_temp_address +
                        " - " +
                        connect_error;
                continue;
            }
        }

        peers.emplace_back(
                    detail::add_channel(m_pimpl.get(),
                                        socket_info.first,
                                        detail::channel::type::streaming));
    }

    if (peers.empty() &&
        false == error_message.empty())
        throw std::runtime_error("connect(): " + error_message);

    return peers;
}

messages socket::read(peer_id& peer)
{
    messages result;

    peer = peer_id();

    std::unordered_set<uint64_t> set_ids =
            m_pimpl->m_poll_master.wait();

    for (uint64_t current_id : set_ids)
    {
        detail::channel& current_channel =
                detail::get_channel(m_pimpl.get(),
                                    current_id);

        if (current_channel.m_type == detail::channel::type::listening)
        {
            sockaddr_storage address;
            socklen_t size = sizeof(sockaddr_storage);
            int joined_socket_descriptor =
                    accept(current_channel.m_socket_descriptor,
                           reinterpret_cast<sockaddr*>(&address),
                           &size);

            if (-1 == joined_socket_descriptor)
            {
                string accept_error = strerror(errno);
                throw std::runtime_error("accept(): " +
                                         accept_error);
            }

            peer = detail::add_channel(m_pimpl.get(),
                                       joined_socket_descriptor,
                                       detail::channel::type::streaming);

            message msg;

            msg.set(m_pimpl->m_rtt_join,
                    m_pimpl->m_fcreator_join(),
                    m_pimpl->m_fsaver_join);

            result.push_back(std::move(msg));
            break;
        }
        else// if(current_channel.m_type == detail::channel::type::streaming)
        {
            current_channel.m_stream.reserve();
            char* p_buffer = nullptr;
            size_t size_buffer = 0;
            current_channel.m_stream.get_free_range(p_buffer, size_buffer);

            auto socket_descriptor = current_channel.m_socket_descriptor;
            int res = recv(socket_descriptor,
                           p_buffer,
                           size_buffer,
                           0);

            if (-1 == res)
            {
                string recv_error = strerror(errno);
                throw std::runtime_error("recv(): " +
                                         recv_error);
            }
            else if (0 == res)
            {
                peer = detail::construct_peer_id(current_id,
                                                 socket_descriptor);
                message msg;
                msg.set(m_pimpl->m_rtt_drop,
                        m_pimpl->m_fcreator_drop(),
                        m_pimpl->m_fsaver_drop);

                detail::delete_channel(m_pimpl.get(), current_id);

                result.push_back(std::move(msg));
                break;
            }
            else
            {
                //  actually the stream here already has all new
                //  data read from socket. only need to push, to
                //  let the container know about it
                for (int index = 0; index < res; ++index)
                    current_channel.m_stream.push(p_buffer[index]);

                while (true &&
                       false == current_channel.m_stream.empty())
                {
                    size_t clean_count = 0;
                    auto const& stm = current_channel.m_stream;
                    auto pmsgall = m_pimpl->m_fmessage_loader(stm.cbegin(),
                                                              stm.cend(),
                                                              clean_count);

                    while (false == current_channel.m_stream.empty() &&
                           clean_count > 0)
                    {
                        --clean_count;
                        current_channel.m_stream.pop();
                    }

                    if (pmsgall.pmsg)
                    {
                        message msg;
                        msg.set(pmsgall.rtt,
                                std::move(pmsgall.pmsg),
                                pmsgall.fsaver);

                        result.push_back(std::move(msg));
                    }
                    else
                        break;
                }

                if (false == result.empty())
                {
                    peer = detail::construct_peer_id(current_id,
                                                     socket_descriptor);
                    break;
                }
            }
        }
    }

    return result;
}

void socket::write(peer_id const& peer,
                   message const& msg)
{
    uint64_t current_id = detail::parse_peer_id(peer);
    if (msg.type() == m_pimpl->m_rtt_drop)
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
            std::vector<char> message_stream = msg.save();
            if (message_stream.empty())
                throw std::runtime_error("send empty message");

            auto socket_descriptor = current_channel.m_socket_descriptor;
            size_t message_len = message_stream.size();
            size_t sent = 0;
            while (sent < message_len)
            {
                int res = send(socket_descriptor,
                               &message_stream[sent],
                               message_stream.size() - sent,
                               0/*MSG_NOSIGNAL*/);
                //  need to test when sending to socket closed by the peer

                if (-1 == res)
                {
                    string send_error = strerror(errno);
                    throw std::runtime_error("send(): " +
                                             send_error);
                }
                else
                    sent += res;
            }
        }
    }
}

socket::peer_info socket::info(peer_id const& peer) const
{
    uint64_t current_id = detail::parse_peer_id(peer);
    detail::channel& current_channel =
            detail::get_channel(m_pimpl.get(),
                                current_id);

    return detail::info(current_channel.m_socket_descriptor);
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
                                                channel_data.m_socket_descriptor);
            result += "\n";
            ++id;
        }
    }

    return result;
}

namespace detail
{
beltpp::address dumpaddr(sockaddr &address);
string dump(beltpp::address const& addr);

beltpp::socket::peer_info info(int socket_descriptor)
{
    beltpp::socket::peer_info result;

    sockaddr_storage address;
    socklen_t size = sizeof(sockaddr_storage);
    memset(&address, 0, size);
    {
        int res = ::getsockname(socket_descriptor,
                                reinterpret_cast<sockaddr*>(&address),
                                &size);
        if (-1 == res)
        {
            string gpn_error = strerror(errno);
            throw std::runtime_error("getsockname(): " + gpn_error);
        }
    }
    result.local = detail::dumpaddr(reinterpret_cast<sockaddr&>(address));

    {
        int res = ::getpeername(socket_descriptor,
                                reinterpret_cast<sockaddr*>(&address),
                                &size);
        if (0 == res)
            result.remote = detail::dumpaddr(reinterpret_cast<sockaddr&>(address));
    }

    return result;
}

string construct_peer_id(uint64_t id,
                         int socket_descriptor)
{
    string result(std::to_string(id));

    beltpp::socket::peer_info pi = info(socket_descriptor);

    result += "<=>";
    result += dump(pi.local);

    if (false == pi.remote.empty())
    {
        result += "<=>";
        result += dump(pi.remote);
    }

    return result;
}

uint64_t parse_peer_id(string const& peer_id)
{
    size_t pos = peer_id.find("<=>");
    if (string::npos == pos)
        throw std::runtime_error("parse_peer_id");

    size_t parse_end;
    uint64_t id = std::stoll(peer_id, &parse_end);

    if (parse_end != pos)
        throw std::runtime_error("parse_peer_id");

    return id;
}

beltpp::address dumpaddr(sockaddr &address)
{
    beltpp::address addr;

    if (address.sa_family == AF_INET)
    {
        sockaddr_in* addressv4 = reinterpret_cast<sockaddr_in*>(&address);
        char ipstr[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(addressv4->sin_addr), ipstr, INET_ADDRSTRLEN);

        addr.m_port = ntohs(addressv4->sin_port);
        addr.m_address = ipstr;
    }
    else if (address.sa_family == AF_INET6)
    {
        sockaddr_in6* addressv6 = reinterpret_cast<sockaddr_in6*>(&address);
        char ipstr[INET6_ADDRSTRLEN];

        inet_ntop(AF_INET6, &(addressv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);

        addr.m_port = ntohs(addressv6->sin6_port);
        addr.m_address = ipstr;
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
    beltpp::address addr = dumpaddr(address);
    return dump(addr);
}

string dump(beltpp::address const& addr)
{
    if (string::npos != addr.m_address.find(':'))
        return "[" + addr.m_address + "]:" + std::to_string(addr.m_port);
    else
        return addr.m_address + ":" + std::to_string(addr.m_port);
}

void getaddressinfo(addrinfo* &servinfo,
                    beltpp::socket::socketv version,
                    string const& str_address,
                    unsigned short port,
                    beltpp::scope_helper& servinfo_cleaner)
{
    int ai_family;
    switch (version)
    {
    case beltpp::socket::socketv::any:
        ai_family = AF_UNSPEC;
        break;
    case beltpp::socket::socketv::ipv4:
        ai_family = AF_INET;
        break;
    case beltpp::socket::socketv::ipv6:
    default:
        ai_family = AF_INET6;
    }

    getaddressinfo(servinfo,
                   ai_family,
                   str_address,
                   port,
                   servinfo_cleaner);
}

void getaddressinfo(addrinfo* &servinfo,
                    int ai_family,
                    string const& str_address,
                    unsigned short port,
                    beltpp::scope_helper& servinfo_cleaner)
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

    int gai_rv = ::getaddrinfo(sz_address,
                               std::to_string(port).c_str(),
                               &hints,
                               &servinfo);
    if (gai_rv)
        throw std::runtime_error(gai_strerror(gai_rv));

    assert(servinfo);
    if (nullptr == servinfo)
        throw std::runtime_error("assert(servinfo);");

    servinfo_cleaner = beltpp::scope_helper([]{}, [&servinfo]{freeaddrinfo(servinfo);});
}

sockets socket(addrinfo* servinfo,
               bool bind,
               bool reuse)
{
    sockets result;

    assert(servinfo);
    if (nullptr == servinfo)
        throw std::runtime_error("assert(servinfo);");

    for (addrinfo* p = servinfo; p != nullptr; p = p->ai_next)
    {
        int socket_descriptor = ::socket(p->ai_family,
                                         p->ai_socktype,
                                         p->ai_protocol);
        if (-1 == socket_descriptor)
        {
            string socket_error = strerror(errno);
            throw std::runtime_error("socket(): " + socket_error);
        }

        string str_temp_address = detail::dump(*p->ai_addr);

        {   //  reuse address always makes sense for linux
            int yes = 1;
            int res = ::setsockopt(socket_descriptor,
                                   SOL_SOCKET,
                                   SO_REUSEADDR,
                                   &yes,
                                   sizeof(int));
            if (-1 == res)
            {
                string setsockopt_error = strerror(errno);
                throw std::runtime_error("setsockopt(): " + setsockopt_error);
            }
        }

        if (reuse)
        {   //  for windows this should actually be reuse address
            //  for windows exclusive flags should make sense otherwise
            int yes = 1;
            int res = ::setsockopt(socket_descriptor,
                                   SOL_SOCKET,
                                   SO_REUSEPORT,
                                   &yes,
                                   sizeof(int));
            if (-1 == res)
            {
                string setsockopt_error = strerror(errno);
                throw std::runtime_error("setsockopt(): " + setsockopt_error);
            }
        }

        if (bind)
        {
            int res = ::bind(socket_descriptor,
                             p->ai_addr,
                             p->ai_addrlen);
            if (-1 == res)
            {
                string bind_error = strerror(errno);
                ::close(socket_descriptor);
                throw std::runtime_error("bind(): " + str_temp_address + ", " + bind_error);
            }
        }

        result.push_back(std::make_pair(socket_descriptor, p));
        //break;
    }

    return result;
}

beltpp::socket::peer_id add_channel(detail::socket_internals* pimpl,
                                    int socket_descriptor,
                                    detail::channel::type e_type)
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

    beltpp::socket::peer_id current_peer =
            detail::construct_peer_id(id, socket_descriptor);

    pimpl->m_poll_master.add(socket_descriptor, id);

    beltpp::scope_helper scope_guard([]{},
    [pimpl, socket_descriptor, id]
    {
        pimpl->m_poll_master.remove(socket_descriptor, id);
    });

    it_channels->push(detail::channel(socket_descriptor, e_type));
    scope_guard.commit();

    return current_peer;
}

detail::channel& get_channel(detail::socket_internals* pimpl,
                             uint64_t current_id)
{
    std::lock_guard<std::mutex> lock(pimpl->m_mutex);

    for (auto& channels_item : pimpl->m_lst_channels)
    {
        if (channels_item.valid_index(current_id))
            return channels_item[current_id];
    }

    throw std::runtime_error("get_channel");
}

void delete_channel(detail::socket_internals* pimpl,
                    uint64_t current_id)
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

            ::close(current_channel.m_socket_descriptor);
            current_channel.m_closed = true;

            //  explicit remove here gives error
            //  apparently it's already removed
            /*pimpl->m_poll_master.remove(current_channel.m_socket_descriptor,
                                        current_id);*/

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


