#pragma once

#include "global.hpp"
#include "parser.hpp"

#include <memory>
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <cassert>

namespace beltpp
{
namespace json
{

template <typename T_types>
class operator_comma;
template <typename T_types>
class scope_brace;
template <typename T_types>
class scope_bracket;
template <typename T_types>
class operator_colon;
template <typename T_types>
class value_string;
template <typename T_types>
class value_number;
template <typename T_types>
class value_bool;
template <typename T_types>
class value_null;
template <typename T_types>
class discard;

template <typename T_types>
class lexers
{
public:
    using types = T_types;
    using list = beltpp::typelist::type_list<
        operator_comma<T_types>,
        scope_brace<T_types>,
        scope_bracket<T_types>,
        operator_colon<T_types>,
        value_string<T_types>,
        value_number<T_types>,
        value_bool<T_types>,
        value_null<T_types>,
        discard<T_types>
        >;
};

template <typename T_types>
class operator_comma :
        public beltpp::detail::operator_lexer_base<operator_comma<T_types>, lexers<T_types>>
{
public:
    enum { priority = 1 };

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end,
               bool& place_as_operator,
               typename T_types::T_size_type& right,
               typename T_types::T_property_type& property)
    {
        if (*it_begin != ',')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class operator_colon :
        public beltpp::detail::operator_lexer_base<operator_colon<T_types>, lexers<T_types>>
{
public:
    enum { priority = 2 };
    
    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end,
               bool& place_as_operator,
               typename T_types::T_size_type& right,
               typename T_types::T_property_type& property)
    {
        if (*it_begin != ':')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 0;
        return true;
    }
};

template <typename T_types>
class scope_brace : public beltpp::detail::operator_lexer_base<scope_brace<T_types>, lexers<T_types>>
{
public:
    enum { priority = 3 };
    
    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end,
               bool& place_as_operator,
               typename T_types::T_size_type& right,
               typename T_types::T_property_type& property)
    {
        if (*it_begin != '{' && *it_begin != '}')
            return false;

        if (*it_begin == '{')
        {
            place_as_operator = false;
            right = 1;
            property = 4;
        }
        else
        {
            place_as_operator = true;
            right = 0;
            property = 8;
        }

        ++it_begin;

        return true;
    }
};

template <typename T_types>
class scope_bracket : public beltpp::detail::operator_lexer_base<scope_bracket<T_types>, lexers<T_types>>
{
public:
    enum { priority = 4 };

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end,
               bool& place_as_operator,
               typename T_types::T_size_type& right,
               typename T_types::T_property_type& property)
    {
        if (*it_begin != '[' && *it_begin != ']')
            return false;

        if (*it_begin == '[')
        {
            place_as_operator = false;
            right = 1;
            property = 4;
        }
        else
        {
            place_as_operator = true;
            right = 0;
            property = 8;
        }

        ++it_begin;

        return true;
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

template <typename T_types>
class value_string :
        public beltpp::detail::value_lexer_base<value_string<T_types>, lexers<T_types>>
{
    uint8_t escape_sequence_index = 0;
    uint8_t escape_sequence_remaining = 0;
    bool start = true;
    bool final_check_status = false;
public:

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        if (*it_begin != '\"')
            return false;

        auto it_copy = it_begin;
        ++it_copy;

        while (it_copy != it_end)
        {
            if ('\"' == *it_copy)
            {
                ++it_copy;
                it_begin = it_copy;
                return true;
            }

            if ('\x20' > static_cast<unsigned char>(*it_copy))
                return false;

            if ('\\' != *it_copy)
                ++it_copy;
            else
            {
                ++it_copy;
                if (it_copy != it_end)
                {
                    if ('\"' == *it_copy || '\\' == *it_copy ||
                        '/' == *it_copy || 'b' == *it_copy ||
                        'f' == *it_copy || 'n' == *it_copy ||
                        'r' == *it_copy || 't' == *it_copy)
                        ++it_copy;
                    else if ('u' == *it_copy)
                    {
                        ++it_copy;

                        for (size_t index = 0; index != 4 && it_copy != it_end; ++index, ++it_copy)
                        {
                            if ((*it_copy < '0' || '9' < *it_copy) &&
                                (*it_copy < 'a' || 'f' < *it_copy) &&
                                (*it_copy < 'A' || 'F' < *it_copy))
                                return false;
                        }
                    }
                    else
                        return false;
                }
            }
        }

        return true;
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
        uint8_t escape_sequence_index = 0;
        uint8_t escape_sequence_remaining = 0;

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

template <typename T_types>
class value_number :
        public beltpp::detail::value_lexer_base<value_number<T_types>, lexers<T_types>>
{
public:
    template <bool stream_mode = true, typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        return beltpp::check_number(it_begin, it_end);
    }
};

template <typename T_types>
class value_bool : public beltpp::detail::value_lexer_base<value_bool<T_types>, lexers<T_types>>
{
public:

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        return beltpp::check(it_begin, it_end, "true") || beltpp::check(it_begin, it_end, "false");
    }
};

template <typename T_types>
class value_null : public beltpp::detail::value_lexer_base<value_null<T_types>, lexers<T_types>>
{
public:

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        return beltpp::check(it_begin, it_end, "null");
    }
};

//template <typename T_types>
//class discard :
//        public beltpp::discard_lexer_base<discard<T_types>,
//                                            lexers<T_types>,
//                                            beltpp::standard_white_space_set<void>>
//{};
template <typename T_types>
class discard :
        public beltpp::detail::discard_lexer_base_flexible<discard<T_types>,
                                                           lexers<T_types>>
{
public:
    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        bool code = false;
        while (it_begin != it_end &&
               (' ' == *it_begin || '\t' == *it_begin ||
                '\r' == *it_begin || '\n' == *it_begin))
        {
            ++it_begin;
            code = true;
        }

        return code;
    }
};

class json_parser_types
{
public:
    using T_rtt_type = uint8_t;
    using T_priority_type = uint8_t;
    using T_size_type = uint32_t;
    using T_property_type = uint8_t;
};

using expression_tree = beltpp::expression_tree<lexers<json_parser_types>>;

template <typename T_iterator>
beltpp::e_three_state_result
parse_stream(expression_tree& expression,
             T_iterator& it_begin,
             T_iterator const& it_end,
             size_t const not_yet_parsed_size_limit,
             size_t const depth_limit)
{
    assert(it_end != it_begin);
    
    bool advanced = false;

    T_iterator it_begin_track;
    while (true)
    {
        it_begin_track = it_begin;
        if (false == beltpp::parse(expression,
                                   it_begin,
                                   it_end,
                                   not_yet_parsed_size_limit,
                                   depth_limit,
                                   size_t(-1)))
        {
            it_begin = it_end;
            return beltpp::e_three_state_result::error;
        }
        
        if (it_begin_track == it_begin)
            break;

        advanced = true;
    }

    if (false == advanced || false == expression.complete())
        //  parser was not able to advance the iterator at all or
        //  the iterator was advanced, but didn't form a full object
        return beltpp::e_three_state_result::attempt;
    
    //  iterator was advanced as much as possible
    //  and we have a fully parsed object
    expression.node_address.resize(1);

    return beltpp::e_three_state_result::success;
    
    // beltpp::e_three_state_result code = beltpp::e_three_state_result::attempt;
    // beltpp::e_three_state_result parser_code = beltpp::e_three_state_result::attempt;

    // auto const it_backup = it_begin;
    // auto it_begin_keep = it_begin;
    // while (true)
    // {
    //     parser_code = beltpp::parse(expression,
    //                                 it_begin,
    //                                 it_end,
    //                                 not_yet_parsed_size_limit,
    //                                 depth_limit,
    //                                 size_t(-1));
    //     if (beltpp::e_three_state_result::success != parser_code)
    //         break;

    //     if (it_begin == it_begin_keep)
    //         break;
    //     else
    //         it_begin_keep = it_begin;
    // }
    // //  parser can return attempt
    // //  but it will not necessarily mean an error
    // //  probably the stream will be filled later, and parser will succeed

    // if (it_backup == it_begin ||
    //     false == expression.complete())
    // {   //  parser was not able to advance the iterator at all
    //     //  but it's not always able to tell if there is junk
    //     //  or part of correct buffer

    //     //  alternatively the iterator was advanced, but the content such as white space
    //     //  was discarded or something parsed but didn't form a full object,
    //     //  and further it was not possible
    //     //  to advance the iterator

    //     //  so let's check the remaining length and limit it

    //     auto temp = it_begin;
    //     size_t count = 0;
    //     while (temp != it_end)
    //     {   //  poor man's distance, will fix later
    //         ++count;
    //         ++temp;
    //     }

    //     if (count > junk_limit ||
    //         beltpp::e_three_state_result::error == parser_code)
    //     {
    //         it_begin = it_end;
    //         code = beltpp::e_three_state_result::error;
    //     }
    //     else
    //         code = beltpp::e_three_state_result::attempt;
    // }
    // else
    // {   //  iterator was advanced as much as possible
    //     //  and we have a fully parsed object
    //     expression.node_address.resize(1);

    //     if (expression.depth() > depth_limit)
    //         code = beltpp::e_three_state_result::error;
    //     else
    //         code = beltpp::e_three_state_result::success;
    // }

    // return code;
}

}// end of json
}
