#pragma once

#include "global.hpp"

#include <chrono>
#include <memory>
#include <unordered_set>

namespace beltpp
{
namespace detail
{
class event_handler_impl;
}

class event_handler;

class EVENTSHARED_EXPORT ievent_item
{
public:
    friend class event_handler;
    ievent_item(event_handler&) {}
    virtual ~ievent_item() {}

protected:
    virtual void prepare_wait() = 0;
    virtual void timer_action() = 0;
};

class EVENTSHARED_EXPORT event_handler
{
public:
    friend class ievent_item;

    event_handler();
    ~event_handler();

    enum wait_result {nothing, timer_out, event};

    wait_result wait(std::unordered_set<ievent_item const*>& set_items);
    std::unordered_set<uint64_t> waited(ievent_item& ev_it) const;

    void set_timer(std::chrono::steady_clock::duration const& period);

    uint64_t add(ievent_item& ev_it, int handle, uint64_t id, bool out);
    void remove(int handle, uint64_t id, bool already_closed, bool out);

    void add(ievent_item& ev_it);
    void remove(ievent_item& ev_it);
private:
    std::unique_ptr<detail::event_handler_impl> m_pimpl;
};

}
