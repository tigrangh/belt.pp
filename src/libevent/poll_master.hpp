#pragma once

#include "global.hpp"

#ifndef B_OS_WINDOWS
#include <unistd.h>
#endif

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cerrno>
#include <cstring>
#include <vector>
#include <chrono>

namespace beltpp
{
namespace detail
{

namespace chrono = std::chrono;
using time_point = chrono::steady_clock::time_point;
using duration = chrono::steady_clock::duration;

class timer_helper
{
public:
    timer_helper()
        : is_set(false)
        , last_point()
        , timer_period()
    {}

    static time_point now()
    {
        return chrono::steady_clock::now();
    }

    void set(duration const& period)
    {
        is_set = true;
        last_point = now();
        timer_period = period;
    }

    void clean()
    {
        is_set = false;
    }

    void update()
    {
        last_point += timer_period;
    }

    bool expired() const
    {
        auto time_since_started = now() - last_point;
        return is_set && (time_since_started >= timer_period);
    }

    bool is_set;
    time_point last_point;
    duration timer_period;
};
}
}

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

    void add(int socket_descriptor, uint64_t id, bool out)
    {
        m_event.data.u64 = id;

        auto backup = m_event.events;
        if (out)
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

    void remove(int socket_descriptor, uint64_t id, bool already_closed, bool)  //  last argument is used for mac os version
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

    std::unordered_set<uint64_t> wait(timer_helper const& tm)
    {
        std::unordered_set<uint64_t> set_ids;

        int count = -1;

        {
            int milliseconds = -1;
            if (tm.is_set)
            {
                auto timeout = (tm.last_point + tm.timer_period) - tm.now();
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

    void add(int socket_descriptor, uint64_t id, bool out)
    {
        if (out)
            EV_SET(&m_event, socket_descriptor, EVFILT_WRITE, EV_ADD, NOTE_WRITE, 0, nullptr);
        else
            EV_SET(&m_event, socket_descriptor, EVFILT_READ, EV_ADD, NOTE_WRITE, 0, nullptr);

        static_assert(sizeof(void*) == sizeof(uint64_t), "64 bit pointers, no?");
        m_event.udata = reinterpret_cast<void*>(id);

        m_arr_event.resize(m_arr_event.size() + 1);

        int res = kevent(m_fd, &m_event, 1, nullptr, 0, nullptr);
        if (-1 == res)
        {
            m_arr_event.resize(m_arr_event.size() - 1);
            std::string kevent_error = strerror(errno);
            throw std::runtime_error("kevent() add: " +
                                     kevent_error);
        }
    }

    void remove(int socket_descriptor, uint64_t id, bool already_closed, bool out)
    {
        if (out)
            EV_SET(&m_event, socket_descriptor, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        else
            EV_SET(&m_event, socket_descriptor, EVFILT_READ, EV_DELETE, 0, 0, nullptr);

        static_assert(sizeof(void*) == sizeof(uint64_t), "64 bit pointers, no?");
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

        m_arr_event.resize(m_arr_event.size() - 1);
    }

    std::unordered_set<uint64_t> wait(timer_helper const& tm)
    {
        std::unordered_set<uint64_t> set_ids;

        int count = -1;

        {
            int milliseconds = -1;
            if (tm.is_set)
            {
                auto timeout = (tm.last_point + tm.timer_period) - tm.now();
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
            count = kevent(m_fd,
                           nullptr, 0,
                           &m_arr_event.front(), m_arr_event.size(),
                           ptm);
        }

        if (-1 == count && errno == EINTR)
            count = 0;

        if (-1 == count)
        {
            std::string kevent_error = strerror(errno);
            throw std::runtime_error("kevent(): " +
                                     kevent_error);
        }

        // for each ready socket
        for(int i = 0; i < count; ++i)
        {
            uint64_t id = reinterpret_cast<uint64_t>(m_arr_event[i].udata);
            set_ids.insert(id);
        }

        return set_ids;
    }

    int m_fd;
    struct kevent m_event;
    std::vector<struct kevent> m_arr_event;
};
}
}

#elif defined B_OS_WINDOWS

#include <WinSock2.h>

namespace beltpp
{
    namespace detail
    {
        class poll_master
        {
        public:
            struct poll_events
            {
                native::sk_handle socket;
                uint64_t id;
                long events;
                int wsa_index;
            };

            int event_count;
            poll_events m_event;
            WSAEVENT wsa_event_array[WSA_MAXIMUM_WAIT_EVENTS];
            std::unordered_map<int, native::sk_handle> index_map;
            std::unordered_map<unsigned long long, poll_events> socket_map;

            poll_master()
            {
                event_count = 0;
                m_event.events = FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_READ | FD_WRITE | FD_OOB;
            }

            ~poll_master()
            {
            }

            void add(native::sk_handle socket_descriptor, uint64_t id, bool out)
            {
                if (event_count >= WSA_MAXIMUM_WAIT_EVENTS)
                    throw std::runtime_error("Too many connections");

                wsa_event_array[event_count] = WSACreateEvent();

                if (wsa_event_array[event_count] == WSA_INVALID_EVENT)
                    throw std::runtime_error("WSACreateEvent() failed with error: " + WSAGetLastError());

                m_event.id = id;
                m_event.socket = socket_descriptor;

                auto backup = m_event.events;
                
                if (out) //TODO
                    m_event.events |= FD_WRITE;
                
                m_event.events = backup;

                m_event.wsa_index = event_count;
                socket_map[socket_descriptor.handle] = m_event;
                index_map[event_count] = socket_descriptor;
                
                WSAEventSelect(socket_descriptor.handle, wsa_event_array[event_count], m_event.events);
                                
                ++event_count;                
            }

            void remove(native::sk_handle const& socket_descriptor, uint64_t id, bool already_closed, bool)  //  last argument is used for mac os version
            {
                if (socket_map.find(socket_descriptor.handle) == socket_map.end())
                    throw std::runtime_error("remove can't process socket_descriptor:" + socket_descriptor.handle);

                //TODO already_closed

                for (int i = socket_map[socket_descriptor.handle].wsa_index; i < event_count; ++i)
                {
                    wsa_event_array[i] = wsa_event_array[i + 1];
                    socket_map[index_map[i].handle].wsa_index--;
                    index_map[i] = index_map[i + 1];
                }

                index_map.erase(event_count);
                socket_map.erase(socket_descriptor.handle);
                --event_count;
            }

            std::unordered_set<uint64_t> wait(timer_helper const& tm)
            {
                std::unordered_set<uint64_t> set_ids;
                
                DWORD milliseconds = -1;
                if (tm.is_set)
                {
                    auto timeout = (tm.last_point + tm.timer_period) - tm.now();
                    auto timeout_ms = chrono::duration_cast<chrono::milliseconds>(timeout);
                    milliseconds = DWORD(timeout_ms.count());
                    if (milliseconds < 0)
                        //  timeout should at least be 0
                        //  in order for epoll_wait to return immediately
                        milliseconds = 0;
                }
                
                DWORD index = WSAWaitForMultipleEvents(event_count, wsa_event_array, false, milliseconds, false);
 
                if (index == WSA_WAIT_FAILED)
                    throw std::runtime_error("WSAWaitForMultipleEvents() failed with error: " + WSAGetLastError());
 
                index = index - WSA_WAIT_EVENT_0;

                // Iterate through all events and enumerate
                // if the wait does not fail.

                WSANETWORKEVENTS networkevents;
                for (int i = index; i < event_count; ++i) 
                {
                    index = WSAWaitForMultipleEvents(1, &wsa_event_array[i], TRUE, 1000, FALSE);
                    
                    if ((index != WSA_WAIT_FAILED) && (index != WSA_WAIT_TIMEOUT))
                    {
                        WSAEnumNetworkEvents(index_map[index].handle, wsa_event_array[index], &networkevents);

                        if ((networkevents.lNetworkEvents & FD_ACCEPT) &&
                            networkevents.iErrorCode[FD_ACCEPT_BIT] == 0)
                        {
                            uint64_t id = socket_map[index_map[index].handle].id;
                            set_ids.insert(id);
                        }
                    }                    
                }
                
                return set_ids;
            }
        };
    }
}

#endif
