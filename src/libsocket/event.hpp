#pragma once

#include "global.hpp"
#include "event_impl.hpp"

#include <belt.pp/ievent.hpp>

#include <chrono>
#include <unordered_set>

namespace beltpp_socket_impl
{
using namespace beltpp;

class event_handler_ex : public event_handler
{
public:
    event_handler_ex();
    ~event_handler_ex() override;

    wait_result wait(std::unordered_set<event_item const*>& set_items) override;
    std::unordered_set<uint64_t> waited(event_item& ev_it) const override;

    void wake() override;
    void set_timer(std::chrono::steady_clock::duration const& period) override;

    void add(event_item& ev_it) override;
    void remove(event_item& ev_it) override;

    detail::event_handler_impl m_impl;
};
}// beltpp_socket_impl
