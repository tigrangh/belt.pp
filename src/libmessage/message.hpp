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
    class message_internals;
}

class MESSAGESHARED_EXPORT message
{
public:
    using fptr_deleter = detail::fptr_deleter;
    using fptr_saver = detail::fptr_saver;
    using ptr_msg = detail::ptr_msg;

    message();
    message(message&& other);
    virtual ~message();

    template <typename MessageValue>
    message(MessageValue&& msg) :
        message()
    {
        set(std::forward<MessageValue>(msg));
    }

    message& operator = (message const&) = delete;
    message& operator = (message&&) = delete;

    size_t type() const;
    void clean();
    std::vector<char> save() const;
    void set(size_t rtt,
             ptr_msg pmsg,
             fptr_saver fsaver);

    template <typename MessageValue>
    void set(MessageValue&& msg);

    template <typename MessageValue>
    void get(MessageValue& msg) const;

protected:
    void const* _get_internal() const noexcept;
    void _set_internal(size_t rtt,
                       ptr_msg pmsg,
                       fptr_saver fsaver) noexcept;

    std::unique_ptr<detail::message_internals> m_pimpl;
};

template <typename MessageValue>
void message::set(MessageValue&& msg)
{
    using MessageValueT = typename std::remove_reference<MessageValue>::type;
    ptr_msg pmsg(MessageValueT::creator());
    void* pv = pmsg.get();
    MessageValueT* pmv = static_cast<MessageValueT*>(pv);
    MessageValueT& ref = *pmv;
    ref = std::forward<MessageValue>(msg);
    set(MessageValueT::rtt,
        std::move(pmsg),
        &MessageValueT::saver);
}

template <typename MessageValue>
void message::get(MessageValue& msg) const
{
    auto rtt = type();
    if (rtt == MessageValue::rtt)
    {
        msg = *reinterpret_cast<MessageValue const*>(_get_internal());
    }
    /*else
        msg = MessageValue::convert(_get_internal());*/
}

}
