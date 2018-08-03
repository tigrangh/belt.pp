#pragma once

#include "global.hpp"
#include "event.hpp"

#include <belt.pp/isocket.hpp>
#include <belt.pp/message_global.hpp>

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

    socket(event_handler& eh,
           detail::fptr_message_loader _fmessage_loader,
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

    ip_address info(peer_id const& peer) const;

    std::string dump() const;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

template <typename T_socket_family>
socket getsocket(event_handler& eh, beltpp::void_unique_ptr&& putl)
{
    return
    socket(eh,
           T_socket_family::fmessage_loader,
           std::move(putl));
}

template <typename T_socket_family>
socket getsocket(event_handler& eh)
{
    beltpp::void_unique_ptr putl = beltpp::new_void_unique_ptr<beltpp::message_loader_utility>();
    return getsocket<T_socket_family>(eh, std::move(putl));
}

}

