#include "event.hpp"
#include "native.hpp"
#include "poll_master.hpp"
#include "event_impl.hpp"

#include <mutex>
#include <unordered_set>

namespace beltpp
{

using std::unordered_set;

event_handler::event_handler()
    : m_pimpl(new detail::event_handler_impl())
{
}

event_handler::~event_handler()
{
}

event_handler::wait_result event_handler::wait(std::unordered_set<ievent_item const*>& set_items)
{
    set_items.clear();
    m_pimpl->m_event_item_ids.clear();

    for (auto& event_item_in_set : m_pimpl->m_event_items)
    {
        event_item_in_set->prepare_wait();
    }

    if (m_pimpl->m_timer_helper.expired())
    {
        m_pimpl->m_timer_helper.update();
        return event_handler::timer_out;
    }

    std::unordered_set<uint64_t> set_ids = m_pimpl->m_poll_master.wait(m_pimpl->m_timer_helper);

    std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);
    auto it = set_ids.begin();
    while (it != set_ids.end())
    {
        auto id = *it;
        bool found = false;
        for (auto const& ids_item : m_pimpl->m_ids)
        {
            if (ids_item.valid_index(id))
            {
                auto& ref_item = ids_item[id];
                ievent_item* pitem = ref_item.m_pitem;
                set_items.insert(pitem);
                found = true;

                auto& ref_event_item_ids = m_pimpl->m_event_item_ids;
                auto find_it = ref_event_item_ids.find(pitem);
                if (find_it == ref_event_item_ids.end())
                    ref_event_item_ids.insert(std::make_pair(pitem, unordered_set<uint64_t>{ref_item.m_item_id}));
                else
                    find_it->second.insert(ref_item.m_item_id);

                break;
            }
        }

        if (false == found)
            it = set_ids.erase(it);
        else
            ++it;
    }

    if (set_ids.empty() && m_pimpl->m_timer_helper.expired())
    {
        m_pimpl->m_timer_helper.update();
        return event_handler::timer_out;
    }
    else if (set_ids.empty())
        return event_handler::nothing;
    else
        return event_handler::event;
}

std::unordered_set<uint64_t> event_handler::waited(ievent_item& ev_it) const
{
    auto& ref_event_item_ids = m_pimpl->m_event_item_ids;
    auto find_it = ref_event_item_ids.find(&ev_it);
    if (find_it == ref_event_item_ids.end())
    {
        //assert(false);
        throw std::runtime_error("event_handler::waited");
    }
    else
        return find_it->second;
}

void event_handler::set_timer(std::chrono::steady_clock::duration const& period)
{
    m_pimpl->m_timer_helper.set(period);
}

void event_handler::add(ievent_item& ev_it)
{
    m_pimpl->m_event_items.insert(&ev_it);
}
void event_handler::remove(ievent_item& ev_it)
{
    m_pimpl->m_event_items.erase(&ev_it);
}
}


