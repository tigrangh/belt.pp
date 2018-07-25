#include "packet.hpp"

#include <vector>
#include <cstring>
#include <memory>

namespace beltpp
{
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
        , m_ptr_message(nullptr, [](void*){})
        , m_fsaver(nullptr)
    {}
    ~packet_internals() noexcept
    {
        m_ptr_message.reset();
    }

    size_t m_rtt;
    beltpp::void_unique_ptr m_ptr_message;
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
    other.clean();
}

packet::~packet()
{
}

packet& packet::operator = (packet&& other) noexcept
{
    m_pimpl = std::move(other.m_pimpl);
    other.clean();

    return *this;
}

bool packet::empty() const noexcept
{
    return m_pimpl->m_rtt == size_t(-1);
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

std::string packet::save() const
{
    fptr_saver& fsaver = m_pimpl->m_fsaver;
    if (nullptr == fsaver)
        return std::string();

    return fsaver(m_pimpl->m_ptr_message.get());
}

void packet::set(size_t rtt,
                 void_unique_ptr pmsg,
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
    return m_pimpl->m_ptr_message.get();
}

void* packet::_get_internal() noexcept
{
    return m_pimpl->m_ptr_message.get();
}

void packet::_set_internal(size_t rtt,
                           void_unique_ptr pmsg,
                           fptr_saver fsaver) noexcept
{
    m_pimpl->m_ptr_message = std::move(pmsg);
    m_pimpl->m_rtt = rtt;
    m_pimpl->m_fsaver = fsaver;
}

namespace detail
{
}

}


