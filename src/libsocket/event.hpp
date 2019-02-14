#pragma once

#include "global.hpp"

#include <belt.pp/ievent.hpp>

#include <chrono>
#include <memory>
#include <unordered_set>

namespace beltpp
{
namespace detail
{
    class event_handler_impl;
}

class SOCKETSHARED_EXPORT event_handler : public ievent_handler
{
public:
    enum class task {remove, accept, connect, receive, send};
    friend class ievent_item;

    event_handler();
    event_handler(event_handler&&);
    ~event_handler() override;

    wait_result wait(std::unordered_set<ievent_item const*>& set_items) override;
    std::unordered_set<uint64_t> waited(ievent_item& ev_it) const override;

    void terminate() override;
    void set_timer(std::chrono::steady_clock::duration const& period) override;

    void add(ievent_item& ev_it) override;
    void remove(ievent_item& ev_it) override;

    std::unique_ptr<detail::event_handler_impl> m_pimpl;
};
}
