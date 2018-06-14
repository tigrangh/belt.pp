#include "event.hpp"
#include "poll_master.hpp"
#include "native.hpp"

#include <belt.pp/queue.hpp>
#include <belt.pp/scope_helper.hpp>

#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace beltpp
{

using std::list;
using std::unordered_map;
using std::unordered_set;
namespace detail
{
class event_slot;
using event_slots = beltpp::queue<event_slot>;
}
using beltpp::scope_helper;

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

class event_handler_impl
{
public:
    timer_helper m_timer_helper;
    poll_master m_poll_master;
    list<event_slots> m_ids;
    std::mutex m_mutex;
    unordered_map<ievent_item*, unordered_set<uint64_t>> m_event_item_ids;
    unordered_set<ievent_item*> m_event_items;
};
}

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

    std::unordered_set<uint64_t> set_ids = m_pimpl->m_poll_master.wait(m_pimpl->m_timer_helper);

    if (m_pimpl->m_timer_helper.expired())
    {
        m_pimpl->m_timer_helper.update();
        return event_handler::timer_out;
    }

    std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);
    for (uint64_t id : set_ids)
    {
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
            throw std::runtime_error("event_handler::wait");
    }

    if (set_ids.empty())
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
        assert(false);
        throw std::runtime_error("event_handler::waited");
    }
    else
        return find_it->second;
}

void event_handler::set_timer(std::chrono::steady_clock::duration const& period)
{
    m_pimpl->m_timer_helper.set(period);
}

uint64_t event_handler::add(ievent_item& ev_it, native::sk_handle const& handle, uint64_t item_id, bool out)
{
    std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);

    auto& ids = m_pimpl->m_ids;
    if (ids.empty())
        ids.emplace_back(detail::event_slots(16));
    auto it_ids = ids.end();
    --it_ids;
    if (it_ids->full())
    {
        ids.emplace_back(detail::event_slots(it_ids->size() * 2,
                                             it_ids->end_index()));
        it_ids = ids.end();
        --it_ids;
    }

    uint64_t id = it_ids->end_index();

    m_pimpl->m_poll_master.add(handle.handle, id, out);

    beltpp::scope_helper scope_guard([]{},
    [this, handle, id, out]
    {
        m_pimpl->m_poll_master.remove(handle.handle,
                                      id,
                                      false,  //  already_closed
                                      out);
    });

    it_ids->push(detail::event_slot(item_id, &ev_it));
    scope_guard.commit();

    return id;
}

void event_handler::remove(native::sk_handle const& handle, uint64_t id, bool already_closed, bool out)
{
    std::lock_guard<std::mutex> lock(m_pimpl->m_mutex);

    auto it_ids = m_pimpl->m_ids.begin();
    for (; it_ids != m_pimpl->m_ids.end(); ++it_ids)
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

            m_pimpl->m_poll_master.remove(handle.handle,
                                          id,
                                          already_closed,
                                          out);

            while (false == it_ids->empty())
            {
                if (false == it_ids->front().m_closed)
                    break;
                it_ids->pop();
            }

            auto it_ids_check = it_ids;
            ++it_ids_check;
            if (it_ids->empty() &&
                it_ids_check != m_pimpl->m_ids.end())
                m_pimpl->m_ids.erase(it_ids);

            return;
        }
    }

    assert(false);
    throw std::runtime_error("event_handler::remove");
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


