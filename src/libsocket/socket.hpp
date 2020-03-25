#pragma once

#include "global.hpp"

#include <belt.pp/isocket.hpp>
#include <belt.pp/message_global.hpp>
#include <belt.pp/ievent.hpp>

#include <memory>
#include <string>
#include <list>
#include <vector>

namespace beltpp
{

namespace detail
{
    class socket_internals;
}

template <detail::fptr_message_loader _fmessage_loader>
class socket_family_t
{
public:
    static constexpr detail::fptr_message_loader fmessage_loader = _fmessage_loader;
};

class SOCKETSHARED_EXPORT socket : public beltpp::isocket
{
public:
    using peer_id = isocket::peer_id;
    using peer_ids = std::list<peer_id>;
    using packets = isocket::packets;
    enum option
    {
        option_none = 0x0,
        option_reuse_port = 0x1,
        option_keep_alive = 0x2
    };
    enum class peer_type
    {
        listening,
        streaming_opened,
        streaming_accepted
    };

    socket(ievent_handler& eh,
           detail::fptr_message_loader _fmessage_loader,
           option e_option,
           beltpp::void_unique_ptr&& putl);
    socket(socket&& other);
    ~socket() override;

    peer_ids listen(ip_address const& address,
                    int backlog = 100);

    peer_ids open(ip_address address,
                  size_t attempts = 0);

    void prepare_wait() override;

    packets receive(peer_id& peer) override;

    void send(peer_id const& peer,
              packet&& pack) override;

    void timer_action() override;

    peer_type get_peer_type(peer_id const& peer) const;
    ip_address info(peer_id const& peer) const;
    ip_address info_connection(peer_id const& peer) const;
    detail::session_special_data& session_data(peer_id const& peer);

    std::string dump() const;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

template <typename T_socket_family>
socket getsocket(ievent_handler& eh,
                 socket::option e_option,
                 beltpp::void_unique_ptr&& putl)
{
    return
    socket(eh,
           T_socket_family::fmessage_loader,
           e_option,
           std::move(putl));
}

template <typename T_socket_family>
socket getsocket(ievent_handler& eh,
                 socket::option e_option = socket::option_none)
{
    beltpp::void_unique_ptr putl = beltpp::new_void_unique_ptr<beltpp::message_loader_utility>();
    return getsocket<T_socket_family>(eh, e_option, std::move(putl));
}

}

