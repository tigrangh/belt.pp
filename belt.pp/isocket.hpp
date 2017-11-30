#ifndef BELT_ISOCKET_H
#define BELT_ISOCKET_H

#include <string>
#include <list>

namespace beltpp
{

class message;

class ip_destination
{
public:
    std::string address;
    unsigned short port;

    bool empty() const {return port == 0 || address.empty();}
};

class ip_address
{
public:
    enum class e_type {any, ipv4, ipv6};

    ip_address(std::string const& str_local_address = std::string(),
               unsigned short local_port = 0,
               std::string const& str_remote_address = std::string(),
               unsigned short remote_port = 0,
               e_type en_type = e_type::any)
        : type(en_type)
        , local{str_local_address, local_port}
        , remote{str_remote_address, remote_port}
    {}

    ip_address(std::string const& str_local_address,
               unsigned short local_port,
               e_type en_type)
        : type(en_type)
        , local{str_local_address, local_port}
        , remote()
    {}

    ip_address(ip_destination const& dest,
               e_type en_type)
        : type(en_type)
        , local{dest.address, dest.port}
        , remote()
    {}

    std::string to_string(ip_destination const* dest = nullptr) const
    {
        if (nullptr == dest)
        {
            std::string result = to_string(&local);
            if (false == remote.empty())
                result += "<=>" + to_string(&remote);
            return result;
        }
        else
        {
            if (dest->empty())
                return std::string();
            else if (type == e_type::ipv4)
                return dest->address + ":" + std::to_string(dest->port);
            else
                return "[" + dest->address + "]:" + std::to_string(dest->port);
        }
    }

    e_type type;
    ip_destination local;
    ip_destination remote;
};

class isocket
{
public:
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
