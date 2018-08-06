#pragma once

#include "poll_master.hpp"

#include <belt.pp/queue.hpp>
#include <belt.pp/scope_helper.hpp>
#include <belt.pp/timer.hpp>

#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

using beltpp::on_failure;

using std::list;
using std::unordered_map;
using std::unordered_set;

namespace beltpp
{
namespace detail
{
class event_slot
{
public:
    event_slot(uint64_t item_id = 0, ievent_item* pitem = nullptr)
        : m_item_id(item_id)
        , m_pitem(pitem)
    {}

    bool m_closed = false;
    uint64_t m_item_id = 0;
    ievent_item* m_pitem = nullptr;
};
using event_slots = beltpp::queue<event_slot>;

class event_handler_impl
{
public:
    beltpp::timer m_timer_helper;
    poll_master m_poll_master;
    list<event_slots> m_ids;
    std::mutex m_mutex;
    unordered_map<ievent_item*, unordered_set<uint64_t>> m_event_item_ids;
    unordered_set<ievent_item*> m_event_items;
    unordered_set<uint64_t> sync_eh_ids;

    inline uint64_t add(ievent_item& ev_it,
                        native::socket_handle::handle_type handle,
                        uint64_t id,
                        event_handler::task action,
                        bool reuse_slot = false,
                        uint64_t slot_id = 0);
    inline void remove(native::socket_handle::handle_type handle,
                       uint64_t id, bool already_closed,
                       event_handler::task action,
                       bool keep_slot = false);
    inline void reset(uint64_t reset_id);
    inline void set_sync_result(uint64_t eh_id);
};

inline uint64_t event_handler_impl::add(ievent_item& ev_it,
                                        native::socket_handle::handle_type handle,
                                        uint64_t item_id,
                                        event_handler::task action,
                                        bool reuse_slot/* = false*/,
                                        uint64_t slot_id/* = 0*/)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& ids = m_ids;
    if (ids.empty())
        ids.emplace_back(detail::event_slots(16));
    auto it_ids = ids.end();
    --it_ids;

    uint64_t id = 0;
    if (reuse_slot)
    {
        assert(it_ids->valid_index(slot_id));
        id = slot_id;
    }
    else
    {
        if (it_ids->full())
        {
            ids.emplace_back(detail::event_slots(it_ids->size() * 2,
                it_ids->end_index()));
            it_ids = ids.end();
            --it_ids;
        }
        id = it_ids->end_index();
    }

    m_poll_master.add(handle, id, action);

    beltpp::on_failure scope_guard(
        [this, handle, id, action]
    {
        m_poll_master.remove(handle,
                             id,
                             false,  //  already_closed
                             action);
    });

    if (reuse_slot)
    {
        detail::event_slot& current_event_slot =
            it_ids->operator [](id);

        current_event_slot = detail::event_slot(item_id, &ev_it);
    }
    else
        it_ids->push(detail::event_slot(item_id, &ev_it));
    scope_guard.dismiss();

    return id;
}

inline void event_handler_impl::remove(native::socket_handle::handle_type handle,
                                       uint64_t id,
                                       bool already_closed,
                                       event_handler::task action,
                                       bool keep_slot/* = false*/)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it_ids = m_ids.begin();
    for (; it_ids != m_ids.end(); ++it_ids)
    {
        if (it_ids->valid_index(id))
        {
            detail::event_slot& current_event_slot =
                it_ids->operator [](id);

            if (current_event_slot.m_closed)
            {
                assert(false);
                throw std::runtime_error("event_handler::remove");
            }

            current_event_slot.m_closed = true;

            m_poll_master.remove(handle,
                                 id,
                                 already_closed,
                                 action);

            if (keep_slot == false)
            {
                while (false == it_ids->empty())
                {
                    if (false == it_ids->front().m_closed)
                        break;
                    it_ids->pop();
                }
            }

            auto it_ids_check = it_ids;
            ++it_ids_check;
            if (it_ids->empty() &&
                it_ids_check != m_ids.end())
                m_ids.erase(it_ids);

            return;
        }
    }

    assert(false);
    throw std::runtime_error("event_handler::remove");
}

inline void event_handler_impl::reset(uint64_t reset_id)
{
    m_poll_master.reset(reset_id);
}

inline void event_handler_impl::set_sync_result(uint64_t eh_id)
{
    sync_eh_ids.insert(eh_id);
}
}   //  end namespace detail
}   //  end namespace beltpp
