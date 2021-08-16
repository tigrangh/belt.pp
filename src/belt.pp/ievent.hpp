#pragma once

#include "global.hpp"

#include <chrono>
#include <unordered_set>

namespace beltpp
{

class event_handler;

class event_item
{
public:
    event_item(event_handler&) {}
    virtual ~event_item() {}

    virtual void prepare_wait() = 0;
    virtual void timer_action() = 0;
};

class event_handler
{
public:
    event_handler() {}
    virtual ~event_handler() {}

    enum wait_result
    {
        nothing = 0x0,
        on_demand = 0x1,
        timer_out = 0x2,
            on_demand_and_timer_out = 0x3,
        event = 0x4,
            on_demand_and_event = 0x5,
            timer_out_and_event = 0x6,
                on_demand_and_timer_out_and_event = 0x7,
    };

    virtual wait_result wait(std::unordered_set<event_item const*>& set_items) = 0;
    virtual std::unordered_set<uint64_t> waited(event_item& ev_it) const = 0;

    virtual void wake() = 0;
    virtual void set_timer(std::chrono::steady_clock::duration const& period) = 0;

    virtual void add(event_item& ev_it) = 0;
    virtual void remove(event_item& ev_it) = 0;
};

using event_handler_ptr = std::unique_ptr<event_handler>;

}
