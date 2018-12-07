#pragma once

#include "global.hpp"
#include "meta.hpp"

#include <string>
#include <list>

namespace beltpp
{
class packet;
namespace detail
{
using fptr_creator = beltpp::void_unique_ptr(*)();
using fptr_saver = std::string(*)(void*);

class pmsg_all
{
public:
    pmsg_all(size_t a_rtt,
             beltpp::void_unique_ptr&& a_pmsg,
             fptr_saver a_fsaver)
        : rtt(a_rtt)
        , pmsg(std::move(a_pmsg))
        , fsaver(a_fsaver)
    {}

    size_t rtt;
    beltpp::void_unique_ptr pmsg;
    fptr_saver fsaver;
};

class iscan_status
{
public:
    virtual ~iscan_status() {}
};

class session_special_data
{
public:
    session_special_data()
        : parser_unrecognized_limit(1024 * 1024)
        , ptr_data(beltpp::t_unique_nullptr<iscan_status>())
        , session_specal_handler(nullptr)
        , autoreply() {}
    size_t parser_unrecognized_limit;
    beltpp::t_unique_ptr<iscan_status> ptr_data;
    std::string(*session_specal_handler)(session_special_data&, beltpp::packet const&);
    std::string autoreply;
};

using fptr_message_loader = detail::pmsg_all (*)(
        std::string::const_iterator&,
        std::string::const_iterator const&,
        session_special_data&,
        void*);

}

class message_loader_utility
{
public:
    message_loader_utility()
        : m_fp_message_list_load_helper()
    {}

    using fp_message_list_load_helper = bool (*)
            (void* pexp,
             detail::pmsg_all& return_value,
             message_loader_utility const& utl);

    fp_message_list_load_helper m_fp_message_list_load_helper;
    std::list<fp_message_list_load_helper> m_arr_fp_message_list_load_helper;
};

}

