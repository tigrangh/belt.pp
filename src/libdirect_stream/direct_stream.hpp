#pragma once

#include "global.hpp"

#include <belt.pp/ievent.hpp>
#include <belt.pp/stream.hpp>
#include <belt.pp/queue.hpp>
#include <belt.pp/packet.hpp>

#include <mutex>
#include <unordered_map>
#include <utility>

namespace beltpp
{

class direct_channel
{
public:
    using peer_incoming_streams =
        std::unordered_map<
            stream::peer_id,
            queue<packet>
        >;
    using peer_eh_incoming_streams = std::pair<
        event_handler*,
        peer_incoming_streams
        >;
    std::unordered_map<
        stream::peer_id,
        peer_eh_incoming_streams
        > streams;
    std::mutex mutex;
};

DIRECT_STREAM_SHARED_EXPORT stream_ptr construct_direct_stream(stream::peer_id const& peerid,
                                                               event_handler& eh,
                                                               direct_channel& channel);
}
