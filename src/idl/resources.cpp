#include "resources.hpp"

std::string const resources::file_template = R"file_template(/*
 * the following code is automatically generated
 * idl input/definition/file.idl output/cpp/file.hpp
 */

#include <belt.pp/message_global.hpp>
#include <belt.pp/json.hpp>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {namespace_name}
{
namespace detail
{
class utils;
bool analyze_json_object(beltpp::json::expression_tree* pexp,
                         size_t& rtt);
bool analyze_json_object(beltpp::json::expression_tree* pexp,
                         pmsg_all& return_value,
                         utils const&);
bool analyze_json_common(size_t& rtt,
                         beltpp::json::expression_tree* pexp,
                         std::unordered_map<std::string, beltpp::json::expression_tree*>& members);
bool analyze_colon(beltpp::json::expression_tree* pexp,
                   size_t& rtt);
bool analyze_colon(beltpp::json::expression_tree* pexp,
                   std::unordered_map<std::string, beltpp::json::expression_tree*>& members);
typedef bool(*fptr_utf32_to_utf8)(uint32_t, std::string&);
class utils
{
};
}
detail::pmsg_all message_list_load(
        beltpp::iterator_wrapper<char const>& iter_scan_begin,
        beltpp::iterator_wrapper<char const> const& iter_scan_end)
{
    auto const it_backup = iter_scan_begin;

    json::ptr_expression_tree pexp;
    json::expression_tree* proot = nullptr;

    detail::utils utl;

    detail::pmsg_all return_value (size_t(-1),
                                   detail::ptr_msg(nullptr, [](void*&){}),
                                   nullptr);

    auto code = json::parse_stream(pexp, iter_scan_begin,
                                   iter_scan_end, 1024*1024, proot);

    if (e_three_state_result::success == code &&
        nullptr == pexp)
    {
        assert(false);
        //  this should not happen, but since this is potentially a
        //  network protocol code, let's assume there is a bug
        //  and not crash, not throw, only report an error
        code = e_three_state_result::error;
    }
    else if (e_three_state_result::success == code &&
             false == detail::analyze_json_object(pexp.get(),
                                                  return_value.rtt))
    {
        //  this can happen if a different application is
        //  trying to fool us, by sending incorrect json structures
        code = e_three_state_result::error;
    }
    else if (e_three_state_result::success == code &&
             false == detail::analyze_json_object(pexp.get(), return_value, utl))
    {
        code = e_three_state_result::error;
    }
    //
    //
    if (e_three_state_result::error == code)
    {
        iter_scan_begin = iter_scan_end;
        return_value = detail::pmsg_all(0,
                                        detail::ptr_msg(nullptr, [](void*&){}),
                                        nullptr);
    }
    else if (e_three_state_result::attempt == code)
    {   //  revert the cursor, so everything will be rescaned
        //  once there is more data to scan
        //  in future may implement persistent state, so rescan will not
        //  be needed
        iter_scan_begin = it_backup;
        return_value = detail::pmsg_all(size_t(-1),
                                        detail::ptr_msg(nullptr, [](void*&){}),
                                        nullptr);
    }

    return return_value;
}

namespace detail
{
std::string saver(int value)
{
    return std::to_string(value);
}
std::string saver(std::string const& value)
{
    return beltpp::json::value_string::encode(value);
}
std::string saver(bool value)
{
    if (value)
        return "true";
    else
        return "false";
}
std::string saver(int64_t value)
{
    return std::to_string(value);
}
std::string saver(uint64_t value)
{
    return std::to_string(value);
}
std::string saver(float value)
{
    return std::to_string(value);
}
std::string saver(double value)
{
    return std::to_string(value);
}
template <typename T>
bool analyze_json(std::vector<T>& value,
                  beltpp::json::expression_tree* pexp,
                  utils const& utl)
{
    value.clear();
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::scope_bracket::rtt)
        code = false;
    else
    {
        auto pscan = pexp;
        if (pexp->children.size() == 1 &&
            pexp->children.front()->lexem.rtt == json::operator_comma::rtt &&
            false == pexp->children.front()->children.empty())
            pscan = pexp->children.front();

        for (auto const& item : pscan->children)
        {
            T item_value;
            if (analyze_json(item_value, item, utl))
                value.push_back(item_value);
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
bool analyze_json(std::string& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_string::rtt)
        code = false;
    else
        code = beltpp::json::value_string::decode(pexp->lexem.value, value);

    return code;
}
bool analyze_json(bool& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_bool::rtt)
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
bool analyze_json(int8_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(uint8_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(int16_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(uint16_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(int32_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(uint32_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(int64_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(uint64_t& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(float& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
bool analyze_json(double& value,
                  beltpp::json::expression_tree* pexp,
                  utils const&)
{
    bool code = true;
    if (nullptr == pexp ||
        pexp->lexem.rtt != json::value_number::rtt)
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
}

{expand_message_classes}

namespace detail
{
bool analyze_json_common(size_t& rtt,
                         beltpp::json::expression_tree* pexp,
                         std::unordered_map<std::string, beltpp::json::expression_tree*>& members)
{
    bool code = false;

    if (nullptr == pexp ||
        pexp->lexem.rtt != beltpp::json::scope_brace::rtt ||
        1 != pexp->children.size())
        code = false;
    else if (pexp->children.front() &&
             pexp->children.front()->lexem.rtt ==
             beltpp::json::operator_colon::rtt)
    {
        if (analyze_colon(pexp->children.front(), rtt))
            code = true;
        else
            code = false;
    }
    else if (nullptr == pexp->children.front() ||
             pexp->children.front()->lexem.rtt !=
             beltpp::json::operator_comma::rtt ||
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

bool analyze_json_object(beltpp::json::expression_tree* pexp,
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
             beltpp::json::operator_colon::rtt)
    {
        if (analyze_colon(pexp->children.front(), rtt))
            code = true;
        else
            code = false;
    }
    else if (nullptr == pexp->children.front() ||
             pexp->children.front()->lexem.rtt !=
             beltpp::json::operator_comma::rtt ||
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

bool analyze_colon(beltpp::json::expression_tree* pexp,
                   size_t& rtt)
{
    rtt = -1;

    if (pexp &&
        pexp->lexem.rtt == beltpp::json::operator_colon::rtt &&
        2 == pexp->children.size() &&
        pexp->children.front() &&
        pexp->children.back() &&
        pexp->children.front()->lexem.rtt ==
        beltpp::json::value_string::rtt)
    {
        auto key = pexp->children.front()->lexem.value;
        if (key != "\"rtt\"")
        {
            //members.insert(std::make_pair(key, item->children.back()));
        }
        else if (pexp->children.back() &&
                 pexp->children.back()->lexem.rtt ==
                 beltpp::json::value_number::rtt)
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

bool analyze_colon(beltpp::json::expression_tree* pexp,
                   std::unordered_map<std::string, beltpp::json::expression_tree*>& members)
{
    bool code = false;

    if (pexp &&
        pexp->lexem.rtt == beltpp::json::operator_colon::rtt &&
        2 == pexp->children.size() &&
        pexp->children.front() &&
        pexp->children.back() &&
        pexp->children.front()->lexem.rtt ==
        beltpp::json::value_string::rtt)
    {
        auto key = pexp->children.front()->lexem.value;

        members.insert(std::make_pair(key, pexp->children.back()));
        code = true;
    }

    return code;
}
}
}

)file_template";
