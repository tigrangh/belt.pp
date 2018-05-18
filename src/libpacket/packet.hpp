#pragma once

#include "global.hpp"

#include <belt.pp/message_global.hpp>

#include <memory>
#include <vector>
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
    packet(packet&& other);
    virtual ~packet();

    template <typename T_message>
    packet(T_message&& msg) : packet()
    {
        set(std::forward<T_message>(msg));
    }

    packet& operator = (packet const&) = delete;
    packet& operator = (packet&&) noexcept;

    size_t type() const;
    void clean();
    std::vector<char> save() const;
    void set(size_t rtt,
             beltpp::void_unique_ptr pmsg,
             fptr_saver fsaver);

    template <typename T_message>
    void set(T_message&& msg);

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
    using MessageValueT = typename std::remove_reference<T_message>::type;
    beltpp::void_unique_ptr pmsg(new_void_unique_ptr<MessageValueT>());
    void* pv = pmsg.get();
    MessageValueT* pmv = static_cast<MessageValueT*>(pv);
    MessageValueT& ref = *pmv;
    ref = std::forward<T_message>(msg);
    set(MessageValueT::rtt,
        std::move(pmsg),
        &MessageValueT::saver);
}

template <typename T_message>
void packet::get(T_message& msg) const &
{
    auto rtt = type();
    if (rtt == T_message::rtt)
    {
        msg = *reinterpret_cast<T_message const*>(_get_internal());
    }
    else
        throw std::runtime_error("wrong type on get from packet");
}

template <typename T_message>
void packet::get(T_message& msg) &&
{
    auto rtt = type();
    if (rtt == T_message::rtt)
    {
        T_message* p = reinterpret_cast<T_message*>(_get_internal());
        msg = std::move(*p);
    }
    else
        throw std::runtime_error("wrong type on get from packet");
}

}
