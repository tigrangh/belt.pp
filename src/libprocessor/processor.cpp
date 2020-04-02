#include "processor.hpp"

#include <belt.pp/queue.hpp>
#include <belt.pp/packet.hpp>

#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <list>
#include <cassert>

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
class processor_old_internals
{
public:
    processor_old_internals(size_t count)
        : m_policy(pending_policy::allow_new)
        , m_thread_count(count)
        , m_busy_count(0)
        , m_i_task_count(0)
    {}

    processor_old_internals(processor_old_internals const&) = delete;
    processor_old_internals(processor_old_internals&&) = delete;
    processor_old_internals() = delete;

    processor_old_internals const& operator = (processor_old_internals const&) = delete;
    processor_old_internals const& operator = (processor_old_internals&&) = delete;

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
 * processor_old
 */
template <typename task_t>
processor_old<task_t>::processor_old(size_t count)
    : m_pimpl(new detail::processor_old_internals<task_t>(count))
{
    setCoreCount(count);
}

template <typename task_t>
processor_old<task_t>::~processor_old()
{
    try
    {
        setCoreCount(0);
    }
    catch(...){}
}

template <typename task_t>
void processor_old<task_t>::setCoreCount(size_t count)
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
        void (*fptrloop)(size_t, processor_old<task_t>&);
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
size_t processor_old<task_t>::getCoreCount()
{
    std::lock_guard<mutex> lock(m_pimpl->m_mutex);
    return m_pimpl->m_threads.size();
}

template <typename task_t>
void processor_old<task_t>::run(processor_old<task_t>::task const& obtask)
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
void processor_old<task_t>::wait(size_t i_task_count/* = 0*/)
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
inline void loop(size_t id, processor_old<task_t>& host) noexcept
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

template class beltpp::processor_old<beltpp::ctask>;
template class beltpp::processor_old<std::function<void()>>;

///
/// Experimental new processor implementation
/// should behave similar to a stream
///

namespace beltpp
{
class processor_ex;

class processor_event_handler_ex : public event_handler
{
public:
    processor_event_handler_ex();
    ~processor_event_handler_ex() override;

    wait_result wait(std::unordered_set<event_item const*>& set_items) override;
    std::unordered_set<uint64_t> waited(event_item& ev_it) const override;

    void wake() override;
    void set_timer(std::chrono::steady_clock::duration const& period) override;

    void add(event_item& ev_it) override;
    void remove(event_item& ev_it) override;

    processor_ex* ev_it;
};

void loop_ex(size_t id, processor_ex& host) noexcept;

class processor_ex : public processor
{
public:
    using peer_id = std::string;
    using packets = std::list<packet>;

    processor_ex(event_handler& eh,
                 size_t count,
                 std::function<beltpp::packet(beltpp::packet&&)> const& worker);
    processor_ex(processor_ex const&) = delete;
    processor_ex(processor_ex&&) = delete;
    ~processor_ex() noexcept override;

    processor_ex& operator = (processor_ex const&) = delete;
    processor_ex& operator = (processor_ex&&) = delete;

    void prepare_wait() override {};
    void timer_action() override {};

    packets receive(peer_id& peer) override;
    void send(peer_id const& peer, packet&& pack) override;

    //  this function is kept hidden from interface
    //  it waits until all tasks are done, seems not needed for
    //  this usecase
    void wait(size_t i_task_count = 0);

    void setCoreCount(size_t count) override;
    size_t getCoreCount() override;

    pending_policy m_policy;
    size_t m_thread_count;
    size_t m_busy_count;
    size_t m_i_task_count;
    std::function<beltpp::packet(beltpp::packet&&)> m_worker;
    threads m_threads;
    mutex m_mutex;
    cv m_cv_run_exit__loop;
    cv m_cv_loop__wait;
    cv m_cv_loop__output_ready;
    beltpp::queue<beltpp::packet> m_input_queue;
    beltpp::queue<beltpp::packet> m_output_queue;
};

processor_ex::processor_ex(event_handler& eh,
                           size_t count,
                           std::function<beltpp::packet(beltpp::packet&&)> const& worker)
    : processor(eh)
    , m_policy(pending_policy::allow_new)
    , m_thread_count(count)
    , m_busy_count(0)
    , m_i_task_count(0)
    , m_worker(worker)

{
    setCoreCount(count);
}

processor_ex::~processor_ex() noexcept
{
    try
    {
        setCoreCount(0);
    }
    catch(...){}
}

void processor_ex::setCoreCount(size_t count)
{
    {
        std::lock_guard<mutex> lock(m_mutex);
        m_thread_count = count;
    }
    size_t current_thread_count = 0;
    size_t desired_thread_count = 0;
    {
        std::lock_guard<mutex> lock(m_mutex);
        current_thread_count = m_threads.size();
        desired_thread_count = m_thread_count;
    }
    while (current_thread_count < desired_thread_count)
    {
        m_threads.push_back(
                    std::thread(&loop_ex,
                                current_thread_count,
                                std::ref(*this)));

        ++current_thread_count;
    }
    if (current_thread_count > desired_thread_count)
        m_cv_run_exit__loop.notify_all();  //  notify all loop_ex() to wake up from empty state sleep
    while (current_thread_count > desired_thread_count)
    {
        m_threads.back().join();
        m_threads.pop_back();
        --current_thread_count;
    }
}

size_t processor_ex::getCoreCount()
{
    std::lock_guard<mutex> lock(m_mutex);
    return m_threads.size();
}

processor_ex::packets processor_ex::receive(peer_id&/* peer*/)
{
    packets result;

    std::lock_guard<mutex> lock(m_mutex);
    while (false == m_output_queue.empty())
    {
        result.push_back(std::move(m_output_queue.front()));
        m_output_queue.pop();
    }

    return result;
}
void processor_ex::send(peer_id const&/* peer*/, packet&& pack)
{
    bool bNotify = false;
    {
        std::lock_guard<mutex> lock(m_mutex);
        if (m_policy == pending_policy::allow_new)
        {
            m_input_queue.push(std::move(pack)); //  may throw, that's ok
            ++m_i_task_count;
            bNotify = true;
        }
    }
    if (bNotify)
        m_cv_run_exit__loop.notify_one();  //  notify one loop_ex() to wake up from empty wait
}

void processor_ex::wait(size_t i_task_count/* = 0*/)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cv_loop__wait.wait(lock, [this, i_task_count]
    {
        if (m_input_queue.empty() &&
            0 == m_busy_count &&
            m_i_task_count >= i_task_count)
        {
            m_i_task_count = 0;
            return true;
        }
        return false;
    });
}

void loop_ex(size_t id, processor_ex& host) noexcept
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(host.m_mutex);

        if (host.m_input_queue.empty())
        {
            if (host.m_thread_count < id + 1)
                return;

            {
                host.m_cv_run_exit__loop.wait(lock,
                                              [&host,
                                              id]
                {
                    return (false == host.m_input_queue.empty() ||  //  one wait for new message to process (from send)
                            host.m_thread_count < id + 1);          //  all wait for when need to stop the thread
                });
            }
        }

        if (false == host.m_input_queue.empty())
        {
            beltpp::packet package(std::move(host.m_input_queue.front()));
            host.m_input_queue.pop();
            ++host.m_busy_count;

            lock.unlock();

            beltpp::packet response;

            try
            {
                if (pending_policy::empty_queue != host.m_policy)
                    response = host.m_worker(std::move(package));
            }
            catch (...){}

            lock.lock();

            --host.m_busy_count;
            if (host.m_input_queue.empty() &&
                0 == host.m_busy_count)
            {
                host.m_cv_loop__wait.notify_one();
            }

            if (false == response.empty())
            {
                host.m_output_queue.push(std::move(response));
                host.m_cv_loop__output_ready.notify_one();
            }
            lock.unlock();
        }
    }
}

processor_event_handler_ex::processor_event_handler_ex()
    : event_handler()
    , ev_it(nullptr)
{}
processor_event_handler_ex::~processor_event_handler_ex()
{}

event_handler::wait_result processor_event_handler_ex::wait(std::unordered_set<event_item const*>& /*set_items*/)
{
    if (nullptr == ev_it)
        throw std::logic_error("processor_event_handler_ex::wait");

    event_handler::wait_result result;

    std::unique_lock<std::mutex> lock(ev_it->m_mutex);

    ev_it->m_cv_loop__output_ready.wait(lock, [this, &result]
    {
        if (ev_it->m_output_queue.empty())
            result = event_handler::wait_result::event;
        else
            result = event_handler::wait_result::nothing;

        return true;
    });

    return result;
}
std::unordered_set<uint64_t> processor_event_handler_ex::waited(event_item&/* ev_it*/) const
{
    throw std::logic_error("processor_event_handler_ex::waited");
}

void processor_event_handler_ex::wake()
{
    if (nullptr == ev_it)
        throw std::logic_error("processor_event_handler_ex::wake");
    std::unique_lock<std::mutex> lock(ev_it->m_mutex);
    ev_it->m_cv_loop__output_ready.notify_one();
}
void processor_event_handler_ex::set_timer(std::chrono::steady_clock::duration const& /*period*/)
{
    throw std::logic_error("processor_event_handler_ex::set_timer");
}

void processor_event_handler_ex::add(event_item& _ev_it)
{
    if (ev_it)
        throw std::logic_error("processor_event_handler_ex::add: ev_it");
    processor_ex* p = dynamic_cast<processor_ex*>(&_ev_it);
    if (nullptr == p)
        throw std::logic_error("processor_event_handler_ex::add: nullptr == p");

    ev_it = p;
}
void processor_event_handler_ex::remove(event_item&/* ev_it*/)
{
    if (nullptr == ev_it)
        throw std::logic_error("processor_event_handler_ex::add: ev_it");

    ev_it = nullptr;
}

stream_ptr construct_processor(event_handler& eh,
                               size_t count,
                               std::function<beltpp::packet(beltpp::packet&&)> const& worker)
{
    beltpp::stream* p = new processor_ex(eh, count, worker);
    return beltpp::stream_ptr(p);
}
event_handler_ptr construct_event_handler()
{
    beltpp::event_handler* p = new processor_event_handler_ex();
    return beltpp::event_handler_ptr(p);
}

}
