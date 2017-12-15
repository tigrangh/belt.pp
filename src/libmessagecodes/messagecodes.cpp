#include "messagecodes.hpp"

#include <cassert>

using message_code_store = beltpp::detail::message_code_store;
using message_list = beltpp::message_list;

beltpp::detail::fptr_creator
    message_code_store::s_creators[message_list::count];
beltpp::detail::fptr_scanner
    message_code_store::s_scanners[message_list::count];
beltpp::detail::fptr_saver
    message_code_store::s_savers[message_list::count];

namespace beltpp
{
namespace detail
{
void find(std::string const& value,
          beltpp::iterator_wrapper<char const>& iter_start,
          beltpp::iterator_wrapper<char const>& iter_end,
          beltpp::iterator_wrapper<char const> const iter_scan_begin,
          beltpp::iterator_wrapper<char const> const iter_scan_end,
          bool starts_with);

scan_result read(uint64_t& value,
                 beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                 beltpp::iterator_wrapper<char const> const& iter_scan_end) noexcept;
}

beltpp::detail::pmsg_all message_list_load(
        beltpp::iterator_wrapper<char const> const& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end,
        size_t& clean_count)
{
    uint64_t message_code_rtt(-1);
    detail::scan_result result = detail::read(message_code_rtt,
                                              iter_scan_begin,
                                              iter_scan_end);

    beltpp::detail::pmsg_all return_value (size_t(-1),
                                           detail::ptr_msg(nullptr, [](void*&){}),
                                           nullptr);

    if (message_code_rtt >= message_list::count)
        result.first = detail::e_scan_result::error;

    if (result.first == detail::e_scan_result::success)
    {
        beltpp::detail::ptr_msg pmsg =
                message_code_store::s_creators[message_code_rtt]();

        beltpp::iterator_wrapper<char const> iter_scan = iter_scan_begin;
        for (size_t index = 0; index != result.second; ++index)
            ++iter_scan;

        auto& fscanner = message_code_store::s_scanners[message_code_rtt];
        auto& fsaver = message_code_store::s_savers[message_code_rtt];
        detail::scan_result sr = fscanner(pmsg.get(),
                                          iter_scan,
                                          iter_scan_end);

        result.first = sr.first;

        if (sr.first == detail::e_scan_result::success)
        {
            result.second += sr.second;
            return_value =
                beltpp::detail::pmsg_all(   message_code_rtt,
                                            std::move(pmsg),
                                            fsaver);
        }
    }

    if (result.first == detail::e_scan_result::error)
    {
        clean_count = -1;
        return_value =
            beltpp::detail::pmsg_all(   message_code_error::rtt,
                                        message_code_error::creator(),
                                        &message_code_error::saver);
    }
    else if (result.first == detail::e_scan_result::success)
        clean_count = result.second;
    else
        clean_count = 0;

    return return_value;
}

namespace detail
{
void find(std::string const& value,
          beltpp::iterator_wrapper<char const>& iter_start,
          beltpp::iterator_wrapper<char const>& iter_end,
          beltpp::iterator_wrapper<char const> const iter_scan_begin,
          beltpp::iterator_wrapper<char const> const iter_scan_end,
          bool starts_with)
{
    auto const iter_value_begin = value.begin();
    auto const iter_value_end = value.end();
    auto iter_value = iter_value_begin;

    auto iter_scan = iter_scan_begin;

    iter_start = iter_scan_end;
    iter_end = iter_scan_end;

    bool starts_with_failed = false;

    for(;   iter_scan != iter_scan_end &&
            iter_value != iter_value_end; ++iter_scan)
    {
        if (*iter_scan != *iter_value)
        {
            iter_value = iter_value_begin;

            if (starts_with)
            {
                starts_with_failed = true;
                break;
            }
        }
        else
        {
            if (iter_value == iter_value_begin)
                iter_start = iter_scan;
            ++iter_value;
        }
    }

    if (starts_with &&
        starts_with_failed)
    {
        iter_start = iter_scan_end;
        iter_end = iter_scan_end;
    }
    else if (iter_value == iter_value_end)
    {   //  either value was originally empty
        //  or it was found and iter_start is set already
        if (iter_value_begin != iter_value_end)
            iter_end = iter_scan;
    }
    else if (iter_scan == iter_scan_end &&
             iter_value != iter_value_begin)
    {   //  the string to scan was not empty
        //  a substring of value is found in the end
        //  so iter_start is set accordingly
        iter_end = iter_start;
    }
    else// if (iter_scan == iter_scan_end)
    {   //  either the string to scan was empty
        //  or it wasn't, but the string is not found
        assert(iter_scan == iter_scan_end);

        iter_start = iter_scan_end;
        iter_end = iter_scan_end;
    }
}

std::string read(std::string const& before,
                 beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                 beltpp::iterator_wrapper<char const> const& iter_scan_end,
                 bool& whole) noexcept
{
    auto iter_start = iter_scan_begin;
    auto iter_end = iter_scan_begin;

    find(before,
         iter_start,
         iter_end,
         iter_scan_begin,
         iter_scan_end,
         false);

    std::string result;

    if (iter_start == iter_scan_end)
    {
        assert(iter_end == iter_scan_end);
        whole = false;

        result = std::string(iter_scan_begin, iter_scan_end);
    }
    else
    {
        whole = true;
        result = std::string(iter_scan_begin, iter_start);
    }

    return result;
}

scan_result read(uint64_t& value,
                 beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                 beltpp::iterator_wrapper<char const> const& iter_scan_end) noexcept
{
    scan_result result;
    result.first = e_scan_result::error;
    result.second = 0;

    auto iter = iter_scan_begin;

    value = 0;

    while (iter != iter_scan_end &&
           *iter >= '0' &&
           *iter <= '9')
    {
        result.first = e_scan_result::success;
        ++result.second;

        value *= 10;
        value += (*iter - '0');

        ++iter;
        if (iter == iter_scan_end)
        {
            result.first = e_scan_result::attempt;
            break;
        }
    }

    if (iter != iter_scan_end &&
        *iter == ':')
        ++result.second;
    else if (iter != iter_scan_end)
        result.first = e_scan_result::error;

    return result;
}

static_assert(0 == beltpp::message_code_error::rtt, "rtt check");
static_assert(1 == beltpp::message_code_join::rtt, "rtt check");
static_assert(2 == beltpp::message_code_drop::rtt, "rtt check");
}

}
