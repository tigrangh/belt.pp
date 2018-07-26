#pragma once

#include "global.hpp"
#include "iterator_wrapper.hpp"
#include "meta.hpp"

#include <string>
#include <list>

namespace beltpp
{
class message_loader_utility;
namespace detail
{
using fptr_creator = beltpp::void_unique_ptr(*)();
//enum class e_scan_result {success, attempt, error};
using e_scan_result = e_three_state_result;
using scan_result = std::pair<e_scan_result, size_t>;
using fptr_scanner = scan_result(*)(void*,
                                    beltpp::iterator_wrapper<char const> const&,
                                    beltpp::iterator_wrapper<char const> const&);
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
    std::string(*session_specal_handler)(session_special_data&, std::string const&);
};

using fptr_message_loader = detail::pmsg_all (*)(
        beltpp::iterator_wrapper<char const>&,
        beltpp::iterator_wrapper<char const> const&,
        session_special_data&,
        void*);

template <size_t rtt,
          detail::fptr_creator fcreator,
          detail::fptr_scanner fscanner,
          detail::fptr_saver fsaver,
          typename T_MessageStore>
int store_fptr();

template <typename MessageList>
class message_store;
}

namespace inspection
{
DECLARE_MF_INSPECTION(message_saver, TT,
                      std::string(TT::*)() const)
DECLARE_MF_INSPECTION(message_scanner, TT,
                      beltpp::detail::scan_result(TT::*)
                      (beltpp::iterator_wrapper<char const> const&,
                       beltpp::iterator_wrapper<char const> const&))
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

template <typename T_message, typename T_MessageList>
class message_base
{
public:
    using parent_list = T_MessageList;
    enum {rtt = typelist::type_list_index
                <T_message, T_MessageList>::value};
    message_base() {++s_dummy;} //  members in a template class need actually
    virtual ~message_base() {--s_dummy;}    //  to be used, to get right by
    //  other modules linking with this library

protected:
    template <typename T,
              typename TEST = typename std::enable_if<
                  inspection::has_message_saver<T>::value == 1, bool
                  >::type>
    static std::string message_saver(T* pmc)
    {
        return pmc->message_saver();
    }
    template <typename T = T_message,
              typename TEST = typename std::enable_if<
                  inspection::has_message_saver<T>::value == 0, char
                  >::type>
    static std::string message_saver(...)
    {
        return std::string();
    }
    template <typename T,
              typename TEST = typename std::enable_if<
                  inspection::has_message_scanner<T>::value == 1, bool
                  >::type>
    static detail::scan_result message_scanner(
                                       beltpp::iterator_wrapper<char const> const& b,
                                       beltpp::iterator_wrapper<char const> const& e,
                                       T* pmc)
    {
        return pmc->message_scanner(b, e);
    }
    template <typename T = T_message,
              typename TEST = typename std::enable_if<
                  inspection::has_message_scanner<T>::value == 0, char
                  >::type>
    static detail::scan_result message_scanner(
            beltpp::iterator_wrapper<char const> const&,
            beltpp::iterator_wrapper<char const> const&,
            ...)
    {
        detail::scan_result result;
        result.first = detail::e_scan_result::success;
        result.second = 0;
        return result;
    }
public:
    static detail::scan_result scanner(void* p,
                                       beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                                       beltpp::iterator_wrapper<char const> const& iter_scan_end)
    {
        T_message* pmc = static_cast<T_message*>(p);
        return message_scanner(iter_scan_begin, iter_scan_end, pmc);
    }

    static std::string pvoid_saver(void* p)
    {
        T_message* pmc = static_cast<T_message*>(p);
        std::string str_rtt = std::to_string(message_base::rtt);
        str_rtt += ":" + message_saver(pmc);
        return str_rtt;
    }

protected:
    static int s_dummy;
};

template <typename T_message, typename T_MessageList>
int message_base<T_message, T_MessageList>::s_dummy =
        detail::store_fptr<message_base::rtt,
                            &new_void_unique_ptr<T_message>,
                            &message_base::scanner,
                            &message_base::pvoid_saver,
                            detail::message_store<T_MessageList>>();

namespace detail
{
template <typename T_MessageList>
class message_store
{
public:
    static fptr_creator s_creators[T_MessageList::count];
    static fptr_scanner s_scanners[T_MessageList::count];
    static fptr_saver s_savers[T_MessageList::count];
};

template <typename T_MessageList>
fptr_creator message_store<T_MessageList>::s_creators[T_MessageList::count];
template <typename T_MessageList>
fptr_scanner message_store<T_MessageList>::s_scanners[T_MessageList::count];
template <typename T_MessageList>
fptr_saver message_store<T_MessageList>::s_savers[T_MessageList::count];

template <size_t rtt,
          fptr_creator fcreator,
          fptr_scanner fscanner,
          fptr_saver fsaver,
          typename T_MessageStore>
int store_fptr()
{
    T_MessageStore::s_creators[rtt] = fcreator;
    T_MessageStore::s_scanners[rtt] = fscanner;
    T_MessageStore::s_savers[rtt] = fsaver;
    return 0;
}

}
}

