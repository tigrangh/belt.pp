#pragma once

#include "global.hpp"
#include "ievent.hpp"

#include <string>
#include <list>
#include <exception>

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
        return (address == other.address && port == other.port);
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
        , local(str_local_address, local_port)
        , remote(str_remote_address, remote_port)
    {}

    ip_address(std::string const& str_local_address,
               unsigned short local_port,
               e_type en_type)
        : ip_type(en_type)
        , local(str_local_address, local_port)
        , remote()
    {}

    ip_address(ip_destination const& dest,
               e_type en_type)
        : ip_type(en_type)
        , local(dest.address, dest.port)
        , remote()
    {}

    std::string to_string() const
    {
        ip_address local_holder;
        ip_address remote_holder;

        local_holder.local = local;
        local_holder.ip_type = ip_type;

        remote_holder.local = remote;
        remote_holder.ip_type = ip_type;

        std::string result = local_holder.to_string_dest();
        if (false == remote.empty())
            result += "<=>" + remote_holder.to_string();
        return result;
    }

private:
    std::string to_string_dest() const
    {
        if (local.empty())
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

            return op + local.address + cl + ":" +
                    std::to_string(local.port);
        }
    }

public:
    void from_string(std::string const& value)
    {
        ip_address local_holder;
        ip_address remote_holder;

        size_t delimiter_index = value.find("<=>");
        if (std::string::npos != delimiter_index)
        {
            std::string str_local = value.substr(0, delimiter_index);
            std::string str_remote = value.substr(delimiter_index + 3);

            local_holder.from_string_dest(str_local);
            remote_holder.from_string_dest(str_remote);

            if (local_holder.ip_type == e_type::ipv6 ||
                remote_holder.ip_type == e_type::ipv6)
            {
                if (local_holder.ip_type == e_type::ipv4 ||
                    remote_holder.ip_type == e_type::ipv4)
                    throw std::runtime_error("cannot mix ip4 and ip6 addresses"
                                             " in a bundle");

                ip_type = e_type::ipv6;
            }
            if (local_holder.ip_type == e_type::ipv4 ||
                remote_holder.ip_type == e_type::ipv4)
                ip_type = e_type::ipv4;

            local = local_holder.local;
            remote = remote_holder.local;
        }
        else
        {
            local_holder.from_string_dest(value);
            *this = local_holder;
        }
    }

private:
    void from_string_dest(std::string const& value)
    {
        size_t op_bracket_index = value.find("[");
        size_t cl_bracket_index = value.find("]");
        size_t op_brace_index = value.find("{");
        size_t cl_brace_index = value.find("}");

        size_t scope_index = std::string::npos;

        size_t bracket_index = std::string::npos;
        if (0 == op_bracket_index &&
            std::string::npos != cl_bracket_index)
        {
            bracket_index = cl_bracket_index;
            scope_index = bracket_index;
            ip_type = e_type::ipv6;
        }

        size_t brace_index = std::string::npos;
        if (0 == op_brace_index &&
            std::string::npos != cl_brace_index)
        {
            brace_index = cl_brace_index;
            scope_index = brace_index;
        }

        std::string error_help = "cannot parse \"" + value +
                "\". use host_name::port [or"
                " [host_name]::port for ipv6] to specify ip address";

        if (std::string::npos == brace_index &&
            (std::string::npos != op_brace_index ||
             std::string::npos != cl_brace_index))
            throw std::runtime_error(error_help);
        if (std::string::npos == bracket_index &&
            (std::string::npos != op_bracket_index ||
             std::string::npos != cl_bracket_index))
            throw std::runtime_error(error_help);

        size_t colon_index = std::string::npos;
        if (std::string::npos == scope_index)
            colon_index = value.find(":");
        else
            colon_index = value.find(":", scope_index);

        if (std::string::npos == colon_index)
            throw std::runtime_error(error_help);

        std::string str_host;
        if (std::string::npos == scope_index)
            str_host = value.substr(0, colon_index);
        else
            str_host = value.substr(1, colon_index - 2);
        std::string str_port = value.substr(colon_index + 1);

        local.address = str_host;

        size_t end = 0;
        local.port = beltpp::stoui16(str_port, end);
        if (end != str_port.length())
            throw std::runtime_error("cannot parse \"" + str_port +
                                     "\". ip port must be uint16");
    }

public:

    bool operator == (ip_address const& other) const noexcept
    {
        return (ip_type == other.ip_type && local == other.local && remote == other.remote);
    }

    e_type ip_type;
    ip_destination local;
    ip_destination remote;
};

class isocket : public ievent_item
{
public:
    using peer_id = std::string;
    using packets = std::list<packet>;

    isocket(ievent_handler& eh) : ievent_item(eh) {}
    virtual ~isocket() {}

    virtual packets receive(peer_id& peer) = 0;

    virtual void send(peer_id const& peer,
                      packet&& pack) = 0;
};

class isocket_join
{
public:
    static const size_t rtt = size_t(-10);
    static std::string pvoid_saver(void*) { return std::string(); }
};
class isocket_drop
{
public:
    static const size_t rtt = size_t(-11);
    static std::string pvoid_saver(void*) { return std::string(); }
};

class isocket_protocol_error
{
public:
    static const size_t rtt = size_t(-12);
    static std::string pvoid_saver(void*) { return std::string(); }

    isocket_protocol_error(std::string const& buf = std::string())
        : buffer(buf) {}

    std::string buffer;
};

class isocket_open_refused
{
public:
    static const size_t rtt = size_t(-13);
    static std::string pvoid_saver(void*) { return std::string(); }
};

class isocket_open_error
{
public:
    static const size_t rtt = size_t(-14);
    static std::string pvoid_saver(void*) { return std::string(); }

    isocket_open_error(std::string const& reason_ = std::string())
        : reason(reason_) {}

    std::string reason;
};

}

