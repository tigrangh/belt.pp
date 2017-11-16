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

class SOCKETSHARED_EXPORT socket : public beltpp::isocket
{
public:
    using socketv = isocket::socketv;
    using peer_id = isocket::peer_id;
    using peer_ids = std::list<peer_id>;

    socket();
    virtual ~socket();

    peer_ids listen(std::string const& str_local_address,
                    unsigned short local_port,
                    socketv version,
                    int backlog = 100);

    peer_ids open(std::string const& str_local_address,
                  unsigned short local_port,
                  std::string const& str_remote_address,
                  unsigned short remote_port,
                  socketv version);

    bool read(peer_id& peer,
              imessage& msg) override;

    void write(peer_id const& peer,
               imessage const& msg) override;

    std::string dump() const override;

private:
    std::unique_ptr<detail::socket_internals> m_pimpl;
};

}

#endif // BELT_SOCKET_H
