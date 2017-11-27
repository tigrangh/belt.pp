#ifndef BELT_SOCKET_H
#define BELT_SOCKET_H

#include "global.hpp"
#include <isocket.hpp>
#include <message_global.hpp>

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

class SOCKETSHARED_EXPORT address
{
public:
    address(std::string const& aaddress = std::string(),
            unsigned short port = 0);
    bool empty() const noexcept;

    std::string m_address;
    unsigned short m_port;
};

template <size_t _rtt_join,
          size_t _rtt_drop,
          detail::fptr_creator _fcreator_join,
          detail::fptr_creator _fcreator_drop,
          detail::fptr_saver _fsaver_join,
          detail::fptr_saver _fsaver_drop,
          detail::fptr_message_loader _fmessage_loader>
class socket_family_t
{
public:
    static constexpr size_t rtt_join = _rtt_join;
    static constexpr size_t rtt_drop = _rtt_drop;
    static constexpr detail::fptr_creator fcreator_join = _fcreator_join;
    static constexpr detail::fptr_creator fcreator_drop = _fcreator_drop;
    static constexpr detail::fptr_saver fsaver_join = _fsaver_join;
    static constexpr detail::fptr_saver fsaver_drop = _fsaver_drop;
    static constexpr detail::fptr_message_loader fmessage_loader = _fmessage_loader;
};

class SOCKETSHARED_EXPORT socket : public beltpp::isocket
{
public:
    using socketv = isocket::socketv;
    using peer_id = isocket::peer_id;
    using peer_ids = std::list<peer_id>;
    using messages = isocket::messages;

    socket(size_t _rtt_join,
           size_t _rtt_drop,
           detail::fptr_creator _fcreator_join,
           detail::fptr_creator _fcreator_drop,
           detail::fptr_saver _fsaver_join,
           detail::fptr_saver _fsaver_drop,
           detail::fptr_message_loader _fmessage_loader);
    socket(socket&& other);
    virtual ~socket();

    peer_ids listen(address const& local_address,
                    socketv version = socketv::any,
                    int backlog = 100);

    peer_ids open(address const& local_address,
                  address const& remote_address,
                  socketv version = socketv::any);

    messages read(peer_id& peer) override;

    void write(peer_id const& peer,
               message const& msg) override;

    class peer_info
    {
    public:
        address local;
        address remote;
    };
    peer_info info(peer_id const& peer) const;

    std::string dump() const;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

template <typename T_socket_family>
socket SOCKETSHARED_EXPORT getsocket()
{
    return
    socket(T_socket_family::rtt_join,
           T_socket_family::rtt_drop,
           T_socket_family::fcreator_join,
           T_socket_family::fcreator_drop,
           T_socket_family::fsaver_join,
           T_socket_family::fsaver_drop,
           T_socket_family::fmessage_loader);
}

}

#endif // BELT_SOCKET_H
