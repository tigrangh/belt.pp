#pragma once

#include "global.hpp"
#include "meta.hpp"

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
        e_bin_op,
        e_un_op,
        e_value,
        e_scope,
        e_discard
    };

    bool scope_closed = false;
    etype type = etype::e_discard;
    size_t position = 0;
    size_t rtt = -1;
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
    using ptr_expression_tree = std::unique_ptr<expression_tree>;

private:
    bool is_node() const noexcept;
    bool is_leaf() const noexcept;

    ctoken lexem;
    std::unique_ptr<std::vector<ptr_expression_tree>> ptr_arr_children;
};

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
    if (ptr_arr_children &&
        false == ptr_arr_children->empty())
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
    if (nullptr == ptr_arr_children)
        return true;
    return false;
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
          std::pair<size_t, typename detail::token<T_string>::etype>& result)
    {
        auto it_copy = it_begin;
        auto const discard_result = detail::token<T_string>::etype::e_discard;
        if (INDEX::value >= T_list_lexers::count)
            return false;
        else
        {
            using token_type =
                typename typelist::type_list_get<INDEX::value, T_list_lexers>::type;
            auto current_result = discard_result;
            bool current_code =
                    token_type::template internal_read<T_string, T_iterator>
                                            (it_copy,
                                             it_end,
                                             current_result);

            if (INDEX::value + 1 == T_list_lexers::count ||
                (current_code && it_copy != it_begin))
            {
                if (current_code && it_copy != it_begin)
                    it_begin = it_copy;

                result.second = current_result;
                result.first = token_type::rtt;
                return current_code;
            }
            else    // if (INDEX::value + 1 < T_list::count)
            {
                it_copy = it_begin;
                using cdummy = dummy<T_list_lexers, T_string, T_iterator,
                std::integral_constant<size_t, INDEX::value + 1>>;

                auto next_result = std::make_pair(size_t(-1), discard_result);
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
              std::pair<size_t, typename detail::token<T_string>::etype>& result)
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
    auto read_result = std::make_pair(size_t(-1),
                                      T_expression_tree::ctoken::etype::e_discard);

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
        it_begin = it_copy;
        return true;
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_op_code to false
    }

    bool read_val_code = cdummy_val::read(it_copy, it_end, read_result);
    if (read_val_code && it_copy != it_begin)
    {   //  try to place the value token in the expression tree
        //  if successful then update it_begin and return
        it_begin = it_copy;
        return true;
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_val_code to false
    }

    bool read_scope_code = cdummy_scope::read(it_copy, it_end, read_result);
    if (read_scope_code && it_copy != it_begin)
    {   //  try to place the value token in the expression tree
        //  if successful then update it_begin and return
        it_begin = it_copy;
        return true;
        //  otherwise don't forget to reset it_copy to it_begin
        //  and set read_val_code to false
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

struct standard_white_space_set { public: static const std::string value; };
std::string const standard_white_space_set::value = " \t\n";
struct standard_operator_set { public: static const std::string value; };
std::string const standard_operator_set::value = "+-*&/\\=!$@^%,;:";

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
                       typename detail::token<T_string>::etype& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>::etype::e_discard;
        bool code = lexer_helper<T_discard_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            it_begin = it_copy;
            result = detail::token<T_string>::etype::e_discard;
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

    template <typename T_string,
              typename T_iterator>
    static
    bool internal_read(T_iterator& it_begin,
                       T_iterator it_end,
                       typename detail::token<T_string>::etype& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>::etype::e_discard;
        bool code = lexer_helper<T_operator_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            it_begin = it_copy;
            result = detail::token<T_string>::etype::e_bin_op;
        }

        return code;
    }

    std::pair<bool, bool> check(char ch)
    {
        if (T_operator_set::value.find(ch) != std::string::npos)
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }
};

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
                       typename detail::token<T_string>::etype& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>::etype::e_discard;
        bool code = lexer_helper<T_value_lexer, T_string>(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            it_begin = it_copy;
            result = detail::token<T_string>::etype::e_value;
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
                       typename detail::token<T_string>::etype& result)
    {
        auto it_copy = it_begin;
        result = detail::token<T_string>::etype::e_discard;
        T_scope_lexer lexer;
        bool code = lexer_helper<T_scope_lexer, T_string>(it_copy, it_end, &lexer);

        if (code && it_copy != it_begin)
        {
            it_begin = it_copy;
            if (false == lexer.is_close)
                result = detail::token<T_string>::etype::e_scope;
            else
                result = detail::token<T_string>::etype::e_discard;
        }

        return code;
    }
};

}   //  end of namespace beltpp
