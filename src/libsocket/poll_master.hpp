#pragma once

#include "global.hpp"

#ifdef B_OS_WINDOWS
#include <MSWSock.h>
#else// B_OS_WINDOWS
#include <unistd.h>
#endif

#include "native.hpp"

#include <belt.pp/timer.hpp>

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cerrno>
#include <cstring>
#include <vector>
#include <cassert>
#include <memory>

namespace chrono = std::chrono;
using steady_clock = chrono::steady_clock;
using time_point = steady_clock::time_point;
using duration = steady_clock::duration;

#if defined B_OS_LINUX

#include <sys/epoll.h>
namespace beltpp
{
namespace detail
{
class poll_master
{
public:
    poll_master()
    {
        m_fd = ::epoll_create(1);
        if (-1 == m_fd)
        {
            std::string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_create(): " +
                                     epoll_error);
        }

        m_event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP;
    }

    ~poll_master()
    {
        ::close(m_fd);
    }

    void add(int socket_descriptor, uint64_t id, event_handler::task action)
    {
        assert(event_handler::task::remove != action);
        m_event.data.u64 = id;

        auto backup = m_event.events;
        if (action == event_handler::task::connect ||
            action == event_handler::task::send)    //  win version cares for receive case too
            m_event.events |= EPOLLOUT;

        m_arr_event.resize(m_arr_event.size() + 1);

        int res = ::epoll_ctl(m_fd, EPOLL_CTL_ADD, socket_descriptor, &m_event);
        m_event.events = backup;

        if (-1 == res)
        {
            m_arr_event.resize(m_arr_event.size() - 1);
            std::string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_ctl(): " +
                                     epoll_error);
        }
    }

    void remove(int socket_descriptor, uint64_t id, bool already_closed, event_handler::task)  //  last argument is used for mac os version
    {
        m_event.data.u64 = id;

        if (false == already_closed)
        {
            int res = ::epoll_ctl(m_fd, EPOLL_CTL_DEL, socket_descriptor, &m_event);
            if (-1 == res)
            {
                std::string epoll_error = strerror(errno);
                throw std::runtime_error("epoll_ctl(): " +
                                         epoll_error);
            }
        }

        m_arr_event.resize(m_arr_event.size() - 1);
    }

    std::unordered_set<uint64_t> wait(beltpp::timer const& tm)
    {
        std::unordered_set<uint64_t> set_ids;

        int count = -1;

        {
            int milliseconds = -1;
            if (tm.is_set)
            {
                auto timeout = (tm.last_point_expired + tm.timer_period) - tm.now();
                auto timeout_ms =
                        chrono::duration_cast<chrono::milliseconds>(timeout);
                milliseconds = timeout_ms.count();
                if (milliseconds < 0)
                    //  timeout should at least be 0
                    //  in order for epoll_wait to return immediately
                    milliseconds = 0;
            }
            count = ::epoll_wait(m_fd,
                                 &m_arr_event.front(),
                                 m_arr_event.size(),
                                 milliseconds);
        }

        if (-1 == count && errno == EINTR)
            count = 0;

        if (-1 == count)
        {
            std::string epoll_error = strerror(errno);
            throw std::runtime_error("epoll_wait(): " +
                                     epoll_error);
        }

        // for each ready socket
        for(int i = 0; i < count; ++i)
        {
            uint64_t id = m_arr_event[i].data.u64;
            set_ids.insert(id);
        }

        return set_ids;
    }

    void terminate() {}
    void reset(uint64_t) {}

    int m_fd;
    epoll_event m_event;
    std::vector<epoll_event> m_arr_event;
};
}
}

#elif defined B_OS_MACOS

#include <sys/event.h>
#include <sys/time.h>

namespace beltpp
{
namespace detail
{
class poll_master
{
public:
    poll_master()
    {
        m_fd = kqueue();
        if (-1 == m_fd)
        {
            std::string kqueue_error = strerror(errno);
            throw std::runtime_error("kqueue(): " +
                                     kqueue_error);
        }
    }

    ~poll_master()
    {
        close(m_fd);
    }

    void add(int socket_descriptor, uint64_t id, event_handler::task action)
    {
        assert(event_handler::task::remove != action);

        std::vector<int16_t> flags;
        if (event_handler::task::connect == action)    //  win version cares for receive case too
            flags = {EVFILT_WRITE};
        else if (event_handler::task::send == action)
            flags = {EVFILT_WRITE, EVFILT_READ};
        else
            flags = {EVFILT_READ};

        static_assert(sizeof(void*) == sizeof(uint64_t), "64 bit pointers, no?");

        m_arr_event.resize(m_arr_event.size() + flags.size());

        for (auto flag : flags)
        {
            EV_SET(&m_event, uintptr_t(socket_descriptor), flag, EV_ADD, NOTE_WRITE, 0, nullptr);
            m_event.udata = reinterpret_cast<void*>(id);

            int res = kevent(m_fd, &m_event, 1, nullptr, 0, nullptr);
            if (-1 == res)
            {
                m_arr_event.resize(m_arr_event.size() - flags.size());
                std::string kevent_error = strerror(errno);
                throw std::runtime_error("kevent() add: " +
                                         kevent_error);
            }
        }
    }

    void remove(int socket_descriptor, uint64_t id, bool already_closed, event_handler::task action)
    {
        std::vector<int16_t> flags;
        if (event_handler::task::connect == action)
            flags = {EVFILT_WRITE};
        else if (event_handler::task::send == action)
            flags = {EVFILT_WRITE, EVFILT_READ};
        else
            flags = {EVFILT_READ};

        static_assert(sizeof(void*) == sizeof(uint64_t), "64 bit pointers, no?");

        for (auto flag : flags)
        {
            EV_SET(&m_event, uintptr_t(socket_descriptor), flag, EV_DELETE, 0, 0, nullptr);
            m_event.udata = reinterpret_cast<void*>(id);

            if (false == already_closed)
            {
                int res = kevent(m_fd, &m_event, 1, nullptr, 0, nullptr);
                if (-1 == res)
                {
                    std::string kevent_error = strerror(errno);
                    throw std::runtime_error("kevent() delete: " +
                                             kevent_error);
                }
            }
        }

        m_arr_event.resize(m_arr_event.size() - flags.size());
    }

    std::unordered_set<uint64_t> wait(beltpp::timer const& tm)
    {
        std::unordered_set<uint64_t> set_ids;

        size_t count = size_t(-1);

        {
            long long milliseconds = -1;
            if (tm.is_set)
            {
                auto timeout = (tm.last_point_expired + tm.timer_period) - tm.now();
                auto timeout_ms =
                        chrono::duration_cast<chrono::milliseconds>(timeout);
                milliseconds = timeout_ms.count();
                if (milliseconds < 0)
                    //  timeout should at least be 0
                    milliseconds = 0;
            }
            struct timespec tms, *ptm = nullptr;
            tms.tv_sec = milliseconds / 1000;
            tms.tv_nsec = (milliseconds % 1000) * 1000000;
            if (milliseconds > 0)
                ptm = &tms;
            count = size_t(kevent(m_fd,
                                  nullptr, 0,
                                  &m_arr_event.front(),
                                  int(m_arr_event.size()),
                                  ptm));
        }

        if (size_t(-1) == count && errno == EINTR)
            count = 0;

        if (size_t(-1) == count)
        {
            std::string kevent_error = strerror(errno);
            throw std::runtime_error("kevent(): " +
                                     kevent_error);
        }

        // for each ready socket
        for(size_t i = 0; i < count; ++i)
        {
            uint64_t id = reinterpret_cast<uint64_t>(m_arr_event[i].udata);
            set_ids.insert(id);
        }

        return set_ids;
    }

    void terminate() {}
    void reset(uint64_t) {}

    int m_fd;
    struct kevent m_event;
    std::vector<struct kevent> m_arr_event;
};
}
}

#elif defined B_OS_WINDOWS

#include <WinSock2.h>
#include <Windows.h>

namespace beltpp
{
    namespace detail
    {
        class poll_master
        {
            class details
            {
            public:
                enum {
                    async_task_running_none = 0x0,
                    async_task_running_receive = 0x1,
                    async_task_running_send = 0x2
                };
                details(event_handler::task action_, SOCKET socket_)
                    : action(action_)
                    , socket(socket_)
                    , async_task_running(async_task_running_none)
                    , overlapped()
                    , accept_buffer()
                    , accept_socket()
                    , receive_buffer()
                    , last_error(0)
                    , bytes_copied(0)
                {
                    init();
                }

                void init()
                {
                    if (event_handler::task::remove == action)
                        throw std::runtime_error("poll master details: event_handler:::remove == action");

                    ::ZeroMemory(&overlapped, sizeof(WSAOVERLAPPED));

                    if (event_handler::task::accept == action)
                    {
                        if (async_task_running)
                            ::closesocket(accept_socket);
                        //
                        //  WARNING, should not use socket type explicitly
                        //  AF_INET vs AF_INET6
                        //
                        accept_socket = native::socket(AF_INET, SOCK_STREAM, 0);
                        if (INVALID_SOCKET == accept_socket)
                            throw std::runtime_error("wsasocket(): " + native::net_last_error());
                    }
                    else if (event_handler::task::connect == action)
                    {
                    }
                    else/* if (event_handler::task::receive == action ||
                               event_handler::task::receive == action)*/
                    {
                        wsa_receive_buffer.len = 1024 * 4;
                        wsa_receive_buffer.buf = receive_buffer;

                        wsa_send_buffer.len = 1024 * 4;
                        wsa_send_buffer.buf = send_buffer;
                    }

                    last_error = 0;
                    bytes_copied = 0;
                    bytes_offset = 0;
                }

                void connect(const struct sockaddr* addr,
                             size_t len,
                             beltpp::ip_address const& address)
                {
                    // Load the AcceptEx function into memory using WSAIoctl.
                    // The WSAIoctl function is an extension of the ioctlsocket()
                    // function that can use overlapped I/O. The function's 3rd
                    // through 6th parameters are input and output buffers where
                    // we pass the pointer to our AcceptEx function. This is used
                    // so that we can call the AcceptEx function directly, rather
                    // than refer to the Mswsock.lib library.
                    LPFN_CONNECTEX ConnectEx;
                    DWORD dwBytes;
                    GUID guid = WSAID_CONNECTEX;
                    int res = ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &guid, sizeof(guid),
                        &ConnectEx, sizeof(ConnectEx),
                        &dwBytes, NULL, NULL);
                    if (res != 0)
                        throw std::runtime_error("connectex related");

                    char sendbuf[1];

                    bool code = ConnectEx(socket,
                                          addr,
                                          (int)len,
                                          (PVOID)sendbuf,
                                          0,
                                          &dwBytes,
                                          &overlapped);

                    //  to support sync completion, will need to return code and last error
                    //  to native::connect
                    //  and will leave running to false value

                    if (code)
                        throw std::runtime_error("connectex(): sync completion instead of async");

                    if (WSAGetLastError() != WSA_IO_PENDING)
                        throw std::runtime_error("connectex(" + address.to_string() + "): " + native::net_last_error());

                    async_task_running = async_task_running_receive;
                }

                event_handler::task action;
                SOCKET socket;
                int async_task_running;
                WSAOVERLAPPED overlapped;
                WSAOVERLAPPED overlapped_send;
                //  actually not using accept_buffer
                char accept_buffer[2 * (sizeof(sockaddr_storage) + 16)];
                SOCKET accept_socket;
                char receive_buffer[1024 * 4];
                WSABUF wsa_receive_buffer;
                char send_buffer[1024 * 4];
                WSABUF wsa_send_buffer;
                int last_error;
                DWORD bytes_copied;
                bool result_receive_send;
                DWORD bytes_offset; // this is needed for chunk by chunk reading
            };
        public:
            HANDLE m_completion_port;
            //  seems it is not thread safe to have m_events member
            //  most probably it is possible to move inside channels
            //  will optimize lookups, get thread safety, optimize buffer copies
            std::unordered_map<uint64_t, std::unique_ptr<details>> m_events;

            poll_master()
            {
                m_completion_port = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
                if (m_completion_port == NULL)
                    throw std::runtime_error("create io completion port: " + native::win_last_error(::GetLastError()));
            }

            ~poll_master()
            {
                ::CloseHandle(m_completion_port);
            }

            void add(SOCKET socket_descriptor,
                     uint64_t id,
                     event_handler::task action)
            {
                assert(event_handler::task::remove != action);

                auto res = m_events.insert(std::make_pair(id, std::unique_ptr<details>(new details(action, socket_descriptor))));
                if (false == res.second &&
                    (res.first->second->action != event_handler::task::remove ||
                     details::async_task_running_none != res.first->second->async_task_running))
                    throw std::runtime_error("poll master add duplicate id: " + std::to_string(id));
                else if (false == res.second)
                    res.first->second = std::unique_ptr<details>(new details(action, socket_descriptor));
                else if (NULL == ::CreateIoCompletionPort(HANDLE(socket_descriptor), m_completion_port, id, 0))
                    throw std::runtime_error("bind io completion port to socket: " + native::win_last_error(GetLastError()));
            }

            void remove(SOCKET /*socket_descriptor*/,
                        uint64_t id,
                        bool /*already_closed*/,
                        event_handler::task)  //  last argument is used for mac os version
            {
                auto it = m_events.find(id);
                if (it == m_events.end())
                    throw std::runtime_error("poll master remove non existing id: " + std::to_string(id));

                it->second->action = event_handler::task::remove;
            }

            std::unordered_set<uint64_t> wait(beltpp::timer const& tm)
            {
                auto it = m_events.begin();
                while (it != m_events.end())
                {
                    if (false == it->second->async_task_running &&
                        event_handler::task::remove == it->second->action)
                        it = m_events.erase(it);
                    else
                        ++it;
                }

                bool sync_completed = false;
                uint64_t completed_id = 0;
                bool completed_code = false;
                bool result_receive_send = true;

                for (auto& item : m_events)
                {
                    if (event_handler::task::accept == item.second->action &&
                        details::async_task_running_none == item.second->async_task_running)
                    {
                        bool repeat = false;
                        bool code = false;
                        do
                        {
                            repeat = false;
                            code = ::AcceptEx(item.second->socket,
                                item.second->accept_socket,
                                item.second->accept_buffer,
                                0,
                                sizeof(sockaddr_storage) + 16,
                                sizeof(sockaddr_storage) + 16,
                                &item.second->bytes_copied,
                                &item.second->overlapped);

                            if (code == false && WSAGetLastError() == WSAECONNRESET)
                                repeat = true;
                        } while (repeat);

                        if (code || WSAGetLastError() != WSA_IO_PENDING)
                        {
                            sync_completed = true;
                            completed_id = item.first;
                            completed_code = code;
                            result_receive_send = true;
                            break;
                        }

                        item.second->async_task_running = details::async_task_running_receive;
                    }
                    else if (event_handler::task::connect == item.second->action)
                    {
                        item.second->async_task_running = details::async_task_running_receive;
                    }
                    else if ((event_handler::task::receive == item.second->action ||
                              event_handler::task::send == item.second->action))
                    {
                        if (event_handler::task::send == item.second->action &&
                            0 == (details::async_task_running_send & item.second->async_task_running))
                        {
                            int send_code = ::WSASend(item.second->socket,
                                                      &item.second->wsa_send_buffer, 1,
                                                      &item.second->bytes_copied,
                                                      0,
                                                      &item.second->overlapped_send,
                                                      nullptr);

                            if (SOCKET_ERROR == send_code && WSAGetLastError() != WSA_IO_PENDING)
                            {
                                sync_completed = true;
                                completed_id = item.first;
                                completed_code = (SOCKET_ERROR != send_code);
                                result_receive_send = false;
                                break;
                            }

                            item.second->async_task_running |= details::async_task_running_send;
                        }

                        if (0 == (details::async_task_running_receive & item.second->async_task_running))
                        {
                            DWORD flags = 0;
                            int code = ::WSARecv(item.second->socket,
                                                 &item.second->wsa_receive_buffer, 1,
                                                 &item.second->bytes_copied,
                                                 &flags,
                                                 &item.second->overlapped,
                                                 nullptr);

                            if (SOCKET_ERROR == code && WSAGetLastError() != WSA_IO_PENDING)
                            {
                                sync_completed = true;
                                completed_id = item.first;
                                completed_code = (SOCKET_ERROR != code);
                                result_receive_send = true;
                                break;
                            }

                            item.second->async_task_running |= details::async_task_running_receive;
                        }
                        else if (item.second->bytes_offset > 0)
                        {
                            sync_completed = true;
                            completed_id = item.first;
                            completed_code = true;
                            result_receive_send = true;
                            break;
                        }
                    }
                }

                std::unordered_set<uint64_t> set_ids;
                
                DWORD milliseconds = WSA_INFINITE;
                if (tm.is_set)
                {
                    auto timeout = (tm.last_point_expired + tm.timer_period) - tm.now();
                    auto timeout_ms = chrono::duration_cast<chrono::milliseconds>(timeout);
                    milliseconds = DWORD(timeout_ms.count());
                    if (milliseconds < 0)
                        //  timeout should at least be 0
                        //  in order for GetQueuedCompletionStatus to return immediately
                        milliseconds = 0;
                }
                
                DWORD bytesCopied = 0;
                uint64_t id = 0;
                OVERLAPPED* poverlapped = nullptr;

                bool empty = false;

                bool code = completed_code;
                if (false == sync_completed)
                {
                    code = ::GetQueuedCompletionStatus(m_completion_port,
                                                       &bytesCopied,
                                                       &id,
                                                       &poverlapped,
                                                       milliseconds);

                    if (poverlapped)
                    {
                        it = m_events.find(id);

                        if (m_events.end() == it)
                            //  this seems to happen commonly
                            //  probably same event is received both sync and async
                            empty = true;
                        else
                        {
                            it->second->bytes_copied = bytesCopied;
                            if (poverlapped == &it->second->overlapped)
                            {
                                result_receive_send = true;
                                it->second->result_receive_send = true;
                            }
                            else if (poverlapped == &it->second->overlapped_send)
                            {
                                result_receive_send = false;
                                it->second->result_receive_send = false;
                            }
                            else
                            {
                                assert(false);
                                std::terminate();
                            }

                            if (false == code)
                                it->second->last_error = WSAGetLastError();
                        }
                    }
                    else
                        empty = true;
                }
                else
                {
                    id = completed_id;
                    it = m_events.find(id);
                    if (false == code)
                        it->second->last_error = WSAGetLastError();
                }
                
                if (false == empty)
                {
                    if (m_events.end() == it)
                        throw std::runtime_error("GetQueuedCompletionStatus(): got non existing id");

                    if (result_receive_send)
                        it->second->async_task_running &= ~details::async_task_running_receive;
                    else
                        it->second->async_task_running &= ~details::async_task_running_send;
                    set_ids.insert(id);
                }

                return set_ids;
            }

            void terminate()
            {
                ::PostQueuedCompletionStatus(m_completion_port,
                                             0, // bytesCopied
                                             0, // completionKey
                                             0); // overlapped
            }
            void reset(uint64_t /*reset_id*/)
            {   //  can probably get rid of this
            }
        };
    }
}

#endif
