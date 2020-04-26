#pragma once

#include "stream.hpp"

#include <cstddef>
#include <functional>

namespace beltpp
{
template <typename task_t>
class iprocessor_old
{
public:
    iprocessor_old() = default;
    iprocessor_old(size_t count) = delete;
    iprocessor_old(iprocessor_old const&) = delete;
    iprocessor_old(iprocessor_old&&) = delete;
    virtual ~iprocessor_old() = default;

    iprocessor_old& operator = (iprocessor_old const&) = delete;
    iprocessor_old& operator = (iprocessor_old&&) = delete;

    virtual void setCoreCount(size_t count) = 0;
    virtual size_t getCoreCount() = 0;
    virtual void run(task_t const& th) = 0;  // add
    virtual void wait(size_t i_task_count = 0) = 0;
};

class processor : public stream
{
public:
    using peer_id = std::string;
    using packets = std::list<packet>;

    processor(event_handler& eh) : stream(eh) {}
    ~processor() override {}

    processor& operator = (processor const&) = delete;
    processor& operator = (processor&&) = delete;

    void prepare_wait() override = 0;
    void timer_action() override = 0;

    packets receive(peer_id& peer) override = 0;
    void send(peer_id const& peer, packet&& pack) override = 0;

    virtual void setCoreCount(size_t count) = 0;
    virtual size_t getCoreCount() = 0;
};

}

