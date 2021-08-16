#pragma once

#include "parser.hpp"

#include <string>

namespace beltpp
{
namespace math_expression
{
template <typename T_types>
class binary_operator_plus;
template <typename T_types>
class binary_operator_minus;
template <typename T_types>
class binary_operator_multiply;
template <typename T_types>
class binary_operator_divide;
template <typename T_types>
class binary_operator_power;
template <typename T_types>
class bracket;
template <typename T_types>
class unary_operator_plus;
template <typename T_types>
class discard;
template <typename T_types>
class number;

template <typename T_types>
class lexers
{
public:
    using types = T_types;
    using list = beltpp::typelist::type_list<
        discard<T_types>,
        binary_operator_plus<T_types>,
        binary_operator_minus<T_types>,
        number<T_types>,
        binary_operator_multiply<T_types>,
        binary_operator_divide<T_types>,
        binary_operator_power<T_types>,
        bracket<T_types>,
        unary_operator_plus<T_types>
        >;
    
    enum { default_operator = beltpp::typelist::type_list_index<binary_operator_multiply<T_types>, list>::value };
};

template <typename T_types>
class binary_operator_plus : public beltpp::detail::operator_lexer_base<binary_operator_plus<T_types>, lexers<T_types>>
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
        if (*it_begin != '+')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class binary_operator_minus : public beltpp::detail::operator_lexer_base<binary_operator_minus<T_types>, lexers<T_types>>
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
        if (*it_begin != '-')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class binary_operator_multiply : public beltpp::detail::operator_lexer_base<binary_operator_multiply<T_types>, lexers<T_types>>
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
        if (*it_begin != '*')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }

    static inline
    void get_default(bool& place_as_operator,
                     typename T_types::T_size_type& right,
                     typename T_types::T_property_type& property)
    {
        place_as_operator = true;
        right = 1;
        property = 1;
    }
};

template <typename T_types>
class binary_operator_divide : public beltpp::detail::operator_lexer_base<binary_operator_divide<T_types>, lexers<T_types>>
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
        if (*it_begin != '/')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class binary_operator_power : public beltpp::detail::operator_lexer_base<binary_operator_power<T_types>, lexers<T_types>>
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
        if (*it_begin != '^')
            return false;

        ++it_begin;
        place_as_operator = true;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class unary_operator_plus : public beltpp::detail::operator_lexer_base<unary_operator_plus<T_types>, lexers<T_types>>
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
        if (*it_begin != '+')
            return false;

        ++it_begin;
        place_as_operator = false;
        right = 1;
        property = 1;
        return true;
    }
};

template <typename T_types>
class discard :
        public beltpp::detail::discard_lexer_base<discard<T_types>,
                                                  lexers<T_types>,
                                                  beltpp::detail::standard_white_space_set<void>>
{};

template <typename T_types>
class bracket : public beltpp::detail::operator_lexer_base<bracket<T_types>, lexers<T_types>>
{
public:
    enum { priority = 5 };
    
    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end,
               bool& place_as_operator,
               typename T_types::T_size_type& right,
               typename T_types::T_property_type& property)
    {
        if (*it_begin != '(' && *it_begin != ')')
            return false;

        if (*it_begin == '(')
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
class number : public beltpp::detail::value_lexer_base<number<T_types>, lexers<T_types>>
{
public:

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        char* endptr = nullptr;
        std::strtod(&*it_begin, &endptr);
        size_t length = size_t(endptr - &*it_begin);

        if (length == 0)
            return false;
        
        it_begin = it_begin + length;
        return true;
    }
};

}   // end math_expression
}   // end beltpp
