#pragma once

#include "global.hpp"
#include "parser.hpp"

#include "iterator_wrapper.hpp"

#include <memory>

namespace beltpp
{
namespace json
{
using lexers = beltpp::typelist::type_list<
class operator_comma,
class scope_brace,
class operator_colon,
class value_string,
class value_number,
class discard
>;

class operator_set { public: static const std::string value; };
std::string const operator_set::value = ",:";

class operator_comma :
        public beltpp::operator_lexer_base<operator_comma, lexers>
{
public:
    size_t right = 1;
    size_t left_min = 1;
    size_t left_max = -1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return beltpp::standard_operator_check<operator_set>(ch);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == ",";
    }
};

class operator_colon :
        public beltpp::operator_lexer_base<operator_colon, lexers>
{
public:
    size_t right = 1;
    size_t left_min = 1;
    size_t left_max = 1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return beltpp::standard_operator_check<operator_set>(ch);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == ":";
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
            left_max = 1;
            return std::make_pair(true, true);
        }
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        std::string value(it_begin, it_end);
        if (value == "{" || value == "}")
            return true;
        return false;
    }
};

class value_string :
        public beltpp::value_lexer_base<value_string, lexers>
{
    enum states
    {
        state_none = 0x0,
        state_single_quote_open = 0x01,
        state_double_quote_open = 0x02,
        state_escape_symbol = 0x04
    };
    int state = state_none;
    size_t index = -1;
public:
    std::pair<bool, bool> check(char ch)
    {
        ++index;
        if (0 == index && ch == '\'')
        {
            state |= state_single_quote_open;
            return std::make_pair(true, false);
        }
        if (0 == index && ch == '\"')
        {
            state |= state_double_quote_open;
            return std::make_pair(true, false);
        }
        if (0 == index && ch != '\'' && ch != '\"')
            return std::make_pair(false, false);
        if (0 != index && ch == '\'' &&
            (state & state_single_quote_open) &&
            !(state & state_escape_symbol))
            return std::make_pair(true, true);
        if (0 != index && ch == '\"' &&
            (state & state_double_quote_open) &&
            !(state & state_escape_symbol))
            return std::make_pair(true, true);
        if (0 != index && ch == '\\' &&
            !(state & state_escape_symbol))
        {
            state |= state_escape_symbol;
            return std::make_pair(true, false);
        }
        state &= ~state_escape_symbol;
        return std::make_pair(true, false);
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

        beltpp::stoll(v, pos);

        if (pos == v.length())
            return true;

        beltpp::stoull(v, pos);

        if (pos == v.length())
            return true;

        return false;
    }
public:
    std::pair<bool, bool> check(char ch)
    {
        value += ch;
        if ("." == value || "-" == value || "-." == value)
            return std::make_pair(true, false);
        else if (_check(value))
            return std::make_pair(true, false);
        else
            return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return _check(std::string(it_begin, it_end));
    }
};

class discard :
        public beltpp::discard_lexer_base<discard,
                                            lexers,
                                            beltpp::standard_white_space_set<void>>
{};

using expression_tree = beltpp::expression_tree<lexers, std::string>;
using ptr_expression_tree = std::unique_ptr<expression_tree>;

template <typename T_iterator>
beltpp::e_three_state_result
parse_stream(ptr_expression_tree& ptr_expression,
             T_iterator& it_begin,
             T_iterator it_end,
             size_t const junk_limit,
             expression_tree* &proot)
{
    beltpp::e_three_state_result code = beltpp::e_three_state_result::attempt;

    auto const it_backup = it_begin;
    auto it_begin_keep = it_begin;
    while (beltpp::parse(ptr_expression, it_begin, it_end))
    {
        if (it_begin == it_begin_keep)
            break;
        else
            it_begin_keep = it_begin;
    }
    //  parser can give false or not advance the iterator
    //  but it will not necessarily mean an error
    //  probably the stream will be filled later, and parser will succeed

    bool is_value = false;
    proot = beltpp::root(ptr_expression.get(), is_value);

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

        if (count > junk_limit)
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

        ptr_expression.release();
        ptr_expression.reset(proot);

        code = beltpp::e_three_state_result::success;
    }

    return code;
}

}// end of json
}
