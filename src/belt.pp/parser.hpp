#pragma once

#include "global.hpp"
#include "meta.hpp"
#include "queue.hpp"

#include <utility>
#include <memory>
#include <vector>
#include <type_traits>
#include <string>

namespace beltpp
{
namespace detail
{
template <typename T_string>
class token
{
public:
    size_t rtt = -1;
    size_t priority = 0;
    size_t right = -1;
    size_t left_min = -1;
    size_t left_max = -1;
    T_string value;
};
}

template <typename T_lexers, typename T_string>
class expression_tree
{
public:
    using lexers = T_lexers;
    using string = T_string;
    using ctoken = detail::token<T_string>;

    expression_tree() = default;
    expression_tree(expression_tree&) = delete;
    expression_tree(expression_tree&&) = delete;
    ~expression_tree() noexcept;

    size_t depth() const;
    bool is_value() const noexcept;
    bool is_operator() const noexcept;
    expression_tree& add_child(ctoken const& child_lexem);
    expression_tree& add_child(std::unique_ptr<expression_tree>&& ptree);

    expression_tree* parent = nullptr;
    ctoken lexem;
    std::vector<expression_tree*> children;
};

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>::~expression_tree() noexcept
{
    auto pparent = this;
    while (pparent->parent)
        pparent = pparent->parent;

    queue<expression_tree<T_lexers, T_string>*> items;

    items.push(pparent);

    while (false == items.empty())
    {
        auto pitem = items.front();
        for (auto& pchild : pitem->children)
        {
            pchild->parent = nullptr;
            items.push(pchild);
            pchild = nullptr;
        }
        pitem->children.clear();
        items.pop();
        if (pitem != this)
            delete pitem;
    }
}

template <typename T_lexers, typename T_string
          >
size_t expression_tree<T_lexers, T_string>::depth() const
{
    size_t depth = -1;
    queue<expression_tree<T_lexers, T_string> const*> items;

    items.push(this);

    while (false == items.empty())
    {
        auto begin_index = items.begin_index();
        auto end_index = items.end_index();
        for (auto index = begin_index; index != end_index; ++index)
        {
            auto pitem = items[index];
            for (auto& pchild : pitem->children)
                items.push(pchild);
            items.pop();
        }
        ++depth;
    }

    return depth;
}

template <typename T_lexers, typename T_string
          >
bool expression_tree<T_lexers, T_string>::is_value() const noexcept
{
    return lexem.right == 0;
}

template <typename T_lexers, typename T_string>
bool expression_tree<T_lexers, T_string>::is_operator() const noexcept
{
    return lexem.right > 0;
}

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>&
expression_tree<T_lexers,T_string>::add_child(ctoken const& child_lexem)
{
    std::unique_ptr<expression_tree> ptree(new expression_tree);
    ptree->lexem = child_lexem;

    return add_child(std::move(ptree));
}

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>&
expression_tree<T_lexers, T_string>::add_child(
        std::unique_ptr<expression_tree>&& ptree)
{
    if (ptree->parent)
    {
        auto iter = ptree->parent->children.begin();
        for (; iter != ptree->parent->children.end(); ++iter)
        {
            if (*iter == ptree.get())
            {
                ptree->parent->children.erase(iter);
                break;
            }
        }
    }
    ptree->parent = this;
    children.push_back(ptree.get());
    ptree.release();

    return *children.back();
}

namespace detail
{
template <typename T_lexers, typename T_string, typename T_iterator, typename INDEX>
class dummy
{
public:
static
bool read(T_iterator& it_begin,
          T_iterator it_end,
          detail::token<T_string>& result)
    {
        auto it_copy = it_begin;
        auto const discard_result = detail::token<T_string>();
        if (INDEX::value >= T_lexers::count)
            return false;
        else
        {
            using T_lexer =
                typename typelist::type_list_get<INDEX::value, T_lexers>::type;
            auto current_result = discard_result;
            bool current_code =
                    T_lexer::template internal_read<T_string, T_iterator>
                                            (it_copy,
                                             it_end,
                                             current_result);

            if (INDEX::value + 1 == T_lexers::count ||
                (current_code && it_copy != it_begin))
            {
                if (current_code && it_copy != it_begin)
                    it_begin = it_copy;

                result = current_result;
                return current_code;
            }
            else    // if (INDEX::value + 1 < T_list::count)
            {
                it_copy = it_begin;
                using cdummy = dummy<T_lexers, T_string, T_iterator,
                std::integral_constant<size_t, INDEX::value + 1>>;

                auto next_result = discard_result;
                bool next_code = cdummy::read(it_copy,
                                              it_end,
                                              next_result);

                if (current_code == false &&
                    next_code == false)
                    return false;
                else if (next_code && it_copy != it_begin)
                {
                    it_begin = it_copy;
                    result = next_result;
                    return next_code;
                }
                else
                    return true;
            }
        }
    }
};

template <typename T_lexers, typename T_string, typename T_iterator>
class dummy<T_lexers, T_string, T_iterator,
        std::integral_constant<size_t, T_lexers::count>>
{
public:
    static
    bool read(T_iterator& it_begin,
              T_iterator it_end,
              detail::token<T_string>& result)
    {
        return false;
    }
};
}

template <typename T_iterator,
          typename T_expression_tree>
bool parse(std::unique_ptr<T_expression_tree>& ptr_expression,
           T_iterator& it_begin,
           T_iterator it_end)
{
    auto read_result = typename T_expression_tree::ctoken();

    using cdummy =
        detail::dummy<
            typename T_expression_tree::lexers,
            typename T_expression_tree::string,
            T_iterator,
            std::integral_constant<size_t, 0>>;

    auto it_copy = it_begin;

    bool read_op_code = cdummy::read(it_copy, it_end, read_result);
    if (read_op_code && it_copy != it_begin)
    {   //  try to place the operator token in the expression tree
        //  if successful then update it_begin and return
        bool success = false;
        enum token_type {token_type_value, token_type_operator};
        std::vector<token_type> types;
        if (read_result.right == size_t(-1) &&
            read_result.left_max == size_t(-1) &&
            read_result.left_min == size_t(-1))
            success = true; //  discard the result
        else
        {
            if (read_result.left_max > 0)
                types.push_back(token_type_operator);
            if (read_result.left_min == 0)
                types.push_back(token_type_value);
        }

        for (auto ttype : types)
        {
        if (success)
            break;
        if (token_type_operator == ttype &&
            ptr_expression)
        {
            T_expression_tree* pparent = ptr_expression.get();
            T_expression_tree* test_pparent = pparent;
            while (pparent &&
                   pparent->is_value() &&
                   pparent->lexem.priority >= read_result.priority)
            {
                test_pparent = pparent;
                pparent = pparent->parent;
            }
            assert(test_pparent);
            pparent = test_pparent;
            //  now pparent is either the same as ptr_expression
            //  or is the weakest priority binary operator, that is still stronger
            //  than read_result but it cannot go as weak as the enclosing operator
            //  that still needs one or more arguments and is different than read_result

            //  will try to scan up to see if there is an
            //  enclosing operator that needs one or more arguments and
            //  is actually the same as read_result
            while (true)
            {
                if (test_pparent->is_operator() &&
                    test_pparent->lexem.rtt == read_result.rtt)
                {
                    pparent = test_pparent;
                    break;
                }
                if (nullptr == test_pparent->parent ||
                    false == test_pparent->is_value())
                    break;
                test_pparent = test_pparent->parent;
            }

            bool fix_the_same_possible = false;
            if (pparent->lexem.rtt == read_result.rtt)
                fix_the_same_possible = true;

            size_t left_count = 1;
            if (fix_the_same_possible)
                left_count = pparent->children.size();

            if (left_count >= read_result.left_min &&
                left_count <= read_result.left_max)
            {
                read_result.left_min = 0;
                read_result.left_max -= left_count;

                if (ptr_expression.get() != pparent)
                {
                    ptr_expression.release();
                    ptr_expression.reset(pparent);
                }

                if (fix_the_same_possible)
                {
                    auto value_backup = ptr_expression->lexem.value;
                    ptr_expression->lexem = read_result;
                    ptr_expression->lexem.value =
                            value_backup + ptr_expression->lexem.value;
                }
                else
                {
                    auto ptr_backup = std::move(ptr_expression);
                    auto pparent_backup = ptr_backup->parent;

                    ptr_expression.reset(new T_expression_tree);
                    ptr_expression->lexem = read_result;
                    ptr_expression->add_child(std::move(ptr_backup));
                    if (pparent_backup)
                    {
                        auto pexpression =
                                &pparent_backup->add_child(
                                    std::move(ptr_expression));
                        ptr_expression.reset(pexpression);
                    }
                }

                success = true;
            }
        }
        else if (token_type_value == ttype)
        {
            T_expression_tree* pparent = ptr_expression.get();
            while (pparent &&
                   false == pparent->is_operator() &&
                   pparent->is_value())
                pparent = pparent->parent;

            if (nullptr == ptr_expression)
            {
                ptr_expression.reset(new T_expression_tree);
                ptr_expression->lexem = read_result;
                success = true;
            }
            else if (pparent && false == pparent->is_operator())
            {
                assert(false);
            }
            else if (pparent)
            {
                T_expression_tree* ptemp = &pparent->add_child(read_result);

                ptr_expression.release();
                ptr_expression.reset(ptemp);
                --pparent->lexem.right;
                success = true;
            }
        }
        }

        if (success)
        {
            it_begin = it_copy;
            return true;
        }
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_op_code to false
        it_copy = it_begin;
        read_op_code = false;
    }

    if (read_op_code == false)
        return false;

    return true;
}

namespace detail
{
template <typename T_lexer,
          typename T_string,
          typename T_iterator>
class final_check_helper
{
    DECLARE_MF_INSPECTION(final_check, TT,
                          bool (TT::*)(T_iterator const&,
                                       T_iterator const&) const)
public:
    template <typename T,
              typename TEST = typename std::enable_if<
                  has_final_check<T>::value == 1, bool
                  >::type>
    static bool check(T const* p,
                      T_iterator const* it_begin,
                      T_iterator const* it_end)
    {
        return p->final_check(*it_begin, *it_end);
    }
    template <typename T = T_lexer,
              typename TEST = typename std::enable_if<
                  has_final_check<T>::value == 0, char
                  >::type>
    static bool check(...)
    {
        return true;
    }
};
template <typename T_lexer>
class scan_beyond_helper
{
    DECLARE_MF_INSPECTION(scan_beyond, TT,
                          bool (TT::*)() const)
public:
    template <typename T,
              typename TEST = typename std::enable_if<
                  has_scan_beyond<T>::value == 1, bool
                  >::type>
    static bool check(T const* p)
    {
        return p->scan_beyond();
    }
    template <typename T = T_lexer,
              typename TEST = typename std::enable_if<
                  has_scan_beyond<T>::value == 0, char
                  >::type>
    static bool check(...)
    {
        return true;
    }
};
}

template <typename T_lexer,
          typename T_string,
          typename T_iterator>
bool lexer_helper(T_iterator& it_begin,
                  T_iterator it_end,
                  T_lexer* p = nullptr)
{
    bool result = true;

    size_t index = 0;
    auto it_copy = it_begin;
    auto const it_backup = it_begin;
    T_lexer lexer;
    for (; it_copy != it_end; ++it_copy, ++index)
    {
        std::pair<bool, bool> lexer_res = lexer.check(*it_copy);
        if (false == lexer_res.first)
        {   //  will stop, next symbol is not accepted anymore
            if (index == 0)
            {   //  empty, not accepted anything
                result = false;
                break;
            }
            else
            {   //  had something accepted previously
                //  stop and do final check
                it_begin = it_copy;
                break;
            }
        }
        else if (true == lexer_res.second)
        {   //  next symbol is accepted and suggested to be final
            ++it_copy;
            it_begin = it_copy;
            break;
        }
    }

    if (result &&   //  something accepted, but not sure if its final
        it_begin == it_backup &&    //  so it_begin was not altered yet
        false == detail::scan_beyond_helper<T_lexer>::check(&lexer))
        it_begin = it_end;  //  now alter it_begin too, this will not happen by default
    //  insted will have to scan past the last token to accept it

    if (result && it_begin != it_backup)
    {
        if (false ==
                detail::final_check_helper<T_lexer, T_string, T_iterator>
                    ::check(&lexer,
                            &it_backup,
                            &it_begin))
        {
            result = false;
            it_begin = it_backup;
        }
    }

    if (p)
        *p = lexer;
    return result;
}

template <typename T_discard_lexer, typename T_lexers, typename T_white_space_set>
class discard_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_discard_lexer,
                        T_lexers>::value};

    template <typename T_string,
              typename T_iterator>
    static
    bool internal_read(T_iterator& it_begin,
                       T_iterator it_end,
                       detail::token<T_string>& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>();
        bool code = lexer_helper<T_discard_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            result.rtt = T_discard_lexer::rtt;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;
        }

        return code;
    }

    std::pair<bool, bool> check(char ch)
    {
        auto value = T_white_space_set::value;
        if (value.find(ch) != std::string::npos)
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }
};

template <typename T_operator_lexer, typename T_lexers>
class operator_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_operator_lexer,
                        T_lexers>::value};

    static size_t const priority;

    template <typename T_string,
              typename T_iterator>
    static
    bool internal_read(T_iterator& it_begin,
                       T_iterator it_end,
                       detail::token<T_string>& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>();
        T_operator_lexer lexer;
        bool code = lexer_helper<T_operator_lexer, T_string>(it_copy, it_end, &lexer);

        if (code && it_copy != it_begin)
        {
            result.priority = T_operator_lexer::priority;
            result.rtt = T_operator_lexer::rtt;
            result.value.assign(it_begin, it_copy);
            result.right = lexer.right;
            result.left_min = lexer.left_min;
            result.left_max = lexer.left_max;
            it_begin = it_copy;
        }

        return code;
    }
private:
    enum { previous_rtt = (rtt == 0) ? 0 : rtt - 1 };
};

template <typename T_operator_lexer,
          typename T_lexers>
size_t const
operator_lexer_base<T_operator_lexer, T_lexers>::priority =
        T_operator_lexer::grow_priority +
        ((T_operator_lexer::rtt == 0) ? 0 :
         (typelist::type_list_get<T_operator_lexer::previous_rtt, T_lexers>::type::priority));

template <typename T_operator_set>
std::pair<bool, bool> standard_operator_check(char ch)
{
    if (T_operator_set::value.find(ch) != std::string::npos)
        return std::make_pair(true, false);
    return std::make_pair(false, false);
}

template <typename T_value_lexer, typename T_lexers>
class value_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_value_lexer,
                        T_lexers>::value};

    template <typename T_string,
              typename T_iterator>
    static
    bool internal_read(T_iterator& it_begin,
                       T_iterator it_end,
                       detail::token<T_string>& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>();
        bool code = lexer_helper<T_value_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            result.priority = size_t(-1);
            result.rtt = T_value_lexer::rtt;
            result.right = 0;
            result.left_min = 0;
            result.left_max = 0;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;
        }

        return code;
    }
};

//
//  if not make these template leads to severe segfault on app exit
//
template <typename N = void>
struct standard_white_space_set { public: static const std::string value; };
template <typename N>
std::string const standard_white_space_set<N>::value = " \t\r\n";
template <typename N = void>
struct standard_operator_set { public: static const std::string value; };
template <typename N>
std::string const standard_operator_set<N>::value = "+-*&/|\\=!$@^%,;:<>";

template <typename T_lexers>
class standard_white_space_lexer :
        public beltpp::discard_lexer_base<standard_white_space_lexer<T_lexers>,
                                            T_lexers,
                                            beltpp::standard_white_space_set<void>>
{};

template <typename T_lexers>
class standard_value_string_lexer :
        public beltpp::value_lexer_base<standard_value_string_lexer<T_lexers>, T_lexers>
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

template <typename T_lexers>
class standard_value_number_lexer :
        public beltpp::value_lexer_base<standard_value_number_lexer<T_lexers>,
                                        T_lexers>
{
    std::string value;
private:
    bool _check(std::string const& v) const
    {
        size_t pos = 0;
        beltpp::stod(v, pos);
        if (pos != v.length())
            beltpp::stoll(v, pos);

        if (pos != v.length())
            return false;
        return true;
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

    bool final_check(std::string::iterator it_begin,
                     std::string::iterator it_end) const
    {
        return _check(std::string(it_begin, it_end));
    }

    bool scan_beyond() const
    {
        return false;
    }
};

template <typename T_lexers>
class standard_operator_comma_lexer :
        public beltpp::operator_lexer_base<standard_operator_comma_lexer<T_lexers>,
                                            T_lexers>
{
public:
    size_t right = 1;
    size_t left_max = -1;
    size_t left_min = 1;

    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return standard_operator_check<beltpp::standard_operator_set<void>>(ch);
    }

    bool final_check(std::string::iterator it_begin,
                     std::string::iterator it_end) const
    {
        return std::string(it_begin, it_end) == ",";
    }
};

template <typename T_expression_tree>
typename T_expression_tree::string dump(T_expression_tree const* pexpression)
{
    typename T_expression_tree::string result;

    if (nullptr == pexpression)
        return result;

    auto p_iterator = pexpression;
    std::vector<size_t> track;
    size_t depth = 0;

    while (p_iterator)
    {
        if (track.size() == depth)
            track.push_back(0);

        if (track.size() == depth + 1)
        {
            for (size_t index = 0; index != depth; ++index)
                result += ".";
            result += p_iterator->lexem.value;
            if (p_iterator->lexem.right > 0)
                result += " !!!! ";
            result += "\n";
        }

        size_t next_child_index = -1;
        if (false == p_iterator->children.empty())
        {
            size_t childindex = 0;
            if (track.size() > depth + 1)
            {
                ++track[depth + 1];
                childindex = track[depth + 1];
            }

            if (childindex < p_iterator->children.size())
                next_child_index = childindex;
        }

        if (size_t(-1) != next_child_index)
        {
            p_iterator = p_iterator->children[next_child_index];
            ++depth;
            if (track.size() > depth + 1)
                track.resize(depth + 1);
        }
        else
        {
            p_iterator = p_iterator->parent;
            --depth;
        }
    }

    return result;
}

template <typename T_expression_tree>
T_expression_tree* root(T_expression_tree* pexpression, bool& is_value)
{
    bool missing_expression = false;
    if (nullptr == pexpression)
        missing_expression = true;
    else if (false == pexpression->is_value())
        missing_expression = true;

    if (pexpression)
    while (auto parent = pexpression->parent)
    {
        if (false == parent->is_value())
            missing_expression = true;
        pexpression = parent;
    }

    is_value = !missing_expression;
    return pexpression;
}

}   //  end of namespace beltpp
