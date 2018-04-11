#pragma once

#include "global.hpp"
#include <belt.pp/isocket.hpp>
#include <belt.pp/message_global.hpp>

#include <memory>
#include <string>
#include <list>
#include <vector>
#include <chrono>

namespace beltpp
{

namespace detail
{
    class socket_internals;
}

template <size_t _rtt_error,
          size_t _rtt_join,
          size_t _rtt_drop,
          size_t _rtt_timer_out,
          detail::fptr_creator _fcreator_error,
          detail::fptr_creator _fcreator_join,
          detail::fptr_creator _fcreator_drop,
          detail::fptr_creator _fcreator_timer_out,
          detail::fptr_saver _fsaver_error,
          detail::fptr_saver _fsaver_join,
          detail::fptr_saver _fsaver_drop,
          detail::fptr_saver _fsaver_timer_out,
          detail::fptr_message_loader _fmessage_loader>
class socket_family_t
{
public:
    static constexpr size_t rtt_error = _rtt_error;
    static constexpr size_t rtt_join = _rtt_join;
    static constexpr size_t rtt_drop = _rtt_drop;
    static constexpr size_t rtt_timer_out = _rtt_timer_out;
    static constexpr detail::fptr_creator fcreator_error = _fcreator_error;
    static constexpr detail::fptr_creator fcreator_join = _fcreator_join;
    static constexpr detail::fptr_creator fcreator_drop = _fcreator_drop;
    static constexpr detail::fptr_creator fcreator_timer_out = _fcreator_timer_out;
    static constexpr detail::fptr_saver fsaver_error = _fsaver_error;
    static constexpr detail::fptr_saver fsaver_join = _fsaver_join;
    static constexpr detail::fptr_saver fsaver_drop = _fsaver_drop;
    static constexpr detail::fptr_saver fsaver_timer_out = _fsaver_timer_out;
    static constexpr detail::fptr_message_loader fmessage_loader = _fmessage_loader;
};

class SOCKETSHARED_EXPORT socket : public beltpp::isocket
{
public:
    using peer_id = isocket::peer_id;
    using peer_ids = std::list<peer_id>;
    using packets = isocket::packets;

    socket(size_t _rtt_error,
           size_t _rtt_join,
           size_t _rtt_drop,
           size_t _rtt_timer_out,
           detail::fptr_creator _fcreator_error,
           detail::fptr_creator _fcreator_join,
           detail::fptr_creator _fcreator_drop,
           detail::fptr_creator _fcreator_timer_out,
           detail::fptr_saver _fsaver_error,
           detail::fptr_saver _fsaver_join,
           detail::fptr_saver _fsaver_drop,
           detail::fptr_saver _fsaver_timer_out,
           detail::fptr_message_loader _fmessage_loader,
           beltpp::void_unique_ptr&& putl);
    socket(socket&& other);
    virtual ~socket();

    peer_ids listen(ip_address const& address,
                    int backlog = 100);

    void open(ip_address address,
              size_t attempts = 0);

    packets receive(peer_id& peer) override;

    void send(peer_id const& peer,
              packet const& msg) override;

    void set_timer(std::chrono::steady_clock::duration const& period);

    ip_address info(peer_id const& peer) const;

    std::string dump() const;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

template <typename T_socket_family>
socket SOCKETSHARED_EXPORT getsocket(beltpp::void_unique_ptr&& putl)
{
    return
    socket(T_socket_family::rtt_error,
           T_socket_family::rtt_join,
           T_socket_family::rtt_drop,
           T_socket_family::rtt_timer_out,
           T_socket_family::fcreator_error,
           T_socket_family::fcreator_join,
           T_socket_family::fcreator_drop,
           T_socket_family::fcreator_timer_out,
           T_socket_family::fsaver_error,
           T_socket_family::fsaver_join,
           T_socket_family::fsaver_drop,
           T_socket_family::fsaver_timer_out,
           T_socket_family::fmessage_loader,
           std::move(putl));
}

template <typename T_socket_family>
socket SOCKETSHARED_EXPORT getsocket()
{
    beltpp::void_unique_ptr putl = beltpp::new_void_unique_ptr<beltpp::message_loader_utility>();
    return getsocket<T_socket_family>(std::move(putl));
}

}

