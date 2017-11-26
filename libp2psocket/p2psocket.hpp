#ifndef BELT_P2PSOCKET_H
#define BELT_P2PSOCKET_H

#include "global.hpp"
#include <socket.hpp>

#include <memory>
#include <string>
#include <list>

namespace beltpp
{

namespace detail
{
    class p2psocket_internals;
}

class P2PSOCKETSHARED_EXPORT p2psocket : public beltpp::socket
{
public:
    using peer_id = socket::peer_id;
    using peer_ids = std::list<peer_id>;
    using messages = socket::messages;

    p2psocket();
    virtual ~p2psocket();

    peer_ids open(address const& introducer_address);

    messages read(peer_id& peer) override;

    void write(peer_id const& peer,
               message const& msg) override;

    std::string dump() const override;

private:
    std::unique_ptr<detail::p2psocket_internals> m_pimpl;
};

}

#endif // BELT_P2PSOCKET_H
