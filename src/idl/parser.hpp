#pragma once

#include <belt.pp/parser.hpp>
#include <belt.pp/meta.hpp>

#include <string>

using lexers = beltpp::typelist::type_list<
//class operator_semicolon,
class keyword_module,
class keyword_type,
class keyword_class,
class keyword_array,
class keyword_hash,
class scope_brace,
class identifier,
class discard
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
class keyword_module : public beltpp::operator_lexer_base<keyword_module, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
    enum { grow_priority = 1 };

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

class keyword_type : public beltpp::operator_lexer_base<keyword_type, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
    enum { grow_priority = 1 };

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
        return std::string(it_begin, it_end) == "type";
    }
};

class keyword_class : public beltpp::operator_lexer_base<keyword_class, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
    enum { grow_priority = 1 };

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

class keyword_array : public beltpp::operator_lexer_base<keyword_array, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
    enum { grow_priority = 1 };

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

class keyword_hash : public beltpp::operator_lexer_base<keyword_hash, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 1;
    enum { grow_priority = 1 };

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

class scope_brace : public beltpp::operator_lexer_base<scope_brace, lexers>
{
public:
    size_t right = -1;
    size_t left_max = 0;
    size_t left_min = 0;
    size_t property = 8;
    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
    {
        if (ch == '{')
        {
            right = -1;
            left_min = 0;
            left_max = 0;
            property = 8;
            return beltpp::e_three_state_result::success;
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = -1;
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

class identifier : public beltpp::value_lexer_base<identifier, lexers>
{
    size_t index = -1;
public:
    beltpp::e_three_state_result check(char ch)
    {
        ++index;
        if ((tolower(ch) >= 'a' && tolower(ch) <= 'z') ||
            ch == '_' ||
            (ch >= '0' && ch <= '9' && index > 0))
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
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
