#ifndef BELT_PROCESSOR_H
#define BELT_PROCESSOR_H

#include "iprocessor.hpp"
#include "global.hpp"

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <list>
#include <assert.h>

namespace beltpp
{

void task(int i)
{
    if (i > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(i));
    /*volatile int count = i;
    for (int a = 0; a < count; ++a)
    {
        ++a;
    }*/
}
class PROCESSORSHARED_EXPORT ctask
{
public:
    void mem() const
    {
        task(i);
    }

    void operator()() const
    {
        task(i);
    }

    int i;
};

template <typename task_t>
class processor;

namespace detail
{
template <typename task_t>
void thread_run(size_t id, processor<task_t>& host) noexcept;
}

template <typename task_t>
class PROCESSORSHARED_EXPORT processor : public iprocessor<task_t>
{
private:
    enum class pending_policy { allow_new, prevent_new, empty_queue };
public:
    template <typename task_tt>
    friend void detail::thread_run(size_t id, processor<task_tt> &host) noexcept;
    using task = task_t;

    processor(size_t count);
    processor(processor const&) = delete;
    processor(processor&&) = delete;
    ~processor() override;

    processor& operator = (processor const&) = delete;
    processor& operator = (processor&&) = delete;

    void setCoreCount(size_t count) override;
    size_t getCoreCount() override;
    void run(task_t const& th) override;  // add
    void wait() override;
private:
    using threads = std::list<std::thread>;
    using mutex = std::mutex;
    using cv = std::condition_variable;
    using queue = std::queue<task_t>;
    //
    pending_policy m_policy;
    size_t m_thread_count;
    size_t m_busy_count;
    threads m_threads;
    mutex m_mutex;
    cv m_cv_add_exit__thrun;
    cv m_cv_thrun__wait;
    queue m_queue;
};

}

#endif // BELT_PROCESSOR_H
