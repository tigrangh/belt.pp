#pragma once

#include "global.hpp"
#include "parser.hpp"

#include <memory>
#include <sstream>
#include <cstdint>

namespace beltpp
{
namespace json
{
using lexers = beltpp::typelist::type_list<
class operator_comma,
class scope_brace,
class scope_bracket,
class operator_colon,
class value_string,
class value_number,
class value_bool,
class value_null,
class discard
>;

template <typename T=void>
class operator_set { public: static const std::string value; };
template <typename T>
std::string const operator_set<T>::value = ",:";

class operator_comma :
        public beltpp::operator_lexer_base<operator_comma, lexers>
{
public:
    size_t right = 1;
    size_t left_min = 1;
    size_t left_max = size_t(-1);
    size_t property = 1;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
    {
        //return beltpp::standard_operator_check<operator_set<>>(ch);
        return ch == ',' ? beltpp::e_three_state_result::attempt
                         : beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        //return std::string(it_begin, it_end) == ",";
        T_iterator other = it_begin;
        ++other;
        return (it_begin != it_end &&
                other == it_end &&
                *it_begin == ',');
    }
};

class operator_colon :
        public beltpp::operator_lexer_base<operator_colon, lexers>
{
public:
    size_t right = 1;
    size_t left_min = 1;
    size_t left_max = 1;
    size_t property = 2;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
    {
        //return beltpp::standard_operator_check<operator_set<>>(ch);
        return ch == ':' ? beltpp::e_three_state_result::attempt
                         : beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        //return std::string(it_begin, it_end) == ":";
        T_iterator other = it_begin;
        ++other;
        return (it_begin != it_end &&
                other == it_end &&
                *it_begin == ':');
    }
};

class scope_brace : public beltpp::operator_lexer_base<scope_brace, lexers>
{
public:
    size_t right = size_t(-1);
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 8;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
    {
        if (ch == '{')
        {
            right = size_t(-1);
            left_min = 0;
            left_max = 0;
            property = 8;
            return beltpp::e_three_state_result::success;
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = 1;
            property = 4;
            return beltpp::e_three_state_result::success;
        }
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        /*std::string value(it_begin, it_end);
        if (value == "{" || value == "}")
            return true;
        return false;*/
        T_iterator other = it_begin;
        ++other;
        return (it_begin != it_end &&
                other == it_end &&
                (*it_begin == '{' || *it_begin == '}'));
    }
};

class scope_bracket : public beltpp::operator_lexer_base<scope_bracket, lexers>
{
public:
    size_t right = size_t(-1);
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 8;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
    {
        if (ch == '[')
        {
            right = size_t(-1);
            left_min = 0;
            left_max = 0;
            property = 8;
            return beltpp::e_three_state_result::success;
        }
        if (ch == ']')
        {
            right = 0;
            left_min = 0;
            left_max = 1;
            property = 4;
            return beltpp::e_three_state_result::success;
        }
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        /*std::string value(it_begin, it_end);
        if (value == "[" || value == "]")
            return true;
        return false;*/
        T_iterator other = it_begin;
        ++other;
        return (it_begin != it_end &&
                other == it_end &&
                (*it_begin == '[' || *it_begin == ']'));
    }
};

namespace detail
{
enum class utf16_range {bmp, high, low};
using uchar = unsigned char;
inline utf16_range utf16_check(uint16_t code)
{
    if (0xD800 <= code && code <= 0xDBFF)
        return utf16_range::high;
    if (0xDC00 <= code && code <= 0xDFFF)
        return utf16_range::low;
    return utf16_range::bmp;
}
inline uint32_t utf16_surrogate_pair_to_code_point(uint16_t high, uint16_t low)
{
    assert(utf16_check(high) == utf16_range::high);
    assert(utf16_check(low) == utf16_range::low);
    return 0x10000 + 0x400*(high - 0xD800) + (low - 0xDC00);
}
static_assert(0x10000 + 0x400*(uint16_t(0xD801) - 0xD800) +
              (uint16_t(0xDC01) - 0xDC00) == 0x10401, "sure?");
static_assert(0x10000 + 0x400*(uint16_t(0xDBFF) - 0xD800) +
              (uint16_t(0xDFFF) - 0xDC00) == 0x10FFFF, "sure?");

inline size_t utf32_to_utf8(uint32_t cp, char (&buffer)[4]) noexcept
{
    if (cp <= 0x7F)
    {
        buffer[0] = static_cast<char>(cp);      //  0xxxxxxx
        return 1;
    }
    if (cp <= 0x7FF)
    {
        buffer[0] = char(0xC0 | (cp >> 6));     //  110xxxxx
        buffer[1] = char(0x80 | (cp & 0x3F));   //  10xxxxxx
        return 2;
    }
    if (cp <= 0xFFFF)
    {
        buffer[0] = char(0xE0 | (cp >> 12));          //  1110xxxx
        buffer[1] = char(0x80 | ((cp >> 6) & 0x3F));  //  10xxxxxx
        buffer[2] = char(0x80 | (cp & 0x3F));         //  10xxxxxx
        return 3;
    }
    if (cp <= 0x10FFFF)
    {
        buffer[0] = char(0xF0 | (cp >> 18));          //  11110xxx
        buffer[1] = char(0x80 | ((cp >> 12) & 0x3F)); //  10xxxxxx
        buffer[2] = char(0x80 | ((cp >> 6) & 0x3F));  //  10xxxxxx
        buffer[3] = char(0x80 | (cp & 0x3F));         //  10xxxxxx
        return 4;
    }

    assert(false);
    //  the input to this function is either a code such as \uABCD
    //  or a resulting code of a surrogate pair
    //  so reaching here is not possible
    std::terminate();

    //  unreachable code
    //return 0;
}
}

class value_string :
        public beltpp::value_lexer_base<value_string, lexers>
{
    size_t escape_sequence_index = 0;
    size_t escape_sequence_remaining = 0;
    size_t index = size_t(-1);
    bool final_check_status = false;
public:
    beltpp::e_three_state_result check(unsigned char ch)
    {
        ++index;
        if (0 == index)
        {
            if (ch == '\"')
                return beltpp::e_three_state_result::attempt;
            if (ch != '\"')
                return beltpp::e_three_state_result::error;
        }
        if (0 == escape_sequence_remaining && ch == '\\')
        {
            ++escape_sequence_remaining;
            escape_sequence_index = 1;
            return beltpp::e_three_state_result::attempt;
        }
        if (0 < escape_sequence_remaining &&
            1 == escape_sequence_index)
        {
            if ('\"' == ch || '\\' == ch ||
                '/' == ch || 'b' == ch ||
                'f' == ch || 'n' == ch ||
                'r' == ch || 't' == ch)
            {
                escape_sequence_index = 0;
                escape_sequence_remaining = 0;
                return beltpp::e_three_state_result::attempt;
            }
            else if ('u' == ch)
            {
                ++escape_sequence_index;
                escape_sequence_remaining = 4;
                return beltpp::e_three_state_result::attempt;
            }
            else    //  unsupported escape sequence
                return beltpp::e_three_state_result::error;
        }
        if (0 < escape_sequence_remaining)
        {
            assert(2 <= escape_sequence_index);
            assert(6 > escape_sequence_index);

            if (('0' <= ch && ch <= '9') ||
                ('a' <= tolower(ch) && tolower(ch) <= 'f'))
            {
                --escape_sequence_remaining;
                ++escape_sequence_index;
                if (0 == escape_sequence_remaining)
                    escape_sequence_index = 0;
                return beltpp::e_three_state_result::attempt;
            }
            else    //  unsupported escape sequence
                return beltpp::e_three_state_result::error;
        }

        assert(0 == escape_sequence_remaining);
        assert(0 == escape_sequence_index);

        if ('\"' == ch)
        {
            final_check_status = true;
            return beltpp::e_three_state_result::success;
        }
        if ('\x20' > ch)    //  unsupported charachter
            return beltpp::e_three_state_result::error;

        return beltpp::e_three_state_result::attempt;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const&/* it_begin*/,
                     T_iterator const&/* it_end*/) const
    {
        return final_check_status;
    }

    static std::string encode(std::string const& utf8_value)
    {
        auto it_end = utf8_value.end();
        auto it_begin = utf8_value.begin();

        std::ostringstream out;
        out << '\"';
        for (auto it = it_begin; it != it_end; ++it)
        {
            switch (*it)
            {
            case '\x00': out << "\\u0000"; break;
            case '\x01': out << "\\u0001"; break;
            case '\x02': out << "\\u0002"; break;
            case '\x03': out << "\\u0003"; break;
            case '\x04': out << "\\u0004"; break;
            case '\x05': out << "\\u0005"; break;
            case '\x06': out << "\\u0006"; break;
            case '\x07': out << "\\u0007"; break;
            case '\x08': out << "\\b"; break;
            case '\x09': out << "\\t"; break;
            case '\x0a': out << "\\n"; break;
            case '\x0b': out << "\\u000b"; break;
            case '\x0c': out << "\\f"; break;
            case '\x0d': out << "\\r"; break;
            case '\x0e': out << "\\u000e"; break;
            case '\x0f': out << "\\u000f"; break;

            case '\x10': out << "\\u0010"; break;
            case '\x11': out << "\\u0011"; break;
            case '\x12': out << "\\u0012"; break;
            case '\x13': out << "\\u0013"; break;
            case '\x14': out << "\\u0014"; break;
            case '\x15': out << "\\u0015"; break;
            case '\x16': out << "\\u0016"; break;
            case '\x17': out << "\\u0017"; break;
            case '\x18': out << "\\u0018"; break;
            case '\x19': out << "\\u0019"; break;
            case '\x1a': out << "\\u001a"; break;
            case '\x1b': out << "\\u001b"; break;
            case '\x1c': out << "\\u001c"; break;
            case '\x1d': out << "\\u001d"; break;
            case '\x1e': out << "\\u001e"; break;
            case '\x1f': out << "\\u001f"; break;

            case '\\': out << "\\\\"; break;
            case '\"': out << "\\\""; break;
            default: out << *it;
            }
        }
        out << '\"';

        return out.str();
    }

    static bool decode(std::string const& utf8_encoded,
                       std::string& utf8_value)
    {
        bool code = true;
        utf8_value.reserve(utf8_encoded.size());

        auto it_begin = utf8_encoded.begin();
        auto it_end = utf8_encoded.end();

        //  these two keep the state to parse escape sequences
        size_t escape_sequence_index = 0;
        size_t escape_sequence_remaining = 0;

        uint32_t code_point_encoded = uint32_t(-1);
        uint32_t surrogate_pair_high = uint32_t(-1);

        bool proper_ending = false;

        for (auto it = it_begin; code && it != it_end; ++it)
        {
            char ch = *it;
            std::string item;

            if (it_begin == it)
            {
                if (ch != '\"')
                    code = false;
            }
            else if (0 == escape_sequence_remaining && ch == '\\')
            {
                ++escape_sequence_remaining;
                escape_sequence_index = 1;
            }
            else if (0 < escape_sequence_remaining &&
                     1 == escape_sequence_index)
            {
                if ('\"' == ch || '\\' == ch ||
                    '/' == ch || 'b' == ch ||
                    'f' == ch || 'n' == ch ||
                    'r' == ch || 't' == ch)
                {
                    escape_sequence_index = 0;
                    escape_sequence_remaining = 0;
                    switch (ch)
                    {
                    case '\"': item += ch; break;
                    case '\\': item += ch; break;
                    case '/': item += ch; break;
                    case 'b': item += '\x08'; break;
                    case 'f': item += '\x0c'; break;
                    case 'n': item += '\n'; break;
                    case 'r': item += '\r'; break;
                    case 't': item += '\t'; break;
                    }
                }
                else if ('u' == ch)
                {
                    ++escape_sequence_index;
                    escape_sequence_remaining = 4;
                }
                else    //  unsupported escape sequence
                    code = false;
            }
            else if (0 < escape_sequence_remaining)
            {
                assert(2 <= escape_sequence_index);
                assert(6 > escape_sequence_index);

                if (('0' <= ch && ch <= '9') ||
                    ('a' <= tolower(ch) && tolower(ch) <= 'f'))
                {
                    uint32_t ch_value = 0;
                    if ('0' <= ch && ch <= '9')
                        ch_value = uint32_t(ch - '0');
                    else if ('a' <= tolower(ch) && tolower(ch) <= 'f')
                        ch_value = uint32_t(10 + tolower(ch) - 'a');

                    if (2 == escape_sequence_index)
                        code_point_encoded = ch_value;
                    else
                    {
                        code_point_encoded *= 16;
                        code_point_encoded += ch_value;
                    }

                    --escape_sequence_remaining;
                    ++escape_sequence_index;
                    if (0 == escape_sequence_remaining)
                        escape_sequence_index = 0;
                }
                else    //  unsupported escape sequence
                    code = false;
            }
            else if ('\"' == ch)
            {
                auto test = it;
                ++test;
                if (test != it_end)
                    code = false;
                else
                    proper_ending = true;
            }
            else if ('\x20' > detail::uchar(ch))    //  unsupported charachter
                code = false;
            else
                item += ch;

            if (code &&
                0 == escape_sequence_remaining &&
                false == proper_ending &&
                it != it_begin)
            {
                assert((uint32_t(-1) != code_point_encoded && item.empty()) ||
                       (uint32_t(-1) == code_point_encoded && false == item.empty()));

                if (uint32_t(-1) != surrogate_pair_high &&
                    uint32_t(-1) != code_point_encoded &&
                    detail::utf16_check(uint16_t(code_point_encoded)) ==
                        detail::utf16_range::low)
                {
                    code_point_encoded =
                            detail::utf16_surrogate_pair_to_code_point(
                                uint16_t(surrogate_pair_high), uint16_t(code_point_encoded));
                    surrogate_pair_high = uint32_t(-1);
                }
                else if (uint32_t(-1) != surrogate_pair_high)
                    code = false;
                //  cases below this have (-1 == surrogate_pair_high)
                else if (uint32_t(-1) != code_point_encoded &&
                         detail::utf16_check(uint16_t(code_point_encoded)) ==
                             detail::utf16_range::high)
                {
                    surrogate_pair_high = code_point_encoded;
                    code_point_encoded = uint32_t(-1);
                }
                else if (uint32_t(-1) != code_point_encoded &&
                         detail::utf16_check(uint16_t(code_point_encoded)) ==
                             detail::utf16_range::low)
                    code = false;

                if (code &&
                    uint32_t(-1) != code_point_encoded)
                {
                    char buffer[4];
                    size_t utf8size = detail::utf32_to_utf8(code_point_encoded, buffer);
                    for (size_t index = 0; index < utf8size; ++index)
                        item += buffer[index];

                    code_point_encoded = uint32_t(-1);
                }

                if (code)
                {
                    assert(uint32_t(-1) == code_point_encoded);
                    assert((false == item.empty() && uint32_t(-1) == surrogate_pair_high) ||
                           (item.empty() && uint32_t(-1) != surrogate_pair_high));
                    utf8_value += item;
                }
            }
        }

        if (false == proper_ending ||
            uint32_t(-1) != code_point_encoded ||
            uint32_t(-1) != surrogate_pair_high ||
            0 != escape_sequence_remaining)
            code = false;

        if (false == code)
            utf8_value.clear();

        return code;
    }
};

class value_number :
        public beltpp::value_lexer_base<value_number, lexers>
{
    std::string value;
private:
    bool _check(std::string const& v) const
    {
        size_t pos = 0;
        beltpp::stod(v, pos);

        if (pos == v.length())
            return true;

        beltpp::stoi64(v, pos);

        if (pos == v.length())
            return true;

        beltpp::stoui64(v, pos);

        if (pos == v.length())
            return true;

        return false;
    }
public:
    beltpp::e_three_state_result check(char ch)
    {
        value += ch;
        if (_check(value) || _check(value + '0'))
            return beltpp::e_three_state_result::attempt;
        else
            return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return _check(std::string(it_begin, it_end));
    }
};

class value_bool : public beltpp::value_lexer_base<value_bool, lexers>
{
public:
    beltpp::e_three_state_result check(char ch)
    {
        if (ch >= 'a' && ch <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string value(it_begin, it_end);
        if (value == "true" || value == "false")
            return true;
        return false;
    }
};

class value_null : public beltpp::value_lexer_base<value_null, lexers>
{
public:
    beltpp::e_three_state_result check(char ch)
    {
        if (ch >= 'a' && ch <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "null";
    }
};

class discard :
        public beltpp::discard_lexer_base<discard,
                                            lexers,
                                            beltpp::standard_white_space_set<void>>
{};

using expression_tree = beltpp::expression_tree<lexers, std::string>;
using expression_tree_pointer = beltpp::expression_tree_pointer<lexers, std::string>;

template <typename T_iterator>
beltpp::e_three_state_result
parse_stream(expression_tree_pointer& ptr_expression,
             T_iterator& it_begin,
             T_iterator const& it_end,
             size_t const junk_limit,
             size_t const depth_limit,
             expression_tree* &proot)
{
    beltpp::e_three_state_result code = beltpp::e_three_state_result::attempt;
    beltpp::e_three_state_result parser_code = beltpp::e_three_state_result::attempt;

    auto const it_backup = it_begin;
    auto it_begin_keep = it_begin;
    while (true)
    {
        parser_code = beltpp::parse(ptr_expression, it_begin, it_end);
        if (beltpp::e_three_state_result::success != parser_code)
            break;

        if (it_begin == it_begin_keep)
            break;
        else
            it_begin_keep = it_begin;
    }
    //  parser can return attempt
    //  but it will not necessarily mean an error
    //  probably the stream will be filled later, and parser will succeed

    bool is_value = false;
    proot = beltpp::root(ptr_expression, is_value);

    if (it_backup == it_begin ||
        false == is_value)
    {   //  parser was not able to advance the iterator at all
        //  but it's not always able to tell if there is junk
        //  or part of correct buffer

        //  or iterator was advanced, but the content such as white space
        //  was discarded or something parsed but didn't form a full object,
        //  and further it was not possible
        //  to advance the iterator

        //  so let's check the remaining lenght and limit it

        auto temp = it_begin;
        size_t count = 0;
        while (temp != it_end)
        {   //  poor man's distance, will fix later
            ++count;
            ++temp;
        }

        if (count > junk_limit ||
            beltpp::e_three_state_result::error == parser_code)
        {
            it_begin = it_end;
            code = beltpp::e_three_state_result::error;
        }
        else
            code = beltpp::e_three_state_result::attempt;
    }
    else
    {   //  iterator was advanced as much as possible
        //  and we have a fully parsed object

        if (proot->depth() > depth_limit)
            code = beltpp::e_three_state_result::error;
        else
        {
            ptr_expression.stack.resize(1);

            code = beltpp::e_three_state_result::success;
        }
    }

    return code;
}

}// end of json
}
