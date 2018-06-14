#pragma once

#include "global.hpp"

#include <chrono>
#include <unordered_set>

namespace beltpp
{

class ievent_handler;

class ievent_item
{
public:
    friend class ievent_handler;
    ievent_item(ievent_handler&) {}
    virtual ~ievent_item() {}

protected:
    virtual void prepare_wait() = 0;
    virtual void timer_action() = 0;
};

class ievent_handler
{
public:
    ievent_handler() {}
    virtual ~ievent_handler() {}

    enum wait_result {nothing, timer_out, event};

    virtual wait_result wait(std::unordered_set<ievent_item const*>& set_items) = 0;
    virtual std::unordered_set<uint64_t> waited(ievent_item& ev_it) const = 0;

    virtual void set_timer(std::chrono::steady_clock::duration const& period) = 0;

    virtual void add(ievent_item& ev_it) = 0;
    virtual void remove(ievent_item& ev_it) = 0;
};

}
