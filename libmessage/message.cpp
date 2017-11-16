#include "message.hpp"

#include <vector>
#include <cstring>

namespace beltpp
{
using vector_buffer = std::vector<char>;
/*
 * internals
 */
namespace detail
{

class message_internals
{
public:
    message_internals()
        : m_type(message::message_type::empty)
    {}

    message::message_type m_type;
    vector_buffer m_buffer;
};

}
/*
 * message
 */
message::message()
    : m_pimpl(new detail::message_internals())
{
}

message::~message()
{
}

void message::clean()
{
    m_pimpl->m_type = message_type::empty;
    m_pimpl->m_buffer.resize(0);
}

message::message_type message::get_type() const
{
    return m_pimpl->m_type;
}

void message::get_buffer(char const* &p_buffer,
                         size_t& size) const
{
    size = m_pimpl->m_buffer.size();

    if (size)
        p_buffer = &m_pimpl->m_buffer[0];
    else
        p_buffer = nullptr;
}

void message::set(message::message_type type,
                  char const* buffer/* = nullptr*/,
                  size_t size/* = 0*/)
{
    m_pimpl->m_type = type;

    if (buffer && size)
    {
        m_pimpl->m_buffer.resize(size);
        std::memcpy(&m_pimpl->m_buffer[0], buffer, size);
    }
    else
        m_pimpl->m_buffer.resize(0);
}

size_t message::scan(std::vector<char const*, size_t> const& arr_buffer) const
{
    return 0;
}

namespace detail
{
}

}


