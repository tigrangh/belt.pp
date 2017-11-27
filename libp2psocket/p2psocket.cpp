#include "p2psocket.hpp"

#include <socket.hpp>

//#include <scope_helper.hpp>
//#include <queue.hpp>
#include <processor.hpp>
#include <message.hpp>

//#include <exception>
//#include <unordered_map>
//#include <unordered_set>
#include <cassert>
#include <utility>
//#include <mutex>
//#include <memory>
//#include <list>
//
using string = std::string;
using peer_ids = beltpp::p2psocket::peer_ids;
using messages = beltpp::p2psocket::messages;

namespace beltpp
{

/*
 * p2psocket_internals
 */
namespace detail
{

class p2psocket_internals
{
public:
    p2psocket_internals(size_t rtt_join,
                        size_t rtt_drop,
                        size_t rtt_peer_info,
                        detail::fptr_creator fcreator_join,
                        detail::fptr_creator fcreator_drop,
                        detail::fptr_creator fcreator_peer_info,
                        detail::fptr_saver fsaver_join,
                        detail::fptr_saver fsaver_drop,
                        detail::fptr_saver fsaver_peer_info,
                        detail::fptr_message_loader fmessage_loader)
                        //std::vector<beltpp::address> const& addresses)
        : m_rtt_join(rtt_join)
        , m_rtt_drop(rtt_drop)
        , m_rtt_peer_info(rtt_peer_info)
        , m_fcreator_join(fcreator_join)
        , m_fcreator_drop(fcreator_drop)
        , m_fcreator_peer_info(fcreator_peer_info)
        , m_fsaver_join(fsaver_join)
        , m_fsaver_drop(fsaver_drop)
        , m_fsaver_peer_info(fsaver_peer_info)
        , m_fmessage_loader(fmessage_loader)
        , m_addresses()
        , m_socket(rtt_join,
                   rtt_drop,
                   fcreator_join,
                   fcreator_drop,
                   fsaver_join,
                   fsaver_drop,
                   fmessage_loader)
    {}

    size_t m_rtt_join;
    size_t m_rtt_drop;
    size_t m_rtt_peer_info;
    detail::fptr_creator m_fcreator_join;
    detail::fptr_creator m_fcreator_drop;
    detail::fptr_creator m_fcreator_peer_info;
    detail::fptr_saver m_fsaver_join;
    detail::fptr_saver m_fsaver_drop;
    detail::fptr_saver m_fsaver_peer_info;
    detail::fptr_message_loader m_fmessage_loader;
    std::vector<beltpp::address> const m_addresses;
    beltpp::socket m_socket;
};

}
/*
 * p2psocket
 */
p2psocket::p2psocket(size_t _rtt_join,
                     size_t _rtt_drop,
                     size_t _rtt_peer_info,
                     detail::fptr_creator _fcreator_join,
                     detail::fptr_creator _fcreator_drop,
                     detail::fptr_creator _fcreator_peer_info,
                     detail::fptr_saver _fsaver_join,
                     detail::fptr_saver _fsaver_drop,
                     detail::fptr_saver _fsaver_peer_info,
                     detail::fptr_message_loader _fmessage_loader)
                     //std::vector<beltpp::address> const& addresses)
    : isocket()
    , m_pimpl(new detail::p2psocket_internals(_rtt_join,
                                              _rtt_drop,
                                              _rtt_peer_info,
                                              _fcreator_join,
                                              _fcreator_drop,
                                              _fcreator_peer_info,
                                              _fsaver_join,
                                              _fsaver_drop,
                                              _fsaver_peer_info,
                                              _fmessage_loader))
                                              //addresses))
{}

p2psocket::p2psocket(p2psocket&&) = default;

p2psocket::~p2psocket()
{
}

messages p2psocket::read(peer_id& peer)
{
    return messages();
}

void p2psocket::write(peer_id const& peer,
                      message const& msg)
{
}

namespace detail
{
}// end detail

}// end beltpp


