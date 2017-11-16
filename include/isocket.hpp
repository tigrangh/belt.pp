#ifndef BELT_ISOCKET_H
#define BELT_ISOCKET_H

#include <string>

namespace beltpp
{

class imessage;

class isocket
{
public:
    enum class socketv {any, ipv4, ipv6};
    using peer_id = std::string;

    isocket() {};
    virtual ~isocket() {};

    virtual bool read(peer_id& peer,
                      imessage& msg) = 0;

    virtual void write(peer_id const& peer,
                       imessage const& msg) = 0;

    virtual std::string dump() const = 0;
};

}

#endif // BELT_ISOCKET_H
