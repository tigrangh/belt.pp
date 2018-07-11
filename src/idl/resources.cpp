#include "resources.hpp"

std::string const resources::file_template = R"file_template(#pragma once

/*
 * the following code is automatically generated
 * idl input/definition/file.idl output/cpp/file.hpp
 */

#include <belt.pp/message_global.hpp>
#include <belt.pp/json.hpp>
#include <belt.pp/packet.hpp>
#include <belt.pp/utility.hpp>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <ctime>
#include <utility>
#include <exception>

namespace {namespace_name}
{
namespace detail
{
inline
bool analyze_json_object(beltpp::json::expression_tree* pexp,
                         size_t& rtt);
inline
bool analyze_json_object(beltpp::json::expression_tree* pexp,
                         beltpp::detail::pmsg_all& return_value,
                         ::beltpp::message_loader_utility const&);
inline
bool analyze_json_common(size_t& rtt,
                         beltpp::json::expression_tree* pexp,
                         std::unordered_map<std::string, beltpp::json::expression_tree*>& members);
inline
bool analyze_colon(beltpp::json::expression_tree* pexp,
                   size_t& rtt);
inline
bool analyze_colon(beltpp::json::expression_tree* pexp,
                   std::unordered_map<std::string, beltpp::json::expression_tree*>& members);
typedef bool(*fptr_utf32_to_utf8)(uint32_t, std::string&);

template <bool (*fpmessage_list_load_helper)
             (::beltpp::json::expression_tree* pexp,
              ::beltpp::detail::pmsg_all& return_value,
              ::beltpp::message_loader_utility const& utl)>
inline
bool template_message_list_load_helper(void* pexp,
                                       ::beltpp::detail::pmsg_all& return_value,
                                       ::beltpp::message_loader_utility const& utl)
{
    return fpmessage_list_load_helper(static_cast<::beltpp::json::expression_tree*>(pexp), return_value, utl);
};

inline
bool message_list_load_helper(::beltpp::json::expression_tree* pexp,
                              ::beltpp::detail::pmsg_all& return_value,
                              ::beltpp::message_loader_utility const& utl)
{
    if (false == analyze_json_object(pexp,
                                     return_value.rtt))
    {
        //  this can happen if a different application is
        //  trying to fool us, by sending incorrect json structures
        return false;
    }
    else if (false == detail::analyze_json_object(pexp, return_value, utl))
        return false;

    return true;
}

inline
void extension_helper(::beltpp::message_loader_utility& utl)
{
    utl.m_arr_fp_message_list_load_helper.push_front(&template_message_list_load_helper<&detail::message_list_load_helper>);
}
}

inline
::beltpp::detail::pmsg_all message_list_load(
        beltpp::iterator_wrapper<char const>& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end,
        beltpp::detail::session_special_data&,
        void* putl)
{
    auto const it_backup = iter_scan_begin;

    ::beltpp::json::ptr_expression_tree pexp;
    ::beltpp::json::expression_tree* proot = nullptr;

    ::beltpp::message_loader_utility utl;
    if (putl)
        utl = *static_cast<::beltpp::message_loader_utility*>(putl);

    ::beltpp::detail::pmsg_all return_value(size_t(-1),
                            ::beltpp::void_unique_ptr(nullptr, [](void*){}),
                            nullptr);

    auto code = ::beltpp::json::parse_stream(pexp,
                                             iter_scan_begin,
                                             iter_scan_end,
                                             1024*1024,
                                             proot);

    if (::beltpp::e_three_state_result::success == code &&
        nullptr == pexp)
    {
        assert(false);
        //  this should not happen, but since this is potentially a
        //  network protocol code, let's assume there is a bug
        //  and not crash, not throw, only report an error
        code = ::beltpp::e_three_state_result::error;
    }
    else if (::beltpp::e_three_state_result::success == code &&
             false == detail::message_list_load_helper(pexp.get(), return_value, utl))
        code = ::beltpp::e_three_state_result::error;
    //
    //
    if (::beltpp::e_three_state_result::error == code)
    {
        iter_scan_begin = iter_scan_end;
        return_value = ::beltpp::detail::pmsg_all(size_t(-2),
                                        ::beltpp::void_unique_ptr(nullptr, [](void*){}),
                                        nullptr);
    }
    else if (::beltpp::e_three_state_result::attempt == code)
    {   //  revert the cursor, so everything will be rescanned
        //  once there is more data to scan
        //  in future may implement persistent state, so rescan will not
        //  be needed
        iter_scan_begin = it_backup;
        return_value = ::beltpp::detail::pmsg_all(size_t(-1),
                                        ::beltpp::void_unique_ptr(nullptr, [](void*){}),
                                        nullptr);
    }

    return return_value;
}

class ctime
{
public:
    inline ctime() : tm() {}
    time_t tm;
    inline bool operator == (ctime const& other) const { return tm == other.tm; }
    inline bool operator != (ctime const& other) const { return tm != other.tm; }
    inline bool operator < (ctime const& other) const { return tm < other.tm; }
    inline bool operator > (ctime const& other) const { return tm > other.tm; }
    inline bool operator <= (ctime const& other) const { return tm <= other.tm; }
    inline bool operator >= (ctime const& other) const { return tm >= other.tm; }
};

namespace detail
{
template <typename T>
inline
bool loader(T& value,
            std::string const& encoded,
            void* putl = nullptr);
inline
std::string saver(int value)
{
    return std::to_string(value);
}
inline
std::string saver(unsigned int value)
{
    return std::to_string(value);
}
inline
std::string saver(std::string const& value)
{
    return ::beltpp::json::value_string::encode(value);
}
inline
std::string saver(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}
inline
std::string saver(int64_t value)
{
    return std::to_string(value);
}
inline
std::string saver(uint64_t value)
{
    return std::to_string(value);
}
inline
std::string saver(float value)
{
    return std::to_string(value);
}
inline
std::string saver(double value)
{
    return std::to_string(value);
}
inline
std::string saver(ctime const& value)
{
    return saver(::beltpp::gm_time_t_to_gm_string(value.tm));
}
inline
bool analyze_json(std::string& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_string::rtt)
        code = false;
    else
        code = ::beltpp::json::value_string::decode(pexp->lexem.value, value);

    return code;
}
inline
bool analyze_json(bool& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_bool::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos = 0;
        if (str_value == "true")
        {
            pos = str_value.length();
            value = true;
        }
        else if (str_value == "false")
        {
            pos = str_value.length();
            value = false;
        }

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
)file_template"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
R"file_template(
inline
bool analyze_json(int8_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoi8(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(uint8_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoui8(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(int16_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoi16(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(uint16_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoui16(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(int32_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoi32(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(uint32_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoui32(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(int64_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoi64(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(uint64_t& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stoui64(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(float& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stof(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(double& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::value_number::rtt)
        code = false;
    else
    {
        std::string const& str_value = pexp->lexem.value;
        size_t pos;
        value = beltpp::stod(str_value, pos);

        if (pos != str_value.length())
            code = false;
    }

    return code;
}
inline
bool analyze_json(ctime& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    std::string str_value;
    if (analyze_json(str_value, pexp, utl) &&
        ::beltpp::gm_string_to_gm_time_t(str_value, value.tm))
        return true;
    return false;
}
inline
std::string saver(::beltpp::packet const& value)
{
    auto buffer = value.save();
    return std::string(buffer.begin(), buffer.end());
}
inline
bool analyze_json(::beltpp::packet& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    ::beltpp::detail::pmsg_all return_value(size_t(-1),
                            ::beltpp::void_unique_ptr(nullptr, [](void*){}),
                            nullptr);

    if (utl.m_fp_message_list_load_helper)
    {
        if (false == utl.m_fp_message_list_load_helper(pexp, return_value, utl))
            return false;
    }
    else if (false == detail::message_list_load_helper(pexp, return_value, utl))
        return false;

    value.set(return_value.rtt,
              std::move(return_value.pmsg),
              return_value.fsaver);

    return true;
}
template <typename T>
inline
bool analyze_json(std::vector<T>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl);
template <typename T>
inline
std::string saver(std::vector<T> const& value);
template <typename T>
inline
bool analyze_json(std::unordered_set<T>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl);
template <typename T>
inline
std::string saver(std::unordered_set<T> const& value);
template <typename T_key, typename T_value>
inline
bool analyze_json(std::unordered_map<T_key, T_value>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl);
template <typename T_value>
inline
bool analyze_json(std::unordered_map<std::string, T_value>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl);
template <typename T_key, typename T_value>
inline
std::string saver(std::unordered_map<T_key, T_value> const& value);
template <typename T_value>
inline
std::string saver(std::unordered_map<std::string, T_value> const& value);

template <typename T>
inline
bool less(T const& first, T const& second)
{
    std::less<T> c;
    return c(first, second);
}
template <typename T_key, typename T_value>
inline
bool less(std::unordered_map<T_key, T_value> const& first,
          std::unordered_map<T_key, T_value> const& second)
{
    std::less<std::string> c;
    return c(saver(first), saver(second));
}
template <typename T>
inline
bool less(std::vector<T> const& first,
          std::vector<T> const& second)
{
    std::less<std::string> c;
    return c(saver(first), saver(second));
}
template <typename T>
inline
bool less(std::unordered_set<T> const& first,
          std::unordered_set<T> const& second)
{
    std::less<std::string> c;
    return c(saver(first), saver(second));
}
inline
bool less(::beltpp::packet const& first,
          ::beltpp::packet const& second)
{
    std::less<std::string> c;
    return c(saver(first), saver(second));
}
inline void assign_packet(::beltpp::packet& self, ::beltpp::packet const& other) noexcept;
inline void assign_packet(::beltpp::packet& self, ::beltpp::packet&& other) noexcept;
template <typename T>
inline void assign_packet(std::vector<T>& self,
                          std::vector<T> const& other);
template <typename T>
inline void assign_packet(std::vector<T>& self,
                          std::vector<T>&& other);
template <typename T>
inline void assign_packet(std::unordered_set<T>& self,
                          std::unordered_set<T> const& other);
template <typename T>
inline void assign_packet(std::unordered_set<T>& self,
                          std::unordered_set<T>&& other);
template <typename T_key, typename T_value>
inline void assign_packet(std::unordered_map<T_key, T_value>& self,
                          std::unordered_map<T_key, T_value> const& other);
template <typename T_key, typename T_value>
inline void assign_packet(std::unordered_map<T_key, T_value>& self,
                          std::unordered_map<T_key, T_value>&& other);
inline void assign_extension(::beltpp::packet& self, ::beltpp::packet const& other) noexcept;
)file_template"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
R"file_template(
inline void assign_extension(::beltpp::packet& self, ::beltpp::packet&& other) noexcept;
template <typename T>
inline void assign_extension(std::vector<T>& self,
                             std::vector<T> const& other);
template <typename T>
inline void assign_extension(std::vector<T>& self,
                             std::vector<T>&& other);
template <typename T>
inline void assign_extension(std::unordered_set<T>& self,
                             std::unordered_set<T> const& other);
template <typename T>
inline void assign_extension(std::unordered_set<T>& self,
                             std::unordered_set<T>&& other);
template <typename T_key, typename T_value>
inline void assign_extension(std::unordered_map<T_key, T_value>& self,
                             std::unordered_map<T_key, T_value> const& other);
template <typename T_key, typename T_value>
inline void assign_extension(std::unordered_map<T_key, T_value>& self,
                             std::unordered_map<T_key, T_value>&& other);
}   // end namespace detail
}   // end namespace {namespace_name}

namespace {namespace_name}
{

{expand_message_classes}

namespace detail
{
template <typename T>
std::string stringsaver(T const& value);
template <typename T>
bool stringloader(T& value,
                  std::string const& encoded);
template <typename T>
std::string saver(std::vector<T> const& value)
{
    std::string result = "[";
    for (size_t index = 0; index < value.size(); ++index)
    {
        result += saver(value[index]);
        if (index + 1 != value.size())
            result += ",";
    }
    result += "]";
    return result;
}
template <typename T>
bool analyze_json(std::vector<T>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    value.clear();
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_bracket::rtt)
        code = false;
    else
    {
        auto pscan = pexp;
        if (pexp->children.size() == 1 &&
            pexp->children.front()->lexem.rtt == ::beltpp::json::operator_comma::rtt &&
            false == pexp->children.front()->children.empty())
            pscan = pexp->children.front();

        for (auto const& item : pscan->children)
        {
            T item_value;
            if (analyze_json(item_value, item, utl))
                value.push_back(std::move(item_value));
            else
            {
                code = false;
                break;
            }
        }
    }

    return code;
}
template <typename T>
std::vector<T const*>
    set2vector(std::unordered_set<T> const& value)
{
    std::vector<T const*> result;
    result.reserve(value.size());
    for (auto const& item : value)
        result.push_back(&item);

    std::sort(result.begin(), result.end(),
          [](T const* first,
             T const* second)
          {
              return *first < *second;
          });

    return result;
}
template <typename T>
std::string saver(std::unordered_set<T> const& value)
{
    std::vector<T const*> arr_value = set2vector(value);
    std::string result = "[";
    for (size_t index = 0; index < arr_value.size(); ++index)
    {
        result += saver(*arr_value[index]);
        if (index + 1 != arr_value.size())
            result += ",";
    }
    result += "]";
    return result;
}
template <typename T>
bool analyze_json(std::unordered_set<T>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    value.clear();
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_bracket::rtt)
        code = false;
    else
    {
        auto pscan = pexp;
        if (pexp->children.size() == 1 &&
            pexp->children.front()->lexem.rtt == ::beltpp::json::operator_comma::rtt &&
            false == pexp->children.front()->children.empty())
            pscan = pexp->children.front();

        for (auto const& item : pscan->children)
        {
            T item_value;
            if (analyze_json(item_value, item, utl))
            {
                auto it_code = value.insert(std::move(item_value));
                if (false == it_code.second)
                {
                    code = false;
                    break;
                }
            }
            else
            {
                code = false;
                break;
            }
        }
    }

    return code;
}
template <typename T_first, typename T_second>
std::string saver(std::pair<T_first, T_second> const& value)
{
    std::string result = "[";
    result += saver(value.first);
    result += ",";
    result += saver(value.second);
    result += "]";
    return result;
}
template <typename T_first, typename T_second>
bool analyze_json(std::pair<T_first, T_second>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_bracket::rtt ||
        pexp->children.size() != 1 ||
        pexp->children.front()->lexem.rtt != ::beltpp::json::operator_comma::rtt ||
        pexp->children.front()->children.size() != 2)
        code = false;
    else
    {
        auto const& pair_item = pexp->children.front();
        if (false == analyze_json(value.first, pair_item->children.front(), utl) ||
            false == analyze_json(value.second, pair_item->children.back(), utl))
            code = false;
    }

    return code;
}
template <typename T_key, typename T_value>
std::vector<std::pair<T_key const, T_value> const*>
    map2vector(std::unordered_map<T_key, T_value> const& value)
{
    std::vector<std::pair<T_key const, T_value> const*> result;
    result.reserve(value.size());
    for (auto const& item : value)
        result.push_back(&item);

    std::sort(result.begin(), result.end(),
          [](std::pair<T_key const, T_value> const* first,
             std::pair<T_key const, T_value> const* second)
          {
              return first->first < second->first;
          });

    return result;
}
template <typename T_key, typename T_value>
std::string saver(std::unordered_map<T_key, T_value> const& value)
{
    auto arr_value = map2vector(value);

    std::string result = "[";
    auto it = arr_value.begin();
    for (; it != arr_value.end(); ++it)
    {
        result += saver(*(*it));
        auto it_temp = it;
        ++it_temp;
        if (it_temp != arr_value.end())
            result += ",";
    }
    result += "]";
    return result;
}
template <typename T_value>
std::string saver(std::unordered_map<std::string, T_value> const& value)
{
    auto arr_value = map2vector(value);

    std::string result = "{";
    auto it = arr_value.begin();
    for (; it != arr_value.end(); ++it)
    {
        result += stringsaver((*it)->first);
        result += ":";
        result += saver((*it)->second);
        auto it_temp = it;
        ++it_temp;
        if (it_temp != arr_value.end())
            result += ",";
    }
    result += "}";
    return result;
}
template <typename T_key, typename T_value>
bool analyze_json(std::unordered_map<T_key, T_value>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    value.clear();
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_bracket::rtt)
        code = false;
    else
    {
        auto pscan = pexp;
        if (pexp->children.size() == 1 &&
            pexp->children.front()->lexem.rtt == ::beltpp::json::operator_comma::rtt &&
            false == pexp->children.front()->children.empty())
            pscan = pexp->children.front();

        for (auto const& item : pscan->children)
        {
            std::pair<T_key, T_value> item_value;
            if (analyze_json(item_value, item, utl))
                value.insert(std::move(item_value));
            else
            {
                code = false;
                break;
            }
        }
    }

    return code;
}

template <typename T_value>
bool analyze_json(std::unordered_map<std::string, T_value>& value,
                  ::beltpp::json::expression_tree* pexp,
                  ::beltpp::message_loader_utility const& utl)
{
    value.clear();
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_brace::rtt)
        code = false;
    else
    {
        auto pscan = pexp;
        if (pexp->children.size() == 1 &&
            pexp->children.front()->lexem.rtt == ::beltpp::json::operator_comma::rtt &&
            false == pexp->children.front()->children.empty())
            pscan = pexp->children.front();

        for (auto const& item : pscan->children)
        {
            if (item->lexem.rtt == ::beltpp::json::operator_colon::rtt &&
                item->children.size() == 2)
            {
                std::string item_key;
                T_value item_value;
                std::string decoded;
                if (item->children.front()->lexem.rtt == ::beltpp::json::value_string::rtt &&
                    stringloader(item_key, item->children.front()->lexem.value) &&
                    analyze_json(item_value, item->children.back(), utl))
                    value.insert(std::move(std::make_pair(item_key, std::move(item_value))));
                else
                {
                    code = false;
                    break;
                }
            }
            else
            {
                code = false;
                break;
            }
        }
    }

    return code;
}

template <typename T>
bool loader(T& value,
            std::string const& encoded,
            void* putl)
{
    //  add space, because parser assumes a stream, and may not parse last symbols
    std::string encoded2 = encoded + " ";
    ::beltpp::iterator_wrapper<char const> iter_scan_begin(encoded2.begin());
    ::beltpp::iterator_wrapper<char const> iter_scan_end(encoded2.end());

    ::beltpp::json::ptr_expression_tree pexp;
    ::beltpp::json::expression_tree* proot = nullptr;

    ::beltpp::message_loader_utility utl;
    if (putl)
        utl = *static_cast<::beltpp::message_loader_utility*>(putl);

    auto code = ::beltpp::json::parse_stream(pexp, iter_scan_begin,
                                             iter_scan_end, 1024*1024, proot);

    if (code != ::beltpp::e_three_state_result::success)
        return false;
    if (nullptr == pexp)
        return false;

    return analyze_json(value, pexp.get(), utl);
}

template <typename T>
std::string stringsaver(T const& value)
{
    return saver(saver(value));
}
template <>
inline
std::string stringsaver(std::string const& value)
{
    return saver(value);
}
template <typename T>
inline
bool stringloader(T& value,
                  std::string const& encoded)
{
    std::string decoded;
    if (loader(decoded, encoded) &&
        loader(value, decoded))
        return true;
    return false;
}
template <>
inline
bool stringloader(std::string& value,
                  std::string const& encoded)
{
    return loader(value, encoded);
}

inline
bool analyze_json_common(size_t& rtt,
                         ::beltpp::json::expression_tree* pexp,
                         std::unordered_map<std::string, beltpp::json::expression_tree*>& members)
{
    bool code = false;

    if (nullptr == pexp ||
        pexp->lexem.rtt != ::beltpp::json::scope_brace::rtt ||
        1 != pexp->children.size())
        code = false;
    else if (pexp->children.front() &&
             pexp->children.front()->lexem.rtt ==
             ::beltpp::json::operator_colon::rtt)
    {
        if (analyze_colon(pexp->children.front(), rtt))
            code = true;
        else
            code = false;
    }
    else if (nullptr == pexp->children.front() ||
             pexp->children.front()->lexem.rtt !=
             ::beltpp::json::operator_comma::rtt ||
             pexp->children.front()->children.empty())
    {
        code = false;
    }
    else
    {
        code = false;
        auto pcomma = pexp->children.front();
        for (auto item : pcomma->children)
        {
            size_t rtt_temp = -1;
            if (false == analyze_colon(item, rtt_temp) &&
                false == analyze_colon(item, members))
                break;

            if (size_t(-1) != rtt_temp)
                rtt = rtt_temp;
        }

        if (members.size() + 1 == pcomma->children.size())
            code = true;
    }

    return code;
}
)file_template"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
R"file_template(
inline
bool analyze_json_object(::beltpp::json::expression_tree* pexp,
                         size_t& rtt)
{
    bool code = false;
    rtt = -1;

    if (nullptr == pexp ||
        pexp->lexem.rtt != beltpp::json::scope_brace::rtt ||
        1 != pexp->children.size())
        code = false;
    else if (pexp->children.front() &&
             pexp->children.front()->lexem.rtt ==
             ::beltpp::json::operator_colon::rtt)
    {
        if (analyze_colon(pexp->children.front(), rtt))
            code = true;
        else
            code = false;
    }
    else if (nullptr == pexp->children.front() ||
             pexp->children.front()->lexem.rtt !=
             ::beltpp::json::operator_comma::rtt ||
             pexp->children.front()->children.empty())
    {
        code = false;
    }
    else
    {
        code = false;
        auto pcomma = pexp->children.front();
        for (auto item : pcomma->children)
        {
            if (analyze_colon(item, rtt))
            {
                code = true;
                break;
            }
        }
    }

    return code;
}

inline
bool analyze_colon(::beltpp::json::expression_tree* pexp,
                   size_t& rtt)
{
    rtt = -1;

    if (pexp &&
        pexp->lexem.rtt == ::beltpp::json::operator_colon::rtt &&
        2 == pexp->children.size() &&
        pexp->children.front() &&
        pexp->children.back() &&
        pexp->children.front()->lexem.rtt ==
        ::beltpp::json::value_string::rtt)
    {
        auto key = pexp->children.front()->lexem.value;
        if (key != "\"rtt\"")
        {
            //members.insert(std::make_pair(key, item->children.back()));
        }
        else if (pexp->children.back() &&
                 pexp->children.back()->lexem.rtt ==
                 ::beltpp::json::value_number::rtt)
        {
            size_t pos;
            auto const& value = pexp->children.back()->lexem.value;
            rtt = beltpp::stoui32(value, pos);
            if (pos != value.size())
                rtt = -1;
        }
    }

    return (size_t(-1) != rtt);
}

inline
bool analyze_colon(::beltpp::json::expression_tree* pexp,
                   std::unordered_map<std::string, ::beltpp::json::expression_tree*>& members)
{
    bool code = false;

    if (pexp &&
        pexp->lexem.rtt == ::beltpp::json::operator_colon::rtt &&
        2 == pexp->children.size() &&
        pexp->children.front() &&
        pexp->children.back() &&
        pexp->children.front()->lexem.rtt ==
        ::beltpp::json::value_string::rtt)
    {
        auto key = pexp->children.front()->lexem.value;

        members.insert(std::make_pair(key, pexp->children.back()));
        code = true;
    }

    return code;
}

inline
void assign_packet(::beltpp::packet& self, ::beltpp::packet const& other) noexcept
{
    if ({namespace_name}::detail::storage<>::s_arr_fptr.size() <= other.type())
    {
        assert(false);
        std::terminate();
    }

    auto const& item = {namespace_name}::detail::storage<>::s_arr_fptr[other.type()];

    self.set(other.type(),
             item.fp_new_void_unique_ptr_copy(other.data()),
             item.fp_saver);
}
inline
void assign_packet(::beltpp::packet& self, ::beltpp::packet&& other) noexcept
{
    self = std::move(other);
}
template <typename T>
inline
void assign_packet(std::vector<T>& self,
                   std::vector<T> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T self_item;
        assign_packet(self_item, other_item);
        self.push_back(std::move(self_item));
    }
}
template <typename T>
inline
void assign_packet(std::vector<T>& self,
                   std::vector<T>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T self_item;
        assign_packet(self_item, std::move(other_item));
        self.push_back(std::move(self_item));
    }
}
template <typename T>
inline
void assign_packet(std::unordered_set<T>& self,
                   std::unordered_set<T> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T self_item;
        assign_packet(self_item, other_item);
        auto it_code = self.insert(std::move(self_item));
        assert(it_code.second);
    }
}
template <typename T>
inline
void assign_packet(std::unordered_set<T>& self,
                   std::unordered_set<T>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T self_item;
        assign_packet(self_item, std::move(other_item));
        auto it_code = self.insert(std::move(self_item));
        assert(it_code.second);
    }
}
template <typename T_key, typename T_value>
inline
void assign_packet(std::unordered_map<T_key, T_value>& self,
                   std::unordered_map<T_key, T_value> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T_key self_key(other_item.first);
        T_value self_value;
        assign_packet(self_value, other_item.second);
        self.insert(std::make_pair(std::move(self_key), std::move(self_value)));
    }
}
template <typename T_key, typename T_value>
inline
void assign_packet(std::unordered_map<T_key, T_value>& self,
                   std::unordered_map<T_key, T_value>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T_key self_key(std::move(other_item.first));
        T_value self_value;
        assign_packet(self_value, std::move(other_item.second));
        self.insert(std::make_pair(std::move(self_key), std::move(self_value)));
    }
}
inline
void assign_extension(::beltpp::packet& self, ::beltpp::packet const& other) noexcept
{
    //  todo
    //  if ({namespace_name}::detail::storage<>::s_arr_fptr.size() <= other.type())
    {
        assert(false);
        std::terminate();
    }

    auto const& item = {namespace_name}::detail::storage<>::s_arr_fptr[other.type()];

    self.set(other.type(),
             item.fp_new_void_unique_ptr_copy(other.data()),
             item.fp_saver);
}
inline
void assign_extension(::beltpp::packet& self, ::beltpp::packet&& other) noexcept
{
    self = std::move(other);
}
template <typename T>
inline
void assign_extension(std::vector<T>& self,
                      std::vector<T> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T self_item;
        assign_extension(self_item, other_item);
        self.push_back(std::move(self_item));
    }
}
template <typename T>
inline
void assign_extension(std::vector<T>& self,
                      std::vector<T>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T self_item;
        assign_extension(self_item, std::move(other_item));
        self.push_back(std::move(self_item));
    }
}
template <typename T>
inline
void assign_extension(std::unordered_set<T>& self,
                      std::unordered_set<T> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T self_item;
        assign_extension(self_item, other_item);
        auto it_code = self.insert(std::move(self_item));
        assert(it_code.second);
    }
}
template <typename T>
inline
void assign_extension(std::unordered_set<T>& self,
                      std::unordered_set<T>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T self_item;
        assign_extension(self_item, std::move(other_item));
        auto it_code = self.insert(std::move(self_item));
        assert(it_code.second);
    }
}
template <typename T_key, typename T_value>
inline
void assign_extension(std::unordered_map<T_key, T_value>& self,
                      std::unordered_map<T_key, T_value> const& other)
{
    self.clear();
    for (auto const& other_item : other)
    {
        T_key self_key(other_item.first);
        T_value self_value;
        assign_extension(self_value, other_item.second);
        self.insert(std::make_pair(std::move(self_key), std::move(self_value)));
    }
}
template <typename T_key, typename T_value>
inline
void assign_extension(std::unordered_map<T_key, T_value>& self,
                      std::unordered_map<T_key, T_value>&& other)
{
    self.clear();
    for (auto& other_item : other)
    {
        T_key self_key(std::move(other_item.first));
        T_value self_value;
        assign_extension(self_value, std::move(other_item.second));
        self.insert(std::make_pair(std::move(self_key), std::move(self_value)));
    }
}

}   //  end namespace detail
}   //  end namespace {namespace_name}

)file_template";
