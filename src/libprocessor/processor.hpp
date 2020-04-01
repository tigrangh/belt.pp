#pragma once

#include "global.hpp"

#include <belt.pp/iprocessor.hpp>
#include <belt.pp/stream.hpp>

#include <memory>
#include <functional>

namespace beltpp
{
class packet;
inline void task(int /*i*/)
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
class processor_old;

namespace detail
{
template <typename task_t>
void loop(size_t id, processor_old<task_t>& host) noexcept;

template <typename task_t>
class processor_old_internals;
}

template <typename task_t>
class PROCESSORSHARED_EXPORT processor_old : public iprocessor_old<task_t>
{
public:
    template <typename task_tt>
    friend void detail::loop(size_t id, processor_old<task_tt> &host) noexcept;
    using task = task_t;

    processor_old(size_t count);
    processor_old(processor_old const&) = delete;
    processor_old(processor_old&&) = delete;
    ~processor_old() override;

    processor_old& operator = (processor_old const&) = delete;
    processor_old& operator = (processor_old&&) = delete;

    void setCoreCount(size_t count) override;
    size_t getCoreCount() override;
    void run(task_t const& th) override;  // add
    void wait(size_t i_task_count = 0) override;
private:
    std::unique_ptr<detail::processor_old_internals<task_t>> m_pimpl;
};

PROCESSORSHARED_EXPORT stream_ptr construct_processor(event_handler& eh,
                                                      size_t count,
                                                      std::function<beltpp::packet(beltpp::packet&&)> const& worker);
PROCESSORSHARED_EXPORT event_handler_ptr construct_event_handler();
}
