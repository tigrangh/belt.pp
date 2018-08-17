#include "processor.hpp"

#include <belt.pp/queue.hpp>

#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <list>
#include <cassert>
#include <functional>

namespace beltpp
{

using threads = std::list<std::thread>;
using mutex = std::mutex;
using cv = std::condition_variable;
enum class pending_policy { allow_new, prevent_new, empty_queue };
/*
 * processor_internals
 */
namespace detail
{
template <typename task_t>
class processor_internals
{
public:
    processor_internals(size_t count)
        : m_policy(pending_policy::allow_new)
        , m_thread_count(count)
        , m_busy_count(0)
        , m_i_task_count(0)
    {}

    processor_internals(processor_internals const&) = delete;
    processor_internals(processor_internals&&) = delete;
    processor_internals() = delete;

    processor_internals const& operator = (processor_internals const&) = delete;
    processor_internals const& operator = (processor_internals&&) = delete;

    pending_policy m_policy;
    size_t m_thread_count;
    size_t m_busy_count;
    size_t m_i_task_count;
    threads m_threads;
    mutex m_mutex;
    cv m_cv_run_exit__loop;
    cv m_cv_loop__wait;
    beltpp::queue<task_t> m_vec_queue;
};
}
/*
 * processor
 */
template <typename task_t>
processor<task_t>::processor(size_t count)
    : m_pimpl(new detail::processor_internals<task_t>(count))
{
    setCoreCount(count);
}

template <typename task_t>
processor<task_t>::~processor()
{
    try
    {
        setCoreCount(0);
    }
    catch(...){}
}

template <typename task_t>
void processor<task_t>::setCoreCount(size_t count)
{
    {
        std::lock_guard<mutex> lock(m_pimpl->m_mutex);
        m_pimpl->m_thread_count = count;
    }
    size_t current_thread_count = 0;
    size_t desired_thread_count = 0;
    {
        std::lock_guard<mutex> lock(m_pimpl->m_mutex);
        current_thread_count = m_pimpl->m_threads.size();
        desired_thread_count = m_pimpl->m_thread_count;
    }
    while (current_thread_count < desired_thread_count)
    {
        void (*fptrloop)(size_t, processor<task_t>&);
        fptrloop = &detail::loop;
        m_pimpl->m_threads.push_back(
                    std::thread(fptrloop,
                                current_thread_count,
                                std::ref(*this)));
        ++current_thread_count;
    }
    if (current_thread_count > desired_thread_count)
        m_pimpl->m_cv_run_exit__loop.notify_all();  //  notify all loop() to wake up from empty state sleep
    while (current_thread_count > desired_thread_count)
    {
        m_pimpl->m_threads.back().join();
        m_pimpl->m_threads.pop_back();
        --current_thread_count;
    }
}

template <typename task_t>
size_t processor<task_t>::getCoreCount()
{
    std::lock_guard<mutex> lock(m_pimpl->m_mutex);
    return m_pimpl->m_threads.size();
}

template <typename task_t>
void processor<task_t>::run(processor<task_t>::task const& obtask)
{
    bool bNotify = false;
    {
        std::lock_guard<mutex> lock(m_pimpl->m_mutex);
        if (m_pimpl->m_policy == pending_policy::allow_new)
        {
            m_pimpl->m_vec_queue.push(obtask); //  may throw, that's ok
            ++m_pimpl->m_i_task_count;
            bNotify = true;
        }
    }
    if (bNotify)
        m_pimpl->m_cv_run_exit__loop.notify_one();  //  notify one loop() to wake up from empty wait
}

template <typename task_t>
void processor<task_t>::wait(size_t i_task_count/* = 0*/)
{
    std::unique_lock<std::mutex> lock(m_pimpl->m_mutex);

    auto const& pimpl = m_pimpl;
    m_pimpl->m_cv_loop__wait.wait(lock, [&pimpl, i_task_count]
    {
        if (pimpl->m_vec_queue.empty() &&
            0 == pimpl->m_busy_count &&
            pimpl->m_i_task_count >= i_task_count)
        {
            pimpl->m_i_task_count = 0;
            return true;
        }
        return false;
    });
}

namespace detail
{
template <typename task_t>
inline void loop(size_t id, processor<task_t>& host) noexcept
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(host.m_pimpl->m_mutex);

        if (host.m_pimpl->m_vec_queue.empty())
        {
            if (host.m_pimpl->m_thread_count < id + 1)
                return;

            {
                auto const& pimpl = host.m_pimpl;
                host.m_pimpl->m_cv_run_exit__loop.wait(lock,
                                                       [&pimpl,
                                                       id]
                {
                    return (false == pimpl->m_vec_queue.empty() ||  //  one wait for processor::add
                            pimpl->m_thread_count < id + 1);        //  all wait for processor::exit
                });
            }
        }

        if (false == host.m_pimpl->m_vec_queue.empty())
        {
            auto obtask = host.m_pimpl->m_vec_queue.front();
            host.m_pimpl->m_vec_queue.pop();
            ++host.m_pimpl->m_busy_count;

            lock.unlock();

            try
            {
                if (pending_policy::empty_queue != host.m_pimpl->m_policy)
                    obtask();
            }
            catch (...){}

            lock.lock();
            --host.m_pimpl->m_busy_count;
            if (host.m_pimpl->m_vec_queue.empty() &&
                0 == host.m_pimpl->m_busy_count)
            {
                host.m_pimpl->m_cv_loop__wait.notify_one();
            }
            lock.unlock();
        }
    }
}
}   //  end detail

}

template class beltpp::processor<beltpp::ctask>;
template class beltpp::processor<std::function<void()>>;

