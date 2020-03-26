#pragma once

#include "global.hpp"

#include <belt.pp/isocket.hpp>
#include <belt.pp/message_global.hpp>
#include <belt.pp/ievent.hpp>

#include <memory>
#include <string>
#include <list>
#include <vector>

namespace beltpp
{

template <detail::fptr_message_loader _fmessage_loader>
class socket_family_t
{
public:
    static constexpr detail::fptr_message_loader fmessage_loader = _fmessage_loader;
};

namespace libsocket
{
enum option
{
    option_none = 0x0,
    option_reuse_port = 0x1,
    option_keep_alive = 0x2
};

SOCKETSHARED_EXPORT socket_ptr construct_socket(event_handler& eh,
                                                detail::fptr_message_loader _fmessage_loader,
                                                option e_option,
                                                beltpp::void_unique_ptr&& putl);
SOCKETSHARED_EXPORT event_handler_ptr construct_event_handler();

template <typename T_socket_family>
socket_ptr getsocket(event_handler& eh,
                     libsocket::option e_option,
                     beltpp::void_unique_ptr&& putl)
{
    return
    construct_socket(eh,
                     T_socket_family::fmessage_loader,
                     e_option,
                     std::move(putl));
}

template <typename T_socket_family>
socket_ptr getsocket(event_handler& eh,
                     libsocket::option e_option = libsocket::option_none)
{
    beltpp::void_unique_ptr putl = beltpp::new_void_unique_ptr<beltpp::message_loader_utility>();
    return getsocket<T_socket_family>(eh, e_option, std::move(putl));
}
}

}

