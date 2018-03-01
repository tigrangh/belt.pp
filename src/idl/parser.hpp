#pragma once

#include <belt.pp/parser.hpp>
#include <belt.pp/meta.hpp>

#include <string>

using lexers = beltpp::typelist::type_list<
class keyword_namespace,
class keyword_class,
class scope_brace,
class operator_colon,
class identifier,
class discard
>;

class keyword_namespace : public beltpp::operator_lexer_base<keyword_namespace, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch >= 'a' && ch <= 'z')
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "namespace";
    }
};

class keyword_class : public beltpp::operator_lexer_base<keyword_class, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch >= 'a' && ch <= 'z')
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "class";
    }
};

class scope_brace : public beltpp::operator_lexer_base<scope_brace, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch == '{')
        {
            right = -1;
            left_min = 0;
            left_max = 0;
            return std::make_pair(true, true);
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = -1;
            return std::make_pair(true, true);
        }
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string temp(it_begin, it_end);
        return (temp == "{" || temp == "}");
    }
};

class operator_colon : public beltpp::operator_lexer_base<operator_colon, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 1;
    size_t left_min = 1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return beltpp::standard_operator_check<beltpp::standard_operator_set<void>>(ch);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == ":";
    }
};

class identifier : public beltpp::value_lexer_base<identifier, lexers>
{
    size_t index = -1;
public:
    std::pair<bool, bool> check(char ch)
    {
        ++index;
        if ((ch >= 'a' && ch <= 'z') ||
            ch == '_' ||
            (ch >= '0' && ch <= '9' && index > 0))
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return it_begin != it_end;
    }

    bool scan_beyond() const
    {
        return false;
    }
};
class discard :
        public beltpp::discard_lexer_base<discard,
                                            lexers,
                                            beltpp::standard_white_space_set<void>>
{
public:
    bool scan_beyond() const
    {
        return false;
    }
};
