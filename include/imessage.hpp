#ifndef BELT_IMESSAGE_H
#define BELT_IMESSAGE_H

#include <string>
#include <vector>

namespace beltpp
{

class imessage
{
public:
    enum class message_type
    {
        empty,
        joined,
        drop,
        dropped,
        leaving,
        session_start,
        session_started,
        session_join,
        session_joined,
        communicate
    };

    imessage() {};
    virtual ~imessage() {};

    virtual void clean() = 0;
    virtual message_type get_type() const = 0;
    virtual void get_buffer(char const* &p_buffer,
                            size_t& size) const = 0;
    virtual void set(message_type type,
                     char const* buffer = nullptr,
                     size_t size = 0) = 0;

    virtual size_t scan(std::vector<char const*, size_t> const& arr_buffer) const = 0;
};

}

#endif // BELT_IMESSAGE_H
