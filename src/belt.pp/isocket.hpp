#pragma once

#include "global.hpp"
#include "ievent.hpp"
#include "stream.hpp"
#include "message_global.hpp"

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
    bool operator != (ip_destination const& other) const noexcept
    {
        return false == (operator == (other));
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

    bool operator == (ip_address const& other) const noexcept
    {
        return (local == other.local &&
                remote == other.remote &&
                ip_type == other.ip_type);
    }
    bool operator != (ip_address const& other) const noexcept
    {
        return false == (operator == (other));
    }

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

            ip_type = e_type::any;

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

        ip_type = e_type::ipv4;

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
            ip_type = e_type::any;
        }

        std::string error_help = "cannot parse \"" + value +
                "\". use host_name::port [or"
                " [host_name]::port for ipv6 [or {host_name}::port for any]] to specify ip address";

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

    e_type ip_type;
    ip_destination local;
    ip_destination remote;
};

class socket : public stream
{
public:
    enum class peer_type
    {
        listening,
        streaming_opened,
        streaming_accepted
    };

    using peer_id = stream::peer_id;
    using packets = stream::packets;
    using peer_ids = std::list<peer_id>;

    socket(event_handler& eh) : stream(eh) {}
    virtual ~socket() {}

    virtual peer_ids listen(ip_address const& address,
                            int backlog = 100) = 0;

    virtual peer_ids open(ip_address address,
                          size_t attempts = 0) = 0;

    packets receive(peer_id& peer) override = 0;

    void send(peer_id const& peer,
              packet&& pack) override = 0;

    virtual peer_type get_peer_type(peer_id const& peer) = 0;

    virtual ip_address info(peer_id const& peer) = 0;
    virtual ip_address info_connection(peer_id const& peer) = 0;
    virtual detail::session_special_data& session_data(peer_id const& peer) = 0;

    virtual std::string dump() const = 0;
};

using socket_ptr = std::unique_ptr<socket>;

class socket_open_refused
{
public:
    static const size_t rtt = size_t(-13);
    static std::string pvoid_saver(void*) { return std::string(); }

    socket_open_refused(std::string const& reason_ = std::string(),
                        ip_address const& addr = ip_address())
        : reason(reason_)
        , address(addr)
    {}

    std::string reason;
    ip_address address;
};

class socket_open_error
{
public:
    static const size_t rtt = size_t(-14);
    static std::string pvoid_saver(void*) { return std::string(); }

    socket_open_error(std::string const& reason_ = std::string(),
                      ip_address const& addr = ip_address())
        : reason(reason_)
        , address(addr)
    {}

    std::string reason;
    ip_address address;
};

}

