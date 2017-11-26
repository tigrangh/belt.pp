#include "message.hpp"

#include <vector>
#include <cstring>
#include <memory>

namespace beltpp
{
using vector_buffer = std::vector<char>;
using ptr_msg = message::ptr_msg;
using fptr_saver = message::fptr_saver;
/*
 * internals
 */
namespace detail
{

class message_internals
{
public:
    message_internals()
        : m_rtt(-1)
        , m_ptr_message_code(nullptr, [](void*&){})
        , m_fsaver(nullptr)
    {}
    ~message_internals() noexcept
    {
        m_ptr_message_code.reset();
    }

    size_t m_rtt;
    ptr_msg m_ptr_message_code;
    fptr_saver m_fsaver;
};

}
/*
 * message
 */
message::message()
    : m_pimpl(new detail::message_internals())
{
}

message::message(message&& other)
    : m_pimpl(std::move(other.m_pimpl))
{
}

message::~message()
{
}

size_t message::type() const
{
    return m_pimpl->m_rtt;
    return 0;
}

void message::clean()
{
    m_pimpl.reset(new detail::message_internals());
}

std::vector<char> message::save() const
{
    fptr_saver& fsaver = m_pimpl->m_fsaver;
    if (nullptr == fsaver)
        throw std::runtime_error("message::save() on empty message");

    return fsaver(m_pimpl->m_ptr_message_code.get());
}

void message::set(size_t rtt,
                  ptr_msg pmsg,
                  fptr_saver fsaver)
{
    _set_internal(rtt,
                  std::move(pmsg),
                  fsaver);
}

void const* message::_get_internal() const noexcept
{
    return m_pimpl->m_ptr_message_code.get();
}

void message::_set_internal(size_t rtt,
                            ptr_msg pmsg,
                            fptr_saver fsaver) noexcept
{
    m_pimpl->m_ptr_message_code = std::move(pmsg);
    m_pimpl->m_rtt = rtt;
    m_pimpl->m_fsaver = fsaver;
}

namespace detail
{
}

}


