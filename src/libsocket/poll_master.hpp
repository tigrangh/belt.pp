#pragma once

#include "global.hpp"
#include "native.hpp"

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

    void add(int socket_descriptor, uint64_t id, bool out, bool) // last argument is used for WIN version
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

    void add(int socket_descriptor, uint64_t id, bool out, bool)// last argument is used for WIN version
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

    void reset(uint64_t) {}

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

            int event_count;
            WSAEVENT wsa_event_array[WSA_MAXIMUM_WAIT_EVENTS];
            std::unordered_map<int, uint64_t> index_map; // wsa_index->id mapping
            std::unordered_map<int, SOCKET> socket_map; // wsa_index->SOCKET mapping
            std::unordered_map<uint64_t, int> reset_map; // id->wsa_index mapping

            poll_master()
            {
                event_count = 0;
            }

            ~poll_master()
            {
            }

            void add(SOCKET socket_descriptor, uint64_t id, bool out, bool close)
            {
                if (event_count >= WSA_MAXIMUM_WAIT_EVENTS)
                    throw std::runtime_error("Too many connections");

                wsa_event_array[event_count] = WSACreateEvent();

                if (wsa_event_array[event_count] == WSA_INVALID_EVENT)
                    throw std::runtime_error("WSACreateEvent() failed with error: " + WSAGetLastError());

                index_map[event_count] = id;
                reset_map[id] = event_count;
                socket_map[event_count] = socket_descriptor;

                if(out)
                    WSAEventSelect(socket_descriptor, wsa_event_array[event_count], FD_WRITE);
                else
                    if(!close)// for listen channels
                        WSAEventSelect(socket_descriptor, wsa_event_array[event_count], FD_ACCEPT | FD_READ);
                    else// for stream cannels
                        WSAEventSelect(socket_descriptor, wsa_event_array[event_count], FD_ACCEPT | FD_READ | FD_CLOSE); 

                ++event_count;
            }

            void remove(SOCKET socket_descriptor, uint64_t id, bool, bool)  //  last argument is used for mac os version
            {
                if (reset_map.find(id) == reset_map.end())
                    return;

                --event_count;
                int wsa_index = reset_map[id];

                WSACloseEvent(wsa_event_array[wsa_index]);

                for (int i = wsa_index; i < event_count; ++i)
                {
                    wsa_event_array[i] = wsa_event_array[i + 1];

                    index_map[i] = index_map[i + 1];
                }

                reset_map.erase(id);
                index_map.erase(event_count);
                socket_map.erase(wsa_index);

                for (auto &item : reset_map)
                    if (item.second > wsa_index)
                        item.second = item.second - 1;
            }

            std::unordered_set<uint64_t> wait(timer_helper const& tm)
            {
                std::unordered_set<uint64_t> set_ids;
                
                DWORD milliseconds = WSA_INFINITE;
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

                if (index == WSA_WAIT_TIMEOUT)
                    throw std::runtime_error("WSAWaitForMultipleEvents() finished with Timeout! ");
 
                index = index - WSA_WAIT_EVENT_0;

                WSANETWORKEVENTS networkevents;
                for (int i = index; i < event_count; ++i) 
                {
                    int _index = WSAWaitForMultipleEvents(1, &wsa_event_array[i], TRUE, 0, FALSE);
                    
                    if ((_index != WSA_WAIT_FAILED) && (_index != WSA_WAIT_TIMEOUT))
                    {
                        WSAEnumNetworkEvents(socket_map[i], NULL, &networkevents);
                        
                        uint64_t id = index_map[i];
                        set_ids.insert(id);
                    }                    
                }
                
                return set_ids;
            }

            void reset(uint64_t reset_id)
            {
                int index = reset_map[reset_id];

                if (!WSAResetEvent(wsa_event_array[index]))
                   throw std::runtime_error("WSAResetEvent failed with error: " + native::last_error());
            }
        };
    }
}

#endif
