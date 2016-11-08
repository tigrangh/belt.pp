#ifndef ASYNC_PROCESSOR_H
#define ASYNC_PROCESSOR_H
//#include "async_decl.h"

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
//#include <memory>
#include <exception>
#include <functional>
#include <list>
#include <assert.h>

namespace async
{
class processor;
void run(size_t id, processor& host) noexcept;

class processor final
{
public:
    enum class pending_policy { allow_new, prevent_new, empty_queue };
    friend void run(size_t id, processor &host) noexcept;
    friend class thread;
    using task = std::function<void()>;

    inline processor(size_t count)
        : m_policy(pending_policy::allow_new)
        , m_thread_count(count)
    {
        setCoreCount(count);
        //wait();
    }
    processor(processor const&) = delete;
    processor(processor&&) = delete;
    inline ~processor()
    {
        //  make sure this does not throw
        setCoreCount(0);
    }

    processor& operator = (processor const&) = delete;
    processor& operator = (processor&&) = delete;

    inline void setCoreCount(size_t count);
    inline size_t getCoreCount();
    inline void add(task& th);
    inline void wait();
private:
    using threads = std::list<std::thread>;
    using mutex = std::mutex;
    using cv = std::condition_variable;
    using queue = std::queue<task>;
    //
    pending_policy m_policy;
    size_t m_thread_count;
    threads m_threads;
    mutex m_mutex;
    cv m_cv_add_exit__run;
    queue m_queue;
};

void processor::setCoreCount(size_t count)
{
    {
        std::lock_guard<mutex> lock(m_mutex);
        m_thread_count = count;
    }
    wait();
}

size_t processor::getCoreCount()
{
    std::lock_guard<mutex> lock(m_mutex);
    return m_threads.size();
}

void processor::add(processor::task& obtask)
{
    bool bNotify = false;
    {
        std::lock_guard<mutex> lock(m_mutex);
        if (m_policy == pending_policy::allow_new)
        {
            m_queue.push(obtask);
            bNotify = true;
        }
    }
    if (bNotify)
        m_cv_add_exit__run.notify_one();  //  notify one run() to wake up from empty wait
}

void processor::wait()
{
    size_t current_thread_count = 0;
    size_t desired_thread_count = 0;
    {
        std::lock_guard<mutex> lock(m_mutex);
        current_thread_count = m_threads.size();
        desired_thread_count = m_thread_count;
    }
    while (current_thread_count < desired_thread_count)
    {
        m_threads.push_back(std::thread(&run, current_thread_count, std::ref(*this)));
        ++current_thread_count;
    }
    if (current_thread_count > desired_thread_count)
        m_cv_add_exit__run.notify_all();  //  notify all run() to wake up from empty state sleep
    while (current_thread_count > desired_thread_count)
    {
        m_threads.back().join();
        m_threads.pop_back();
        --current_thread_count;
    }
}

inline void run(size_t id, processor& host) noexcept
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(host.m_mutex);

        if (host.m_queue.empty())
        {
            if (host.m_thread_count < id + 1)
                return;

            {
                auto const& qu = host.m_queue;
                auto const& thread_count = host.m_thread_count;

                host.m_cv_add_exit__run.wait(lock,
                                             [&qu,
                                             &thread_count,
                                             id]
                {
                    return (false == qu.empty() ||  //  one wait for processor::add
                            thread_count < id + 1); //  all wait for processor::wait
                });
            }
        }

        if (false == host.m_queue.empty())
        {
            auto obtask = host.m_queue.front();
            host.m_queue.pop();

            lock.unlock();

            try
            {
                if (processor::pending_policy::empty_queue != host.m_policy)
                    obtask();
            }
            catch (...){}
        }
    }
}

}

#endif // ASYNC_PROCESSOR_H
