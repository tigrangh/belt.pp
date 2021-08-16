#pragma once

#include "global.hpp"
#include "ievent.hpp"

#include <string>
#include <list>

namespace beltpp
{

class packet;

class stream : public event_item
{
public:
    using peer_id = std::string;
    using packets = std::list<packet>;

    stream(event_handler& eh) : event_item(eh) {}
    virtual ~stream() {}

    void prepare_wait() override = 0;
    void timer_action() override = 0;

    virtual packets receive(peer_id& peer) = 0;

    virtual void send(peer_id const& peer,
                      packet&& pack) = 0;

};

using stream_ptr = std::unique_ptr<stream>;

class stream_join
{
public:
    static const size_t rtt = size_t(-10);
    static std::string pvoid_saver(void*) { return std::string(); }
};
class stream_drop
{
public:
    static const size_t rtt = size_t(-11);
    static std::string pvoid_saver(void*) { return std::string(); }
};

class stream_protocol_error
{
public:
    static const size_t rtt = size_t(-12);
    static std::string pvoid_saver(void*) { return std::string(); }

    stream_protocol_error(std::string const& buf = std::string())
        : buffer(buf) {}

    std::string buffer;
};

}

