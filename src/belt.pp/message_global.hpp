#pragma once

#include "global.hpp"
#include "iterator_wrapper.hpp"
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

class session_special_data
{
public:
    session_special_data()
        : ptr_data(nullptr, [](void*){})
        , session_specal_handler(nullptr) {}
    beltpp::void_unique_ptr ptr_data;
    std::string(*session_specal_handler)(session_special_data&, beltpp::packet const&);
};

using fptr_message_loader = detail::pmsg_all (*)(
        beltpp::iterator_wrapper<char const>&,
        beltpp::iterator_wrapper<char const> const&,
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

