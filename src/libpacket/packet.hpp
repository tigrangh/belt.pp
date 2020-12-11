#pragma once

#include "global.hpp"

#include <belt.pp/message_global.hpp>
#include <belt.pp/meta.hpp>

#include <memory>
#include <string>
#include <cassert>
#include <utility>
#include <type_traits>

namespace beltpp
{

namespace detail
{
    class packet_internals;
}

class PACKETSHARED_EXPORT packet
{
public:
    using fptr_saver = detail::fptr_saver;

    packet();
    packet(packet const&) = delete;
    packet(packet&& other);
    virtual ~packet();

    template <typename T_message>
    explicit packet(T_message&& msg) : packet()
    {
        set(std::forward<T_message>(msg));
    }

    packet& operator = (packet const&) = delete;
    packet& operator = (packet&&) noexcept;

    template <typename T_message>
    packet& operator = (T_message&& msg) noexcept
    {
        set(std::forward<T_message>(msg));
        return *this;
    }

    bool empty() const noexcept;
    size_t type() const;
    void clean();
    std::string to_string() const;
    void set(size_t rtt,
             beltpp::void_unique_ptr pmsg,
             fptr_saver fsaver);

    template <typename T_message>
    void set(T_message&& msg);

    template <typename T_message>
    void get(T_message const* &pmsg) const;

    template <typename T_message>
    void get(T_message* &pmsg);

    template <typename T_message>
    void get(T_message& msg) const &;

    template <typename T_message>
    void get(T_message& msg) &&;

    void const* data() const noexcept;

protected:
    void const* _get_internal() const noexcept;
    void* _get_internal() noexcept;
    void _set_internal(size_t rtt,
                       beltpp::void_unique_ptr pmsg,
                       fptr_saver fsaver) noexcept;

    std::unique_ptr<detail::packet_internals> m_pimpl;
};

template <typename T_message>
void packet::set(T_message&& msg)
{
    using MessageValueT_temp = typename std::remove_reference<T_message>::type;
    using MessageValueT = typename std::remove_const<MessageValueT_temp>::type;
    beltpp::void_unique_ptr pmsg(new_void_unique_ptr<MessageValueT>());
    void* pv = pmsg.get();
    MessageValueT* pmv = static_cast<MessageValueT*>(pv);
    MessageValueT& ref = *pmv;
    ref = std::forward<T_message>(msg);
    set(MessageValueT::rtt,
        std::move(pmsg),
        &MessageValueT::pvoid_saver);
}

template <typename T_message>
void packet::get(T_message* &pmsg)
{
    auto rtt = type();
    if (rtt == T_message::rtt)
    {
        pmsg = reinterpret_cast<T_message*>(_get_internal());
    }
    else
        throw std::runtime_error("wrong type on get from packet");
}

template <typename T_message>
void packet::get(T_message const* &pmsg) const
{
    auto rtt = type();
    if (rtt == T_message::rtt)
    {
        pmsg = reinterpret_cast<T_message const*>(_get_internal());
    }
    else
        throw std::runtime_error("wrong type on get from packet");
}

template <typename T_message>
void packet::get(T_message& msg) const &
{
    T_message const* p;
    get(p);
    msg = *p;
}

template <typename T_message>
void packet::get(T_message& msg) &&
{
    T_message* p;
    get(p);
    msg = std::move(*p);
}

template <int64_t... Vs>
class variant_packet
{
protected:
    packet p;
public:
    using static_set_t = beltpp::static_set::set<Vs...>;

    variant_packet()
        : p()
    {}
    variant_packet(variant_packet<Vs...>&& other)
        : p(std::move(*other))
    {}
    variant_packet(variant_packet<Vs...> const& other) = delete;

    variant_packet(packet&& other)
        : p(std::move(other))
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(p.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("variant_packet(packet&& other)");
    }

    void clean()
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(packet().type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("variant_packet::clean()");

        p.clean();
    }
    void set(packet&& other)
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(other.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("variant_packet::set(packet&& other)");

        p = std::move(other);
    }

    packet const& operator * () const
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(p.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("packet const& operator * () const");

        return p;
    }
    packet& operator * ()
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(p.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("packet& operator * ()");

        return p;
    }

    packet const* operator -> () const
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(p.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("packet const* operator -> () const");

        return &p;
    }
    packet* operator -> ()
    {
        int64_t index = static_set::find<static_set::set<Vs...>>::check(p.type());

        if (index == -1 && static_set::set<Vs...>::count != 0)
            throw std::logic_error("packet* operator -> ()");

        return &p;
    }
};

}

namespace std
{
    template<>
    struct hash<beltpp::packet>
    {
        inline size_t operator()(beltpp::packet const& value) const noexcept
        {
            std::hash<string> hasher;
            return hasher(value.to_string());
        }
    };
}
