#pragma once

#include <belt.pp/parser.hpp>
#include <belt.pp/meta.hpp>

#include <string>

using lexers = beltpp::typelist::type_list<
class operator_semicolon,
class keyword_package,
class keyword_type,
class keyword_struct,
class keyword_map,
class scope_brace,
class scope_bracket,
class identifier,
class discard
>;

class operator_semicolon : public beltpp::operator_lexer_base<operator_semicolon, lexers>
{
public:
    size_t right = 1;
    size_t left_max = -1;
    size_t left_min = 1;
    size_t property = 1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char)
    {
        return std::make_pair(false, false);
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
};
class keyword_package : public beltpp::operator_lexer_base<keyword_package, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
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
        return std::string(it_begin, it_end) == "package";
    }
};

class keyword_type : public beltpp::operator_lexer_base<keyword_type, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
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
        return std::string(it_begin, it_end) == "type";
    }
};

class keyword_struct : public beltpp::operator_lexer_base<keyword_struct, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
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
        return std::string(it_begin, it_end) == "struct";
    }
};

class keyword_map : public beltpp::operator_lexer_base<keyword_map, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
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
        return std::string(it_begin, it_end) == "map";
    }
};

class scope_brace : public beltpp::operator_lexer_base<scope_brace, lexers>
{
public:
    size_t right = -1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 8;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch == '{')
        {
            right = -1;
            left_min = 0;
            left_max = 0;
            property = 8;
            return std::make_pair(true, true);
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = -1;
            property = 4;
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

class scope_bracket :
        public beltpp::operator_lexer_base<scope_bracket, lexers>
{
public:
    size_t right = -1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 8;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch == '[')
        {
            right = -1;
            left_min = 0;
            left_max = 0;
            property = 8;
            return std::make_pair(true, true);
        }
        if (ch == ']')
        {
            right = 1;
            left_min = 0;
            left_max = 1;
            property = 4;
            return std::make_pair(true, true);
        }
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string temp(it_begin, it_end);
        return (temp == "[" || temp == "]");
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
