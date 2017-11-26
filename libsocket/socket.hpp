#ifndef BELT_SOCKET_H
#define BELT_SOCKET_H

#include "global.hpp"
#include <isocket.hpp>

#include <memory>
#include <string>
#include <list>

namespace beltpp
{

namespace detail
{
    class socket_internals;
}

class SOCKETSHARED_EXPORT address
{
public:
    std::string m_address;
    unsigned short m_port;
};

class SOCKETSHARED_EXPORT socket : public beltpp::isocket
{
public:
    using socketv = isocket::socketv;
    using peer_id = isocket::peer_id;
    using peer_ids = std::list<peer_id>;
    using messages = isocket::messages;

    socket();
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

    std::string dump() const override;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

}

#endif // BELT_SOCKET_H
