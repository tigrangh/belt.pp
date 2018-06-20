#pragma once

#include <belt.pp/iprocessor.hpp>
#include "global.hpp"

#include <memory>

namespace beltpp
{

inline void task(int i)
{
    /*if (i > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(i));
    volatile int count = i;
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
void loop(size_t id, processor<task_t>& host) noexcept;

template <typename task_t>
class processor_internals;
}

template <typename task_t>
class PROCESSORSHARED_EXPORT processor : public iprocessor<task_t>
{
public:
    template <typename task_tt>
    friend void detail::loop(size_t id, processor<task_tt> &host) noexcept;
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
    void wait(size_t i_task_count = 0) override;
private:
    std::unique_ptr<detail::processor_internals<task_t>> m_pimpl;
};

}
