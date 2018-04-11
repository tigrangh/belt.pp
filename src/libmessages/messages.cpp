#include "messages.hpp"

#include <belt.pp/message_global.hpp>

#include <cassert>

using message_list = beltpp::message_list;
using message_code_store =
beltpp::detail::message_store<message_list>;

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
        beltpp::iterator_wrapper<char const>& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end)
{
    size_t clean_count = 0;
    uint64_t message_code_rtt(-1);
    detail::scan_result result = detail::read(message_code_rtt,
                                              iter_scan_begin,
                                              iter_scan_end);

    beltpp::detail::pmsg_all return_value (size_t(-1),
                                           void_unique_ptr(nullptr, [](void*&){}),
                                           nullptr);

    if (message_code_rtt >= message_list::count)
        result.first = detail::e_scan_result::error;

    if (result.first == detail::e_scan_result::success)
    {
        void_unique_ptr pmsg =
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
            beltpp::detail::pmsg_all(   message_error::rtt,
                                        new_void_unique_ptr<message_error>(),
                                        &message_error::saver);
    }
    else if (result.first == detail::e_scan_result::success)
        clean_count = result.second;
    else
        clean_count = 0;

    while (iter_scan_begin != iter_scan_end &&
           clean_count > 0)
    {
        --clean_count;
        ++iter_scan_begin;
    }

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

static_assert(0 == beltpp::message_error::rtt, "rtt check");
static_assert(1 == beltpp::message_join::rtt, "rtt check");
static_assert(2 == beltpp::message_drop::rtt, "rtt check");
}

}
