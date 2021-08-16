#pragma once

#include "parser.hpp"

#include <string>

namespace beltpp
{
namespace idl_parser
{
template <typename T_types>
class keyword_module;
template <typename T_types>
class keyword_class;
template <typename T_types>
class keyword_enum;
template <typename T_types>
class keyword_array;
template <typename T_types>
class keyword_optional;
template <typename T_types>
class keyword_variant;
template <typename T_types>
class keyword_set;
template <typename T_types>
class keyword_hash;
template <typename T_types>
class scope_brace;
template <typename T_types>
class identifier;
template <typename T_types>
class discard;
template <typename T_types>
class discard_comment;

template <typename T_types>
using lexers = beltpp::typelist::type_list<
//class operator_semicolon,
keyword_module<T_types>,
keyword_class<T_types>,
keyword_enum<T_types>,
keyword_array<T_types>,
keyword_optional<T_types>,
keyword_variant<T_types>,
keyword_set<T_types>,
keyword_hash<T_types>,
scope_brace<T_types>,
identifier<T_types>,
discard<T_types>,
discard_comment<T_types>
>;

/*class operator_semicolon : public beltpp::operator_lexer_base<operator_semicolon, lexers>
{
public:
    size_t right = 1;
    size_t left_max = -1;
    size_t left_min = 1;
    size_t property = 1;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char)
    {
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const&,
                     T_iterator const&) const
    {
        return false;
    }

    bool get_default(beltpp::detail::token<std::string>& result) const
    {
        result.priority = operator_semicolon::priority;
        result.rtt = operator_semicolon::rtt;
        result.value = ";";
        result.right = right;
        result.left_min = left_min;
        result.left_max = left_max;
        result.property = property;

        return true;
    }
};*/
template <typename T_types>
class keyword_module : public beltpp::operator_lexer_base<keyword_module<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 2;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 1 };

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
        return std::string(it_begin, it_end) == "module";
    }
};

template <typename T_types>
class keyword_class : public beltpp::operator_lexer_base<keyword_class<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 2;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 2 };

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
        return std::string(it_begin, it_end) == "class";
    }
};

template <typename T_types>
class keyword_enum : public beltpp::operator_lexer_base<keyword_enum<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 2;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 3 };

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
        return std::string(it_begin, it_end) == "enum";
    }
};

template <typename T_types>
class keyword_array : public beltpp::operator_lexer_base<keyword_array<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 1;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 4 };

    beltpp::e_three_state_result check(char ch)
    {
        if (tolower(ch) >= 'a' && tolower(ch) <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "Array";
    }
};

template <typename T_types>
class keyword_optional : public beltpp::operator_lexer_base<keyword_optional<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 1;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 5 };

    beltpp::e_three_state_result check(char ch)
    {
        if (tolower(ch) >= 'a' && tolower(ch) <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "Optional";
    }
};

template <typename T_types>
class keyword_variant : public beltpp::operator_lexer_base<keyword_variant<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 2;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 6 };

    beltpp::e_three_state_result check(char ch)
    {
        if (tolower(ch) >= 'a' && tolower(ch) <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "Variant";
    }
};

template <typename T_types>
class keyword_set : public beltpp::operator_lexer_base<keyword_set<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 1;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 7 };

    beltpp::e_three_state_result check(char ch)
    {
        if (tolower(ch) >= 'a' && tolower(ch) <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "Set";
    }
};

template <typename T_types>
class keyword_hash : public beltpp::operator_lexer_base<keyword_hash<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = 2;
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 1;
    enum { priority = 8 };

    beltpp::e_three_state_result check(char ch)
    {
        if (tolower(ch) >= 'a' && tolower(ch) <= 'z')
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "Hash";
    }
};

template <typename T_types>
class scope_brace : public beltpp::operator_lexer_base<scope_brace<T_types>, lexers<T_types>, T_types>
{
public:
    typename T_types::T_size_type right = typename T_types::T_size_type(-1);
    typename T_types::T_size_type left_max = 0;
    typename T_types::T_size_type left_min = 0;
    typename T_types::T_property_type property = 8;
    enum { priority = 9 };

    beltpp::e_three_state_result check(char ch)
    {
        if (ch == '{')
        {
            right = typename T_types::T_size_type(-1);
            left_min = 0;
            left_max = 0;
            property = 8;
            return beltpp::e_three_state_result::success;
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = typename T_types::T_size_type(-1);
            property = 4;
            return beltpp::e_three_state_result::success;
        }
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string temp(it_begin, it_end);
        return (temp == "{" || temp == "}");
    }
};

template <typename T_types>
class identifier : public beltpp::value_lexer_base<identifier<T_types>, lexers<T_types>>
{
    bool start = true;
public:
    enum { scan_beyond = 0 };
    beltpp::e_three_state_result check(char ch)
    {
        bool local_start = start;
        start = false;
        if ((tolower(ch) >= 'a' && tolower(ch) <= 'z') ||
            ch == '_' ||
            (ch >= '0' && ch <= '9' && false == local_start))
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return it_begin != it_end;
    }
};

template <typename T_types>
class discard :
        public beltpp::discard_lexer_base<discard<T_types>,
                                          lexers<T_types>,
                                          beltpp::standard_white_space_set<void>>
{};

template <typename T_types>
class discard_comment :
        public beltpp::discard_lexer_base_flexible<discard_comment<T_types>,
                                                   lexers<T_types>>
{
    uint8_t index = uint8_t(-1);
public:

    beltpp::e_three_state_result check(char ch)
    {
        if (index < 2)
            ++index;
        if ((0 == index || 1 == index) && '/' == ch)
            return beltpp::e_three_state_result::attempt;
        else if (1 < index && '\n' == ch)
            return beltpp::e_three_state_result::success;
        else if (1 < index)
            return beltpp::e_three_state_result::attempt;
        else
            return beltpp::e_three_state_result::error;
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string value(it_begin, it_end);
        if (value.length() >= 3 &&
            value[0] == '/' &&
            value[1] == '/' &&
            value.back() == '\n')
            return true;
        return false;
    }
};
} // end idl_parser
} // end beltpp
