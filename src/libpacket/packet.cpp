#include "packet.hpp"

#include <vector>
#include <cstring>
#include <memory>

namespace beltpp
{
using vector_buffer = std::vector<char>;
using ptr_msg = packet::ptr_msg;
using fptr_saver = packet::fptr_saver;
/*
 * internals
 */
namespace detail
{

class packet_internals
{
public:
    packet_internals()
        : m_rtt(-1)
        , m_ptr_message_code(nullptr, [](void*&){})
        , m_fsaver(nullptr)
    {}
    ~packet_internals() noexcept
    {
        m_ptr_message_code.reset();
    }

    size_t m_rtt;
    ptr_msg m_ptr_message_code;
    fptr_saver m_fsaver;
};

}
/*
 * packet
 */
packet::packet()
    : m_pimpl(new detail::packet_internals())
{
}

packet::packet(packet&& other)
    : m_pimpl(std::move(other.m_pimpl))
{
}

packet::~packet()
{
}

size_t packet::type() const
{
    return m_pimpl->m_rtt;
    return 0;
}

void packet::clean()
{
    m_pimpl.reset(new detail::packet_internals());
}

std::vector<char> packet::save() const
{
    fptr_saver& fsaver = m_pimpl->m_fsaver;
    if (nullptr == fsaver)
        throw std::runtime_error("packet::save() on empty message");

    return fsaver(m_pimpl->m_ptr_message_code.get());
}

void packet::set(size_t rtt,
                  ptr_msg pmsg,
                  fptr_saver fsaver)
{
    _set_internal(rtt,
                  std::move(pmsg),
                  fsaver);
}

void const* packet::data() const noexcept
{
    return _get_internal();
}

void const* packet::_get_internal() const noexcept
{
    return m_pimpl->m_ptr_message_code.get();
}

void packet::_set_internal(size_t rtt,
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


