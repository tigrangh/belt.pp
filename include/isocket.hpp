#ifndef BELT_ISOCKET_H
#define BELT_ISOCKET_H

#include <string>
#include <list>

namespace beltpp
{

class message;

class isocket
{
public:
    enum class socketv {any, ipv4, ipv6};
    using peer_id = std::string;
    using messages = std::list<message>;

    isocket() {};
    virtual ~isocket() {};

    virtual messages read(peer_id& peer) = 0;

    virtual void write(peer_id const& peer,
                       message const& msg) = 0;
};

}

#endif // BELT_ISOCKET_H
