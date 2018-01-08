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
    enum class etype
    {
        bin_op,
        un_op,
        value,
        scope,
        discard
    };

    bool scope_closed = false;
    etype type = etype::discard;
    size_t rtt = -1;
    size_t priority = 0;
    T_string value;
};
}

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
class expression_tree
{
public:
    using list_operator_lexers = T_list_operator_lexers;
    using list_value_lexers = T_list_value_lexers;
    using list_scope_lexers = T_list_scope_lexers;
    using list_discard_lexers = T_list_discard_lexers;
    using string = T_string;
    using ctoken = detail::token<T_string>;

    expression_tree() = default;
    expression_tree(expression_tree&) = delete;
    expression_tree(expression_tree&&) = delete;
    ~expression_tree() noexcept;

    size_t depth() const;
    bool is_node() const noexcept;
    bool is_leaf() const noexcept;
    bool is_value() const noexcept;
    expression_tree& add_child(ctoken const& child_lexem);
    expression_tree& add_child(std::unique_ptr<expression_tree>&& ptree);

    expression_tree* parent = nullptr;
    ctoken lexem;
    std::vector<expression_tree*> children;
};

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::~expression_tree() noexcept
{
    auto pparent = this;
    while (pparent->parent)
        pparent = pparent->parent;

    queue<expression_tree<
            T_list_operator_lexers,
            T_list_value_lexers,
            T_list_scope_lexers,
            T_list_discard_lexers,
            T_string>*> items;

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

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
size_t expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::depth() const
{
    size_t depth = -1;
    queue<expression_tree<
            T_list_operator_lexers,
            T_list_value_lexers,
            T_list_scope_lexers,
            T_list_discard_lexers,
            T_string> const*> items;

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

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
bool expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::is_node() const noexcept
{
    if (false == children.empty())
        return true;
    return false;
}

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
bool expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::is_leaf() const noexcept
{
    if (children.empty())
        return true;
    return false;
}

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
bool expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::is_value() const noexcept
{
    return (lexem.type == ctoken::etype::value ||
            (   lexem.type == ctoken::etype::scope &&
                lexem.scope_closed));
}

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>& expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::add_child(ctoken const& child_lexem)
{
    std::unique_ptr<expression_tree> ptree(new expression_tree);
    ptree->lexem = child_lexem;

    return add_child(std::move(ptree));
}

template <typename T_list_operator_lexers,
          typename T_list_value_lexers,
          typename T_list_scope_lexers,
          typename T_list_discard_lexers,
          typename T_string
          >
expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>& expression_tree<
T_list_operator_lexers,
T_list_value_lexers,
T_list_scope_lexers,
T_list_discard_lexers,
T_string>::add_child(std::unique_ptr<expression_tree>&& ptree)
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
template <typename T_list_lexers,
          typename T_string,
          typename T_iterator,
          typename INDEX>
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
        if (INDEX::value >= T_list_lexers::count)
            return false;
        else
        {
            using T_lexer =
                typename typelist::type_list_get<INDEX::value, T_list_lexers>::type;
            auto current_result = discard_result;
            bool current_code =
                    T_lexer::template internal_read<T_string, T_iterator>
                                            (it_copy,
                                             it_end,
                                             current_result);

            if (INDEX::value + 1 == T_list_lexers::count ||
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
                using cdummy = dummy<T_list_lexers, T_string, T_iterator,
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

template <typename T_list_lexers,
          typename T_string,
          typename T_iterator>
class dummy<T_list_lexers,
        T_string,
        T_iterator,
        std::integral_constant<size_t, T_list_lexers::count>>
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

    using cdummy_discards =
        detail::dummy<
    typename T_expression_tree::list_discard_lexers,
    typename T_expression_tree::string,
    T_iterator,
    std::integral_constant<size_t, 0>>;

    using cdummy_op =
        detail::dummy<
    typename T_expression_tree::list_operator_lexers,
    typename T_expression_tree::string,
    T_iterator,
    std::integral_constant<size_t, 0>>;

    using cdummy_val =
        detail::dummy<
    typename T_expression_tree::list_value_lexers,
    typename T_expression_tree::string,
    T_iterator,
    std::integral_constant<size_t, 0>>;

    using cdummy_scope =
        detail::dummy<
    typename T_expression_tree::list_scope_lexers,
    typename T_expression_tree::string,
    T_iterator,
    std::integral_constant<size_t, 0>>;

    using ctoken = typename T_expression_tree::ctoken;
    using token_type = typename ctoken::etype;

    auto it_copy = it_begin;

    bool read_discard_code = cdummy_discards::read(it_copy, it_end, read_result);
    if (read_discard_code && it_copy != it_begin)
    {
        it_begin = it_copy;
        return true;
    }

    bool read_op_code = cdummy_op::read(it_copy, it_end, read_result);
    if (read_op_code && it_copy != it_begin)
    {   //  try to place the operator token in the expression tree
        //  if successful then update it_begin and return
        if (ptr_expression &&
            ptr_expression->is_value())
        {
            T_expression_tree* pparent = ptr_expression.get();
            T_expression_tree* test_pparent = nullptr;
            while ((test_pparent = pparent->parent) != nullptr &&
                   test_pparent->lexem.type == ctoken::etype::bin_op &&
                   test_pparent->lexem.priority >= read_result.priority)
            {
                pparent = test_pparent;
            }
            //  after this while pparent is either the same as ptr_expression
            //  or is the weakest priority binary operator, that is still stronger
            //  than read_result
            ptr_expression.release();
            ptr_expression.reset(pparent);

            if (ptr_expression->lexem.type == read_result.type &&
                ptr_expression->lexem.rtt == read_result.rtt &&
                ptr_expression->lexem.value == read_result.value)
            {
                //  just move to this place
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
                    auto pexpression = &pparent_backup->add_child(std::move(ptr_expression));
                    ptr_expression.reset(pexpression);
                }
            }

            it_begin = it_copy;
            return true;
        }
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_op_code to false
        it_copy = it_begin;
        read_op_code = false;
    }

    bool read_val_code = cdummy_val::read(it_copy, it_end, read_result);
    if (read_val_code && it_copy != it_begin)
    {   //  try to place the value token in the expression tree
        //  if successful then update it_begin and return
        bool success = false;
        if (nullptr == ptr_expression)
        {
            ptr_expression.reset(new T_expression_tree);
            ptr_expression->lexem = read_result;
            success = true;
        }
        else if (ptr_expression->lexem.type == token_type::bin_op ||
                 (  ptr_expression->lexem.type == token_type::scope &&
                    ptr_expression->lexem.scope_closed == false))
        {
            T_expression_tree* ptemp = &ptr_expression->add_child(read_result);

            ptr_expression.release();
            ptr_expression.reset(ptemp);
            success = true;
        }

        if (success)
        {
            it_begin = it_copy;
            return true;
        }
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_val_code to false
        it_copy = it_begin;
        read_val_code = false;
    }

    bool read_scope_code = cdummy_scope::read(it_copy, it_end, read_result);
    if (read_scope_code && it_copy != it_begin)
    {   //  try to place the value token in the expression tree
        //  if successful then update it_begin and return
        bool success = false;
        if (read_result.scope_closed &&
            ptr_expression &&
            (   ptr_expression->is_value() ||
                (ptr_expression->lexem.type == ctoken::etype::scope &&
                 ptr_expression->lexem.scope_closed == false)))
        {
            T_expression_tree* pparent = ptr_expression.get();
            while (pparent != nullptr &&
                   (pparent->lexem.type != ctoken::etype::scope ||
                    pparent->lexem.scope_closed != false))
            {
                pparent = pparent->parent;
            }
            //  after this while pparent can either be nullptr,
            //  or it's an open scope, waiting to be closed
            if (pparent &&
                read_result.rtt == pparent->lexem.rtt)
            {
                ptr_expression.release();
                ptr_expression.reset(pparent);
                ptr_expression->lexem.scope_closed = true;
                ptr_expression->lexem.value += read_result.value;
                success = true;
            }
        }
        else if (read_result.scope_closed == false &&
                 nullptr == ptr_expression)
        {
            ptr_expression.reset(new T_expression_tree);
            ptr_expression->lexem = read_result;
            success = true;
        }
        else if (read_result.scope_closed == false &&
                 (  ptr_expression->lexem.type == token_type::bin_op ||
                    (   ptr_expression->lexem.type == token_type::scope &&
                        ptr_expression->lexem.scope_closed == false)))
        {
            T_expression_tree* ptemp = &ptr_expression->add_child(read_result);

            ptr_expression.release();
            ptr_expression.reset(ptemp);
            success = true;
        }

        if (success)
        {
            it_begin = it_copy;
            return true;
        }
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_val_code to false
        it_copy = it_begin;
        read_scope_code = false;
    }

    if (read_val_code == false &&
        read_op_code == false &&
        read_scope_code == false)
        return false;

    return true;
}

namespace detail
{
template <typename T_lexer,
          typename T_string>
class final_check_helper
{
    DECLARE_MF_INSPECTION(final_check, TT,
                          bool (TT::*)(typename T_string::iterator,
                                       typename T_string::iterator) const)
public:
    template <typename T,
              typename TEST = typename std::enable_if<
                  has_final_check<T>::value == 1, bool
                  >::type>
    static bool check(T const* p,
                      typename T_string::iterator it_begin,
                      typename T_string::iterator it_end)
    {
        return p->final_check(it_begin, it_end);
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
template <typename T_lexer,
          typename T_string>
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
          typename T_string>
bool lexer_helper(typename T_string::iterator& it_begin,
                  typename T_string::iterator it_end,
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
        {
            if (index == 0)
            {
                result = false;
                break;
            }
            else
            {
                it_begin = it_copy;
                break;
            }
        }
        else if (true == lexer_res.second)
        {
            ++it_copy;
            it_begin = it_copy;
            break;
        }
    }

    if (result &&
        it_begin == it_backup &&
        false == detail::scan_beyond_helper<T_lexer, T_string>::check(&lexer))
        it_begin = it_end;

    if (result && it_begin != it_backup)
    {
        if (false ==
                detail::final_check_helper<T_lexer, T_string>::check(&lexer,
                                                                     it_backup,
                                                                     it_begin))
        {
            result = false;
            it_begin = it_backup;
        }
    }

    if (p)
        *p = lexer;
    return result;
}

template <typename T_discard_lexer,
          typename T_list_discard_lexers,
          typename T_white_space_set>
class discard_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_discard_lexer,
                        T_list_discard_lexers>::value};

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
            result.type = detail::token<T_string>::etype::discard;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;
        }

        return code;
    }

    std::pair<bool, bool> check(char ch)
    {
        if (T_white_space_set::value.find(ch) != std::string::npos)
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }
};

template <typename T_operator_lexer,
          typename T_list_operator_lexers,
          typename T_operator_set>
class operator_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_operator_lexer,
                        T_list_operator_lexers>::value};

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
        bool code = lexer_helper<T_operator_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            result.priority = T_operator_lexer::priority;
            result.rtt = T_operator_lexer::rtt;
            result.type = detail::token<T_string>::etype::bin_op;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;
        }

        return code;
    }

    std::pair<bool, bool> check(char ch)
    {
        if (T_operator_set::value.find(ch) != std::string::npos)
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }
private:
    enum { previous_rtt = (rtt == 0) ? 0 : rtt - 1 };
};

template <typename T_operator_lexer,
          typename T_list_operator_lexers,
          typename T_operator_set>
size_t const
operator_lexer_base<T_operator_lexer, T_list_operator_lexers, T_operator_set>::priority =
        T_operator_lexer::grow_priority +
        ((T_operator_lexer::rtt == 0) ? 0 :
         (typelist::type_list_get<T_operator_lexer::previous_rtt, T_list_operator_lexers>::type::priority));

template <typename T_value_lexer,
          typename T_list_value_lexers>
class value_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_value_lexer,
                        T_list_value_lexers>::value};

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
            result.rtt = T_value_lexer::rtt;
            result.type = detail::token<T_string>::etype::value;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;
        }

        return code;
    }
};

template <typename T_scope_lexer,
          typename T_list_scope_lexers>
class scope_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<
                        T_scope_lexer,
                        T_list_scope_lexers>::value};

    template <typename T_string,
              typename T_iterator>
    static
    bool internal_read(T_iterator& it_begin,
                       T_iterator it_end,
                       typename detail::token<T_string>& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>();
        T_scope_lexer lexer;
        bool code = lexer_helper<T_scope_lexer, T_string>(it_copy, it_end, &lexer);

        if (code && it_copy != it_begin)
        {
            result.rtt = T_scope_lexer::rtt;
            result.scope_closed = false;
            result.type = detail::token<T_string>::etype::scope;
            result.value.assign(it_begin, it_copy);

            it_begin = it_copy;
            if (lexer.is_close)
                result.scope_closed = true;
        }

        return code;
    }
};

struct standard_white_space_set { public: static const std::string value; };
std::string const standard_white_space_set::value = " \0\t\r\n";
struct standard_operator_set { public: static const std::string value; };
std::string const standard_operator_set::value = "+-*&/\\=!$@^%,;:";

using standard_discard_lexers =
beltpp::typelist::type_list<
class standard_white_space_lexer
>;
class standard_white_space_lexer :
        public beltpp::discard_lexer_base<standard_white_space_lexer,
                                            standard_discard_lexers,
                                            beltpp::standard_white_space_set>
{};

using standard_value_lexers =
beltpp::typelist::type_list<
class standard_value_string_lexer,
class standard_value_number_lexer
>;

class standard_value_string_lexer :
        public beltpp::value_lexer_base<standard_value_string_lexer,
                                        standard_value_lexers>
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

class standard_value_number_lexer :
        public beltpp::value_lexer_base<standard_value_number_lexer,
                                        standard_value_lexers>
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

using standard_list_operator_lexers =
beltpp::typelist::type_list<
class standard_operator_comma_lexer
>;
class standard_operator_comma_lexer :
        public beltpp::operator_lexer_base<standard_operator_comma_lexer,
                                            standard_list_operator_lexers,
                                            beltpp::standard_operator_set>
{
public:
    enum { grow_priority = 1 };
    bool final_check(std::string::iterator it_begin,
                     std::string::iterator it_end) const
    {
        return std::string(it_begin, it_end) == ",";
    }
};

using standard_parentheses_lexers =
beltpp::typelist::type_list<
class standard_parentheses_lexer
>;

class standard_parentheses_lexer :
        public beltpp::scope_lexer_base<standard_parentheses_lexer,
                                        standard_parentheses_lexers>
{
public:
    bool is_close = false;

    std::pair<bool, bool> check(char ch)
    {
        if (ch == ')')
        {
            is_close = true;
            return std::make_pair(true, true);
        }
        if (ch == '(')
            return std::make_pair(true, true);

        return std::make_pair(false, false);
    }

    bool final_check(std::string::iterator it_begin,
                     std::string::iterator it_end) const
    {
        --it_end;
        return (*it_end == '(' || *it_end == ')');
    }
};

}   //  end of namespace beltpp
