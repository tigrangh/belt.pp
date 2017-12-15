#pragma once

#include "global.hpp"

#include <belt.pp/iterator_wrapper.hpp>
#include <belt.pp/message_global.hpp>
#include <belt.pp/isocket.hpp>
#include <belt.pp/meta.hpp>

#include <memory>
#include <utility>
#include <vector>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <string>

namespace beltpp
{
namespace detail
{
std::string read(std::string const& before,
                 beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                 beltpp::iterator_wrapper<char const> const& iter_scan_end,
                 bool& whole) noexcept;

template <size_t rtt,
          detail::fptr_creator fcreator,
          detail::fptr_scanner fscanner,
          detail::fptr_saver fsaver>
int store_fptr();
}

using message_list =
typelist::type_list<
class message_code_error,
class message_code_join,
class message_code_drop,
class message_code_hello,
class message_code_get_peers,
class message_code_peer_info,
class message_code_timer_out
>;

namespace detail
{
class MESSAGECODESSHARED_EXPORT message_code_store
{
public:
    static detail::fptr_creator s_creators[message_list::count];
    static detail::fptr_scanner s_scanners[message_list::count];
    static detail::fptr_saver s_savers[message_list::count];
};
}

detail::pmsg_all MESSAGECODESSHARED_EXPORT message_list_load(
        beltpp::iterator_wrapper<char const> const& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end,
        size_t& clean_count);

namespace inspection
{
DECLARE_MF_INSPECTION(message_saver,
                      std::string(TT::*)() const)
DECLARE_MF_INSPECTION(message_scanner,
                      beltpp::detail::scan_result(TT::*)
                      (beltpp::iterator_wrapper<char const> const&,
                       beltpp::iterator_wrapper<char const> const&))
}

template <typename MessageCode>
class message_code
{
public:
    enum {rtt = typelist::type_list_index
                <MessageCode, message_list>::value};
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

template <typename MessageCode>
int message_code<MessageCode>::s_dummy =
        detail::store_fptr<message_code::rtt,
                            &message_code::creator,
                            &message_code::scanner,
                            &message_code::saver>();

class MESSAGECODESSHARED_EXPORT message_code_error : public message_code<message_code_error>
{
};

class MESSAGECODESSHARED_EXPORT message_code_join : public message_code<message_code_join>
{};

class MESSAGECODESSHARED_EXPORT message_code_drop : public message_code<message_code_drop>
{};

class MESSAGECODESSHARED_EXPORT message_code_hello : public message_code<message_code_hello>
{
public:
    message_code_hello()
        : m_message("") {}

    std::string message_saver() const
    {
        return m_message + "\n";
    }

    detail::scan_result message_scanner(beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                                        beltpp::iterator_wrapper<char const> const& iter_scan_end)
    {
        detail::scan_result result;
        result.first = detail::e_scan_result::success;
        result.second = 0;

        bool whole = false;
        std::string message =
                detail::read("\n", iter_scan_begin, iter_scan_end, whole);

        if (whole)
        {
            m_message = message;
            result.second = m_message.length() + 1;
        }
        else
        {
            result.first = detail::e_scan_result::attempt;
            result.second = -1;
        }


        return result;
    }
public:
    std::string m_message;
};

class MESSAGECODESSHARED_EXPORT message_code_get_peers : public message_code<message_code_get_peers>
{};

class MESSAGECODESSHARED_EXPORT message_code_peer_info : public message_code<message_code_peer_info>
{
public:
    std::string message_saver() const
    {
        std::string type;
        switch (address.type)
        {
        case ip_address::e_type::any:
            type = "any";
            break;
        case ip_address::e_type::ipv4:
            type = "ip4";
            break;
        case ip_address::e_type::ipv6:
            type = "ip6";
            break;
        default:
            break;
        }
        return address.local.address + "\n" +
                std::to_string(address.local.port) + "\n" +
                address.remote.address + "\n" +
                std::to_string(address.remote.port) + "\n" +
                type + "\n";
    }

    detail::scan_result message_scanner(beltpp::iterator_wrapper<char const> const& iter_scan_begin,
                                        beltpp::iterator_wrapper<char const> const& iter_scan_end)
    {
        detail::scan_result result;
        result.first = detail::e_scan_result::success;
        result.second = 0;

        bool whole = true;
        auto iter_begin = iter_scan_begin;
        std::string message;

        address = ip_address();
        size_t len = 0;

        auto forward = [&len, &iter_begin](size_t number)
        {
            for (size_t index = 0; index != number; ++index)
            {
                ++iter_begin;
                ++len;
            }
        };
        auto read = [&message, &iter_begin, &iter_scan_end, &whole, &forward]()
        {
            message = detail::read("\n", iter_begin, iter_scan_end, whole);
            forward(message.length() + 1);
        };

        bool error = false;

        if (whole && !error)
        {
            read();
            address.local.address = message;
        }

        if (whole && !error)
        {
            read();

            if (message.empty())
                whole = false;
            else
            {
                size_t ilen;
                try
                {
                    address.local.port = std::stoi(message, &ilen);
                }
                catch(...)
                {
                    ilen = std::string::npos;
                }

                if (ilen != message.length())
                    error = true;
            }
        }

        if (whole && !error)
        {
            read();
            address.remote.address = message;
        }

        if (whole && !error)
        {
            read();

            if (message.empty())
                whole = false;
            else
            {
                size_t ilen;
                try
                {
                    address.remote.port = std::stoi(message, &ilen);
                }
                catch(...)
                {
                    ilen = std::string::npos;
                }

                if (ilen != message.length())
                    error = true;
            }
        }

        if (whole && !error)
        {
            read();
            if (message.empty())
                whole = false;
            else if (message == "ip4")
                address.type = ip_address::e_type::ipv4;
            else if (message == "ip6")
                address.type = ip_address::e_type::ipv6;
            else if (message == "any")
                address.type = ip_address::e_type::any;
            else
                error = true;
        }

        if (whole && !error)
            result.second = len;
        if (error)
        {
            result.first = detail::e_scan_result::error;
            result.second = -1;
        }
        else if (false == whole)
        {
            result.first = detail::e_scan_result::attempt;
            result.second = -1;
        }

        return result;
    }

    ip_address address;
};

class message_code_timer_out : public message_code<message_code_timer_out>
{};

static_assert(inspection::has_message_saver<message_code_hello>::value == 1, "test");
static_assert(inspection::has_message_saver<message_code_drop>::value == 0, "test");
static_assert(inspection::has_message_saver<message_code_error>::value == 0, "test");

namespace detail
{
template <size_t rtt,
          detail::fptr_creator fcreator,
          detail::fptr_scanner fscanner,
          detail::fptr_saver fsaver>
int store_fptr()
{
    message_code_store::s_creators[rtt] = fcreator;
    message_code_store::s_scanners[rtt] = fscanner;
    message_code_store::s_savers[rtt] = fsaver;
    return 0;
}

}

}
