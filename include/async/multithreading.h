#ifndef ASYNC_MULTITHREADING_H
#define ASYNC_MULTITHREADING_H

#include "processor.h"

#include <list>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <functional>
#include <assert.h>

namespace async
{

class thread;
using task = std::function<void()>;

class thread_pool final
{
    friend class thread;
    friend void thread_task(thread*);
public:
    thread_pool(processor<task>& host)
        : m_phost(&host) {}
    thread_pool() = delete;
    thread_pool(thread_pool const&) = delete;
    thread_pool(thread_pool&&) = default;
    inline ~thread_pool();

    thread_pool& operator = (thread_pool const&) = delete;
    thread_pool& operator = (thread_pool&&) = delete;

    using cv = std::condition_variable;
    using mutex = std::mutex;
    using threadset = std::unordered_set<thread*>;

    enum efor {all, any};
    threadset wait(efor option);
private:
    processor<task>* m_phost;
    mutex m_mutex;
    threadset m_pending_threads;
    threadset m_done_threads;
    cv m_cv_run__wait;
};

thread_pool::~thread_pool()
{
    m_phost->setCoreCount(0);
}

thread_pool::threadset thread_pool::wait(efor option)
{
    std::unique_lock<mutex> lock(m_mutex);
    threadset &pending_threads = m_pending_threads;
    threadset &done_threads = m_done_threads;

    if (all == option)
    {
        m_cv_run__wait.wait(lock, [&pending_threads, &done_threads]
        {
            return (pending_threads.size() == done_threads.size());
        });
    }
    else
    {
        m_cv_run__wait.wait(lock, [&pending_threads, &done_threads]
        {
            return (pending_threads.empty() ||
                    false == done_threads.empty());
        });
    }

    for (auto const& pth : done_threads)
    {
        auto it = pending_threads.find(pth);
        if (it != pending_threads.end())
            pending_threads.erase(it);
    }

    auto result = done_threads;
    done_threads.clear();

    return result;
}

class thread final
{
    friend void thread_task(thread*);
public:
    explicit thread(thread_pool&);
    thread() = delete;
    thread(thread const&) = delete;
    thread(thread&&) = delete;
    inline ~thread();

    thread& operator = (thread const&) = delete;
    thread& operator = (thread&&) = delete;

public:
    inline void run(processor<task>::task const& obtask);
    inline void wait(bool bThrow = true);
private:
    using tasks = std::list<processor<task>::task>;
    inline void notify();
    inline void get(tasks::iterator& first, tasks::iterator& last);
private:
    using mutex = std::mutex;
    using cv = std::condition_variable;

    thread_pool* m_phost;
    tasks m_tasks;
    mutex m_mutex;
    cv m_cv;
    std::exception_ptr m_ptr_exception;
};

thread::thread(thread_pool& host)
    : m_phost(&host)
{}

thread::~thread()
{
    wait(false);
}

void thread::run(processor<task>::task const& obtask)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    bool bAddToHost = m_tasks.empty();
    m_tasks.push_back(obtask);

    if (bAddToHost)
    {
        std::function<void()> proc_task([this]{thread_task(this);});
        m_phost->m_phost->run(proc_task);

        std::unique_lock<mutex> lock(m_phost->m_mutex);
        m_phost->m_pending_threads.insert(this);
        auto it_find = m_phost->m_done_threads.find(this);
        if (it_find != m_phost->m_done_threads.end())
            m_phost->m_done_threads.erase(it_find);
    }
}

void thread::wait(bool bThrow/* = true*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto& tasks = m_tasks;
    m_cv.wait(lock, [&tasks]
    {
        return tasks.empty();
    });

    if (bThrow)
    {
        auto pex = std::move(m_ptr_exception);
        if (pex)
            std::rethrow_exception(pex);
    }
}

void thread::get(thread::tasks::iterator& first, thread::tasks::iterator& last)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    assert(!m_tasks.empty());
    first = m_tasks.begin();
    last = m_tasks.end();
    --last;
}

void thread::notify()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_tasks.empty())
        m_cv.notify_one();
}

void thread_task(thread* pth)
{
    thread::tasks::iterator it_first, it_last;
    pth->get(it_first, it_last);
    thread::tasks::iterator it = it_first;

    bool bRun = true;
    {
        std::lock_guard<thread::mutex> lock(pth->m_mutex);
        if (pth->m_ptr_exception)
            bRun = false;
    }

    try
    {
        while (bRun)
        {
            it->operator()();
            if (it == it_last)
                break;
            ++it;
        };
    }
    catch (...)
    {
        pth->m_ptr_exception = std::current_exception();
    }

    ++it_last;

    {
        std::lock_guard<thread::mutex> lock(pth->m_mutex);
        pth->m_tasks.erase(it_first, it_last);

        if (pth->m_ptr_exception)
            pth->m_tasks.clear();

        if (false == pth->m_tasks.empty())
        {
            std::function<void()> proc_task([pth]{thread_task(pth);});
            pth->m_phost->m_phost->run(proc_task);
        }
        else
        {
            {
                std::lock_guard<thread::mutex> lock(pth->m_phost->m_mutex);
                pth->m_phost->m_done_threads.insert(pth);
            }
            pth->m_phost->m_cv_run__wait.notify_one();
        }
    }

    pth->notify();
}

}

#endif // ASYNC_MULTITHREADING_H
