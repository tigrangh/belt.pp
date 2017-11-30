#ifndef BELT_P2PSOCKET_H
#define BELT_P2PSOCKET_H

#include "global.hpp"
#include <socket.hpp>

#include <memory>
#include <vector>

namespace beltpp
{

class socket;

namespace detail
{
    class p2psocket_internals;
}

template <size_t _rtt_join,
          size_t _rtt_drop,
          size_t _rtt_peer_info,
          detail::fptr_creator _fcreator_join,
          detail::fptr_creator _fcreator_drop,
          detail::fptr_creator _fcreator_peer_info,
          detail::fptr_saver _fsaver_join,
          detail::fptr_saver _fsaver_drop,
          detail::fptr_saver _fsaver_peer_info,
          detail::fptr_message_loader _fmessage_loader>
class p2psocket_family_t
{
public:
    static constexpr size_t rtt_join = _rtt_join;
    static constexpr size_t rtt_drop = _rtt_drop;
    static constexpr size_t rtt_peer_info = _rtt_peer_info;
    static constexpr detail::fptr_creator fcreator_join = _fcreator_join;
    static constexpr detail::fptr_creator fcreator_drop = _fcreator_drop;
    static constexpr detail::fptr_creator fcreator_peer_info = _fcreator_peer_info;
    static constexpr detail::fptr_saver fsaver_join = _fsaver_join;
    static constexpr detail::fptr_saver fsaver_drop = _fsaver_drop;
    static constexpr detail::fptr_saver fsaver_peer_info = _fsaver_peer_info;
    static constexpr detail::fptr_message_loader fmessage_loader = _fmessage_loader;
};

class P2PSOCKETSHARED_EXPORT p2psocket : public beltpp::isocket
{
public:
    using peer_id = isocket::peer_id;
    using peer_ids = socket::peer_ids;
    using messages = isocket::messages;

    p2psocket(size_t _rtt_join,
              size_t _rtt_drop,
              size_t _rtt_peer_info,
              detail::fptr_creator _fcreator_join,
              detail::fptr_creator _fcreator_drop,
              detail::fptr_creator _fcreator_peer_info,
              detail::fptr_saver _fsaver_join,
              detail::fptr_saver _fsaver_drop,
              detail::fptr_saver _fsaver_peer_info,
              detail::fptr_message_loader _fmessage_loader);
              //std::vector<beltpp::address> const& addresses);
    p2psocket(p2psocket&& other);
    virtual ~p2psocket();

    messages read(peer_id& peer) override;

    void write(peer_id const& peer,
               message const& msg) override;

private:
    std::unique_ptr<detail::p2psocket_internals> m_pimpl;
};

template <typename T_p2psocket_family>
p2psocket P2PSOCKETSHARED_EXPORT getp2psocket()
{
    return
    p2psocket(T_p2psocket_family::rtt_join,
              T_p2psocket_family::rtt_drop,
              T_p2psocket_family::rtt_peer_info,
              T_p2psocket_family::fcreator_join,
              T_p2psocket_family::fcreator_drop,
              T_p2psocket_family::fcreator_peer_info,
              T_p2psocket_family::fsaver_join,
              T_p2psocket_family::fsaver_drop,
              T_p2psocket_family::fsaver_peer_info,
              T_p2psocket_family::fmessage_loader);
}

}

#endif // BELT_P2PSOCKET_H
