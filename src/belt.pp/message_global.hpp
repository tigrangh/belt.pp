#pragma once

#include "global.hpp"
#include "iterator_wrapper.hpp"
#include "meta.hpp"

#include <vector>

namespace beltpp
{
namespace detail
{
using fptr_deleter = void(*)(void*&);
using ptr_msg = std::unique_ptr<void, fptr_deleter>;
using fptr_creator = ptr_msg(*)();
//enum class e_scan_result {success, attempt, error};
using e_scan_result = e_three_state_result;
using scan_result = std::pair<e_scan_result, size_t>;
using fptr_scanner = scan_result(*)(void*,
                                    beltpp::iterator_wrapper<char const> const&,
                                    beltpp::iterator_wrapper<char const> const&);
using fptr_saver = std::vector<char>(*)(void*);

class pmsg_all
{
public:
    pmsg_all(size_t a_rtt,
             ptr_msg a_pmsg,
             fptr_saver a_fsaver)
        : rtt(a_rtt)
        , pmsg(std::move(a_pmsg))
        , fsaver(a_fsaver)
    {}

    size_t rtt;
    ptr_msg pmsg;
    fptr_saver fsaver;
};

using fptr_message_loader = detail::pmsg_all (*)(
        beltpp::iterator_wrapper<char const> const&,
        beltpp::iterator_wrapper<char const> const&,
        size_t&);

template <size_t rtt,
          detail::fptr_creator fcreator,
          detail::fptr_scanner fscanner,
          detail::fptr_saver fsaver,
          typename MessageCodeStore>
int store_fptr();

template <typename MessageList>
class message_code_store;
}

namespace inspection
{
DECLARE_MF_INSPECTION(message_saver,
                      std::string(TT::*)() const)
DECLARE_MF_INSPECTION(message_scanner,
                      beltpp::detail::scan_result(TT::*)
                      (beltpp::iterator_wrapper<char const> const&,
                       beltpp::iterator_wrapper<char const> const&))
}

template <typename MessageCode, typename MessageList>
class message_code
{
public:
    using parent_list = MessageList;
    enum {rtt = typelist::type_list_index
                <MessageCode, MessageList>::value};
    message_code() {++s_dummy;} //  members in a template class need actually
    virtual ~message_code() {--s_dummy;}    //  to be used, to get right by
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
    template <typename T = MessageCode,
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
    template <typename T = MessageCode,
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
    static detail::ptr_msg creator()
    {
        detail::ptr_msg result(nullptr,
                               [](void* &p)
                                {
                                    MessageCode* pmc =
                                            static_cast<MessageCode*>(p);
                                    delete pmc;
                                    p = nullptr;
                                });

        result.reset(new MessageCode());
        return result;
    }

    static detail::scan_result scanner(void* p,
                                       beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                                       beltpp::iterator_wrapper<char const> const& iter_scan_end)
    {
        MessageCode* pmc = static_cast<MessageCode*>(p);
        return message_scanner(iter_scan_begin, iter_scan_end, pmc);
    }

    static std::vector<char> saver(void* p)
    {
        MessageCode* pmc = static_cast<MessageCode*>(p);
        std::vector<char> result;
        std::string str_rtt = std::to_string(message_code::rtt);
        str_rtt += ":" + message_saver(pmc);
        std::copy(str_rtt.begin(), str_rtt.end(), std::back_inserter(result));
        return result;
    }

protected:

    static int s_dummy;
};

template <typename MessageCode, typename MessageList>
int message_code<MessageCode, MessageList>::s_dummy =
        detail::store_fptr<message_code::rtt,
                            &message_code::creator,
                            &message_code::scanner,
                            &message_code::saver,
                            detail::message_code_store<MessageList>>();

namespace detail
{
template <typename MessageList>
class message_code_store
{
public:
    static fptr_creator s_creators[MessageList::count];
    static fptr_scanner s_scanners[MessageList::count];
    static fptr_saver s_savers[MessageList::count];
};

template <typename MessageList>
fptr_creator message_code_store<MessageList>::s_creators[MessageList::count];
template <typename MessageList>
fptr_scanner message_code_store<MessageList>::s_scanners[MessageList::count];
template <typename MessageList>
fptr_saver message_code_store<MessageList>::s_savers[MessageList::count];

template <size_t rtt,
          fptr_creator fcreator,
          fptr_scanner fscanner,
          fptr_saver fsaver,
          typename MessageCodeStore>
int store_fptr()
{
    MessageCodeStore::s_creators[rtt] = fcreator;
    MessageCodeStore::s_scanners[rtt] = fscanner;
    MessageCodeStore::s_savers[rtt] = fsaver;
    return 0;
}

}

}

