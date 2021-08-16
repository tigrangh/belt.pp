#include "event.hpp"

#include <mutex>
#include <unordered_set>

namespace beltpp_socket_impl
{
using namespace beltpp;

using std::unordered_set;

event_handler_ex::event_handler_ex()
    : m_impl()
{
}

event_handler_ex::~event_handler_ex() = default;

event_handler_ex::wait_result event_handler_ex::wait(std::unordered_set<event_item const*>& set_items)
{
    set_items.clear();
    m_impl.m_event_item_ids.clear();

    for (auto& event_item_in_set : m_impl.m_event_items)
    {
        event_item_in_set->prepare_wait();
    }

    if (m_impl.m_timer_helper.expired())
    {
        m_impl.m_timer_helper.update();
        return event_handler_ex::timer_out;
    }

    std::unordered_set<uint64_t> set_ids;
    {
        std::lock_guard<std::mutex> lock(m_impl.m_mutex);
        auto it = m_impl.sync_eh_ids.begin();
        if (it != m_impl.sync_eh_ids.end())
        {
            set_ids.insert(*it);
            m_impl.sync_eh_ids.erase(it);
        }
    }

    bool on_demand = false;
    if (set_ids.empty())
        set_ids = m_impl.m_poll_master.wait(m_impl.m_timer_helper, on_demand);

    std::lock_guard<std::mutex> lock(m_impl.m_mutex);
    auto it = set_ids.begin();
    while (it != set_ids.end())
    {
        auto id = *it;
        bool found = false;
        for (auto const& ids_item : m_impl.m_ids)
        {
            if (ids_item.valid_index(id))
            {
                auto& ref_item = ids_item[id];
                event_item* pitem = ref_item.m_pitem;
                set_items.insert(pitem);
                found = true;

                auto& item = m_impl.m_event_item_ids[pitem];
                item.insert(ref_item.m_item_id);

                break;
            }
        }

        if (false == found)
            it = set_ids.erase(it);
        else
            ++it;
    }

    bool on_timer = m_impl.m_timer_helper.expired();
    bool on_event = (false == set_ids.empty());

    if (on_timer)
        m_impl.m_timer_helper.update();

    if (false == on_demand &&
        false == on_timer &&
        false == on_event)
        return event_handler_ex::nothing;

    if (on_demand &&
        false == on_timer &&
        false == on_event)
        return event_handler_ex::on_demand;
    if (false == on_demand &&
        on_timer &&
        false == on_event)
        return event_handler_ex::timer_out;
    if (false == on_demand &&
        false == on_timer &&
        on_event)
        return event_handler_ex::event;

    if (on_demand &&
        on_timer &&
        false == on_event)
        return event_handler_ex::on_demand_and_timer_out;
    if (on_demand &&
        false == on_timer &&
        on_event)
        return event_handler_ex::on_demand_and_event;
    if (false == on_demand &&
        on_timer &&
        on_event)
        return event_handler_ex::timer_out_and_event;

    /*if (on_demand &&
        on_timer &&
        on_event)*/
    return event_handler_ex::on_demand_and_timer_out_and_event;
}

std::unordered_set<uint64_t> event_handler_ex::waited(event_item& ev_it) const
{
    return m_impl.m_event_item_ids.at(&ev_it);
}

void event_handler_ex::wake()
{
    m_impl.m_poll_master.wake();
}

void event_handler_ex::set_timer(std::chrono::steady_clock::duration const& period)
{
    m_impl.m_timer_helper.set(period);
}

void event_handler_ex::add(event_item& ev_it)
{
    m_impl.m_event_items.insert(&ev_it);
}
void event_handler_ex::remove(event_item& ev_it)
{
    m_impl.m_event_items.erase(&ev_it);
}
}// beltpp_socket_impl



