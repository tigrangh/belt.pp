#pragma once

#include <string>
#include <list>

namespace beltpp
{

class packet;

class ip_destination
{
public:
    ip_destination(std::string const& addr = std::string(),
                   unsigned short p = 0)
        : address(addr)
        , port(p)
    {}
    std::string address;
    unsigned short port;

    bool empty() const noexcept {return port == 0 || address.empty();}
    bool operator == (ip_destination const& other) const noexcept
    {
        return (address == other.address && port == port);
    }
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
        : ip_type(en_type)
        , local{str_local_address, local_port}
        , remote{str_remote_address, remote_port}
    {}

    ip_address(std::string const& str_local_address,
               unsigned short local_port,
               e_type en_type)
        : ip_type(en_type)
        , local{str_local_address, local_port}
        , remote()
    {}

    ip_address(ip_destination const& dest,
               e_type en_type)
        : ip_type(en_type)
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
            else
            {
                std::string op, cl;
                if (ip_type == e_type::ipv6)
                {
                    op = "[";
                    cl = "]";
                }
                else if (ip_type == e_type::any)
                {
                    op = "{";
                    cl = "}";
                }

                return op + dest->address + cl + ":" +
                        std::to_string(dest->port);
            }
        }
    }

    bool operator == (ip_address const& other) const noexcept
    {
        return (ip_type == other.ip_type && local == other.local && remote == other.remote);
    }

    e_type ip_type;
    ip_destination local;
    ip_destination remote;
};

class isocket
{
public:
    using peer_id = std::string;
    using packets = std::list<packet>;

    isocket() {};
    virtual ~isocket() {};

    virtual packets receive(peer_id& peer) = 0;

    virtual void send(peer_id const& peer,
                      packet const& msg) = 0;
};

}

