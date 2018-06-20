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

namespace native
{
    class sk_handle;
}

class SOCKETSHARED_EXPORT event_handler : public ievent_handler
{
public:
    friend class ievent_item;

    event_handler();
    virtual ~event_handler();

    wait_result wait(std::unordered_set<ievent_item const*>& set_items) override;
    std::unordered_set<uint64_t> waited(ievent_item& ev_it) const override;

    void set_timer(std::chrono::steady_clock::duration const& period) override;

    void add(ievent_item& ev_it) override;
    void remove(ievent_item& ev_it) override;

    uint64_t add(ievent_item& ev_it, native::sk_handle const& handle, uint64_t id, bool out, bool close);
    void remove(native::sk_handle const& handle, uint64_t id, bool already_closed, bool out);
    void reset(uint64_t reset_id);
private:
    std::unique_ptr<detail::event_handler_impl> m_pimpl;
};
}
