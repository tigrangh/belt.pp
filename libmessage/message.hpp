#ifndef BELT_MESSAGE_H
#define BELT_MESSAGE_H

#include "global.hpp"

#include <belt.pp/message_global.hpp>

#include <memory>
#include <vector>
#include <cassert>

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

    size_t type() const;
    void clean();
    std::vector<char> save() const;
    void set(size_t rtt,
             ptr_msg pmsg,
             fptr_saver fsaver);

    template <typename MessageValue>
    void set(MessageValue const& msg);

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
void message::set(MessageValue const& msg)
{
    ptr_msg pmsg(MessageValue::creator());
    void* pv = pmsg.get();
    MessageValue* pmv = static_cast<MessageValue*>(pv);
    MessageValue& ref = *pmv;
    ref = msg;
    set(MessageValue::rtt,
        std::move(pmsg),
        &MessageValue::saver);
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

#endif // BELT_MESSAGE_H
