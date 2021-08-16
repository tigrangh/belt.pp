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
    size_t rtt = size_t(-1);
    size_t priority = 0;
    size_t right = size_t(-1);
    size_t left_min = size_t(-1);
    size_t left_max = size_t(-1);
    size_t property = size_t(-1);
    T_string value;
};


inline bool property_check(size_t property_first, size_t property_second) noexcept
{
    if (0 == property_second &&
        0 != property_first)
        return true;
    if (0 == property_first % property_second &&
        (property_first != property_second ||
         property_first == 1))
        return true;
    return false;
}

inline size_t property_combine(size_t property_first, size_t property_second) noexcept
{
    if (0 == property_second)
        return 0;
    return property_first / property_second;
}

inline bool property_final(size_t property) noexcept
{
    bool result = false;
    switch (property)
    {
    case 0: case 1: case 2: case 3: case 5: case 7:
        result = true;
        break;
    default:
        result = false;
    }
    return result;
}
}

template <typename T_lexers, typename T_string>
class expression_tree;

template <typename T_lexers, typename T_string>
class expression_tree_pointer
{
public:
    using lexers = T_lexers;
    using string = T_string;
    using ctoken = detail::token<T_string>;

    expression_tree<T_lexers, T_string> root;
    std::vector<expression_tree<T_lexers, T_string>*> stack;

    expression_tree_pointer()
        : root()
        , stack()
    {}

    expression_tree_pointer(expression_tree_pointer&&) = default;
    expression_tree_pointer(expression_tree_pointer const&) = delete;
    inline ~expression_tree_pointer() noexcept;

    inline expression_tree_pointer& operator = (expression_tree_pointer const& other) = delete;
    inline expression_tree_pointer& operator = (expression_tree_pointer&& other) noexcept = default;

    inline bool is_empty() const;
    inline void create();
    inline bool has_parent() const;
    inline void become_parent();
    inline void delete_last_child();
    inline void add_and_become_child(ctoken&& child_lexem);
    inline void add_and_become_child(expression_tree<T_lexers, T_string>&& child);
    inline expression_tree<T_lexers, T_string>& item();
};

template <typename T_lexers, typename T_string>
class expression_tree
{
public:
    using lexers = T_lexers;
    using string = T_string;
    using ctoken = detail::token<T_string>;

    expression_tree() = default;
    expression_tree(expression_tree&) = delete;
    expression_tree(expression_tree&&) = default;
    inline ~expression_tree() noexcept;

    inline expression_tree& operator = (expression_tree const& other) = delete;
    inline expression_tree& operator = (expression_tree&& other) noexcept = default;

    inline void clear() noexcept;
    inline size_t depth() const noexcept;
    inline bool is_value() const noexcept;
    inline bool is_operator() const noexcept;
    inline expression_tree& add_child(ctoken&& child_lexem);
    inline expression_tree& add_child(expression_tree<T_lexers, T_string>&& child);

    ctoken lexem;
    std::vector<expression_tree> children;
};


template <typename T_lexers, typename T_string>
expression_tree_pointer<T_lexers, T_string>::~expression_tree_pointer() noexcept
{
    root.clear();
}
template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>&
expression_tree_pointer<T_lexers, T_string>::item()
{
    return *stack.back();
}
template <typename T_lexers, typename T_string>
bool expression_tree_pointer<T_lexers, T_string>::is_empty() const
{
    return stack.empty();
}
template <typename T_lexers, typename T_string>
void expression_tree_pointer<T_lexers, T_string>::create()
{
    stack.push_back(&root);
}
template <typename T_lexers, typename T_string>
bool expression_tree_pointer<T_lexers, T_string>::has_parent() const
{
    return stack.size() > 1;
}
template <typename T_lexers, typename T_string>
void expression_tree_pointer<T_lexers, T_string>::become_parent()
{
    stack.erase(stack.begin() + stack.size() - 1);
}
template <typename T_lexers, typename T_string>
void expression_tree_pointer<T_lexers, T_string>::delete_last_child()
{
    auto& children = item().children;
    children.erase(children.begin() + children.size() - 1);
}
template <typename T_lexers, typename T_string>
void expression_tree_pointer<T_lexers, T_string>::add_and_become_child(expression_tree<T_lexers, T_string>&& child)
{
    auto& ref = item().add_child(std::move(child));
    stack.push_back(&ref);
}
template <typename T_lexers, typename T_string>
void expression_tree_pointer<T_lexers, T_string>::add_and_become_child(ctoken&& child_lexem)
{
    auto& ref = item().add_child(std::move(child_lexem));
    stack.push_back(&ref);
}

template <typename T_lexers, typename T_string>
inline
expression_tree<T_lexers, T_string>::~expression_tree() noexcept = default;
// {
//     if (false == children.empty())
//     {
//         queue<expression_tree<T_lexers, T_string>> items;
//         items.push(std::move(*this));

//         while (false == items.empty())
//         {
//             auto item = std::move(items.front());

//             items.reserve(items.size() + item.children.size());
//             for (auto&& child : item.children)
//                 items.push(std::move(child));
//             item.children.clear();

//             items.pop();
//         }
//     }
// }

template <typename T_lexers, typename T_string>
inline
void expression_tree<T_lexers, T_string>::clear() noexcept
{
    std::vector<size_t> track;
    size_t depth = 1;

    while (depth)
    {
        expression_tree* p_iterator = this;
        for (size_t index = 0; index != depth - 1; ++index)
            p_iterator = &p_iterator->children[track[index]];

        size_t next_child_index = size_t(-1);
        if (false == p_iterator->children.empty())
        {
            size_t childindex;
            if (track.size() < depth)
            {
                track.push_back(0);
                childindex = 0;
            }
            else
            {
                ++track[depth - 1];
                childindex = track[depth - 1];
            }

            if (childindex < p_iterator->children.size())
                next_child_index = childindex;
        }

        if (size_t(-1) != next_child_index)
        {
            ++depth;
            if (track.size() > depth - 1)
                track.resize(depth - 1);
        }
        else
        {
            --depth;
            p_iterator->children.clear();
        }
    }
}
template <typename T_lexers, typename T_string>
inline
size_t expression_tree<T_lexers, T_string>::depth() const noexcept
{
    size_t max_depth = 1;
    std::vector<size_t> track;
    size_t depth = 1;

    while (depth)
    {
        expression_tree const* p_iterator = this;
        for (size_t index = 0; index != depth - 1; ++index)
            p_iterator = &p_iterator->children[track[index]];

        size_t next_child_index = size_t(-1);
        if (false == p_iterator->children.empty())
        {
            size_t childindex;
            if (track.size() < depth)
            {
                track.push_back(0);
                childindex = 0;
            }
            else
            {
                ++track[depth - 1];
                childindex = track[depth - 1];
            }

            if (childindex < p_iterator->children.size())
                next_child_index = childindex;
        }

        if (size_t(-1) != next_child_index)
        {
            ++depth;

            if (max_depth < depth)
                max_depth = depth;

            if (track.size() > depth - 1)
                track.resize(depth - 1);
        }
        else
        {
            --depth;
        }
    }

    {
        size_t depth_other = size_t(-1);
        queue<expression_tree<T_lexers, T_string> const*> items;

        items.push(this);

        while (false == items.empty())
        {
            auto begin_index = items.begin_index();
            auto end_index = items.end_index();
            for (auto index = begin_index; index != end_index; ++index)
            {
                auto pitem = items[index];
                for (auto& child : pitem->children)
                    items.push(&child);
                items.pop();
            }
            ++depth_other;
        }

        if (depth_other != max_depth - 1)
            std::terminate();
    }

    return max_depth - 1;
}

template <typename T_lexers, typename T_string>
bool expression_tree<T_lexers, T_string>::is_value() const noexcept
{
    return (lexem.right == 0 && detail::property_final(lexem.property));
}

template <typename T_lexers, typename T_string>
bool expression_tree<T_lexers, T_string>::is_operator() const noexcept
{
    return lexem.right > 0;
}

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>&
expression_tree<T_lexers,T_string>::add_child(ctoken&& child_lexem)
{
    expression_tree<T_lexers,T_string> child;
    child.lexem = std::move(child_lexem);

    return add_child(std::move(child));
}

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>&
expression_tree<T_lexers, T_string>::add_child(expression_tree<T_lexers, T_string>&& child)
{
    children.push_back(std::move(child));

    return children.back();
}

namespace detail
{
template <typename T_lexer,
          typename T_string>
class get_default_helper
{
    DECLARE_MF_INSPECTION(get_default, TT,
                          bool (TT::*)(token<T_string>& result) const)
public:
    template <typename T,
              typename TEST = typename std::enable_if<
                  has_get_default<T>::value == 1, bool
                  >::type>
    static bool check(T const* p, token<T_string>* result)
    {
        return p->get_default(*result);
    }
    template <typename T = T_lexer,
              typename TEST = typename std::enable_if<
                  has_get_default<T>::value == 0, char
                  >::type>
    static bool check(...)
    {
        return false;
    }
};
template <typename T_lexers, typename T_string, typename T_iterator>
class storage
{
public:
    using fptr_reader = beltpp::e_three_state_result (*)(T_iterator&, T_iterator const&, token<T_string>&);
    enum { count = T_lexers::count };
public:
    storage() {++s_initializer;}
    ~storage() {--s_initializer;}
    static int s_initializer;
public:
    static fptr_reader s_readers[T_lexers::count];
};
template <typename T_lexers, typename T_string, typename T_iterator, typename INDEX>
class dummy
{
public:

    inline static
    int list_readers()
    {
        if (INDEX::value < T_lexers::count)
        {
            using storage =
                detail::storage<T_lexers, T_string, T_iterator>;
            using T_lexer =
                typename typelist::type_list_get<INDEX::value, T_lexers>::type;
            storage::s_readers[INDEX::value] =
                    &T_lexer::template internal_read<T_string, T_iterator>;

            using cdummy = dummy<T_lexers, T_string, T_iterator,
            std::integral_constant<size_t, INDEX::value + 1>>;

            return cdummy::list_readers();
        }
        return 0;
    }

    inline static
    bool get_default_operator(token<T_string>& result)
    {
        auto const discard_result = token<T_string>();
        if (INDEX::value >= T_lexers::count)
            return false;
        else
        {
            using T_lexer =
                typename typelist::type_list_get<INDEX::value, T_lexers>::type;

            T_lexer lexer;
            auto current_result = discard_result;
            bool current_code =
                get_default_helper<T_lexer, T_string>::check(&lexer,
                                                             &current_result);

            if (current_code)
            {
                result = current_result;
                return true;
            }
            else if (INDEX::value + 1 == T_lexers::count)
            {
                return false;
            }
            else    // if (INDEX::value + 1 < T_list::count)
            {
                using cdummy = dummy<T_lexers, T_string, T_iterator,
                std::integral_constant<size_t, INDEX::value + 1>>;

                return cdummy::get_default_operator(result);
            }
        }
    }
};

template <typename T_lexers, typename T_string, typename T_iterator>
class dummy<T_lexers, T_string, T_iterator,
        std::integral_constant<size_t, T_lexers::count>>
{
public:

    inline static
    int list_readers()
    {
        return 0;
    }

    inline static
    bool get_default_operator(token<T_string>&/* result*/)
    {
        return false;
    }
};

template <typename T_lexers, typename T_string, typename T_iterator>
typename storage<T_lexers, T_string, T_iterator>::fptr_reader
storage<T_lexers, T_string, T_iterator>::s_readers[T_lexers::count];
template <typename T_lexers, typename T_string, typename T_iterator>
int storage<T_lexers, T_string, T_iterator>::s_initializer =
        dummy<T_lexers, T_string, T_iterator, std::integral_constant<size_t, 0>>::list_readers();

template <typename T_lexers, typename T_string>
inline
bool parse_helper(expression_tree_pointer<T_lexers, T_string>& ptr_expression,
                  typename expression_tree<T_lexers, T_string>::ctoken&& read_result,
                  bool has_default_operator,
                  typename expression_tree<T_lexers, T_string>::ctoken const& default_operator);
}   //  end detail



inline
std::string::const_iterator
check_begin(std::string::const_iterator const& iter_scan_begin,
            std::string::const_iterator const& iter_scan_end,
            std::string const& value) noexcept
{
    auto it_scan = iter_scan_begin;
    auto it_value = value.begin();

    while (true)
    {
        if (it_scan == iter_scan_end ||
            it_value == value.end())
            break;

        if (*it_value == *it_scan)
        {
            ++it_value;
            ++it_scan;
        }
        else
        {
            it_scan = iter_scan_begin;
            break;
        }
    }

    return it_scan;
}

inline
std::pair<std::string::const_iterator, std::string::const_iterator>
check_end(std::string::const_iterator const& iter_scan_begin,
          std::string::const_iterator const& iter_scan_end,
          std::string const& value) noexcept
{
    auto it_scan_begin = iter_scan_begin;
    auto it_scan = it_scan_begin;
    auto it_value = value.begin();

    while (true)
    {
        if (it_scan == iter_scan_end ||
            it_value == value.end())
            break;

        if (*it_value == *it_scan)
        {
            ++it_value;
            ++it_scan;
        }
        else
        {
            it_value = value.begin();
            ++it_scan_begin;
            it_scan = it_scan_begin;
        }
    }

    return std::make_pair(it_scan_begin, it_scan);
}

template <typename T_iterator,
          typename T_lexers,
          typename T_string>
inline
e_three_state_result parse(expression_tree_pointer<T_lexers, T_string>& ptr_expression,
                           T_iterator& it_begin,
                           T_iterator const& it_end)
{
    using expression_tree_pointer = expression_tree_pointer<T_lexers, T_string>;
    using storage = detail::storage<T_lexers, T_string,T_iterator>;
    //  fake, as if using the following member
    //  force template compiler to use it too
    auto storage_initialized = storage::s_initializer;
    B_UNUSED(storage_initialized);  //  avoid warning/error
    auto readers = storage::s_readers;

    static auto default_operator = typename expression_tree_pointer::ctoken();
    auto read_result = typename expression_tree_pointer::ctoken();

    using cdummy =
        detail::dummy<T_lexers, T_string,
                        T_iterator, std::integral_constant<size_t, 0>>;

    auto it_copy = it_begin;

    // looks like clang is able to take advantage of this function being inline
    // but not g++. anyway, we can make this call static to optimize g++
    static bool has_default_operator = cdummy::get_default_operator(default_operator);

    e_three_state_result error_attempt = e_three_state_result::error;
    for (size_t reader_index = 0; reader_index < storage::count; ++reader_index)
    {
        e_three_state_result read_op_code =
                readers[reader_index](it_copy, it_end, read_result);
        if (e_three_state_result::success == read_op_code &&
            it_copy == it_begin)
        {
            assert(false);
            read_op_code = e_three_state_result::error;
        }

        if (e_three_state_result::success == read_op_code)
        {   //  try to place the operator token in the expression tree
            //  if successful then update it_begin and return
            auto ptr_expression_stack_backup = ptr_expression.stack;

            if (detail::parse_helper(ptr_expression,
                                     std::move(read_result),
                                     has_default_operator,
                                     default_operator))
            {
                it_begin = it_copy;
                return e_three_state_result::success;
            }
            else
            {   //  otherwise don't forget to reset it_copy to it_begin
                it_copy = it_begin;
                assert(ptr_expression_stack_backup == ptr_expression.stack);
            }
        }
        else if (e_three_state_result::attempt == read_op_code)
            //  if at least one lexer returns attempt, and not one success
            //  will return attempt
            error_attempt = e_three_state_result::attempt;
    }
    //  will return error only if all lexers return error or
    //  success without successful parse_helper
    return error_attempt;
}

namespace detail
{
template <typename T_lexers, typename T_string>
bool parse_helper(expression_tree_pointer<T_lexers, T_string>& ptr_expression,
                  typename expression_tree<T_lexers, T_string>::ctoken&& read_result,
                  bool has_default_operator,
                  typename expression_tree<T_lexers, T_string>::ctoken const& default_operator)
{
    using expression_tree = expression_tree<T_lexers, T_string>;

    bool success = false;
    enum token_type {token_type_value, token_type_operator};
    std::vector<token_type> types;
    types.reserve(2);
    if (read_result.right == size_t(-1) &&
        read_result.left_max == size_t(-1) &&
        read_result.left_min == size_t(-1))
        success = true; //  discard the result
    else
    {
        if (read_result.left_max > 0)
            types.push_back(token_type_operator);

        expression_tree exp_tree_temp;
        exp_tree_temp.lexem = read_result;

        if (read_result.left_min == 0 &&
                (
                    exp_tree_temp.is_value() ||
                    exp_tree_temp.is_operator()
                )
            )
            types.push_back(token_type_value);
    }

    for (auto ttype : types)
    {
        if (success)
            break;
        if (token_type_operator == ttype &&
            false == ptr_expression.is_empty())
        {
            size_t pparent_index = ptr_expression.stack.size() - 1;
            size_t test_pparent_index = pparent_index;
            expression_tree* pparent = ptr_expression.stack[pparent_index];
            expression_tree* test_pparent = pparent;
            while (pparent &&
                   pparent->is_value() &&
                   pparent->lexem.priority >= read_result.priority)
            {
                test_pparent = pparent;
                test_pparent_index = pparent_index;

                if (pparent_index == 0)
                    pparent = nullptr;
                else
                {
                    --pparent_index;
                    pparent = ptr_expression.stack[pparent_index];
                }
            }
            assert(test_pparent);
            pparent = test_pparent;
            pparent_index = test_pparent_index;
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
                    pparent_index = test_pparent_index;
                    break;
                }
                if (0 == test_pparent_index ||
                    false == test_pparent->is_value())
                    break;
                --test_pparent_index;
                test_pparent = ptr_expression.stack[test_pparent_index];
            }

            bool fix_the_same_possible = false;
            if (pparent->lexem.rtt == read_result.rtt &&
                detail::property_check(pparent->lexem.property,
                                       read_result.property))
                fix_the_same_possible = true;

            bool ok = true;
            size_t left_count = 1;
            if (fix_the_same_possible)
                left_count = pparent->children.size();
            else if (false == pparent->is_value())
                ok = false;

            if (ok &&
                left_count >= read_result.left_min &&
                left_count <= read_result.left_max)
            {
                read_result.left_min = 0;
                read_result.left_max -= left_count;

                if (&ptr_expression.item() != pparent)
                    ptr_expression.stack.resize(pparent_index + 1);

                if (fix_the_same_possible)
                {
                    auto property_backup = ptr_expression.item().lexem.property;
                    auto value_backup = std::move(ptr_expression.item().lexem.value);
                    ptr_expression.item().lexem = std::move(read_result);
                    ptr_expression.item().lexem.value =
                            value_backup + ptr_expression.item().lexem.value;
                    ptr_expression.item().lexem.property =
                            detail::property_combine(property_backup,
                                                     ptr_expression.item().lexem.property);
                }
                else
                {
                    expression_tree expression_move = std::move(ptr_expression.item());

                    ptr_expression.item().lexem = std::move(read_result);
                    ptr_expression.item().add_child(std::move(expression_move));
                }

                success = true;
            }
        }
        else if (token_type_value == ttype)
        {
            size_t pparent_index = ptr_expression.stack.size() - 1;
            size_t pparent_previous_index = pparent_index;
            expression_tree* pparent = nullptr;
            if (false == ptr_expression.is_empty())
                pparent = ptr_expression.stack[pparent_index];
            expression_tree* pparent_previous = pparent;

            while (pparent &&
                   false == pparent->is_operator() &&
                   pparent->is_value())
            {
                pparent_previous = pparent;
                pparent_previous_index = pparent_index;
                if (pparent_index == 0)
                    pparent = nullptr;
                else
                {
                    --pparent_index;
                    pparent = ptr_expression.stack[pparent_index];
                }
            }

            if (ptr_expression.is_empty())
            {
                ptr_expression.create();
                ptr_expression.item().lexem = std::move(read_result);
                success = true;
            }
            else if (pparent && false == pparent->is_operator())
            {
                assert(false);
            }
            else if (pparent)
            {
                ptr_expression.stack.resize(pparent_index + 1);
                ptr_expression.add_and_become_child(std::move(read_result));

                --pparent->lexem.right;
                success = true;
            }
            else if (has_default_operator)
            {
                assert(pparent_previous);
                assert(0 == pparent_previous_index);
                pparent = pparent_previous;
                pparent_index = pparent_previous_index;

                if (pparent->lexem.rtt == default_operator.rtt)
                {
                    ptr_expression.stack.resize(pparent_index + 1);
                    ptr_expression.add_and_become_child(std::move(read_result));
                    success = true;
                }
                else
                {
                    ptr_expression.stack.resize(pparent_index + 1);
                    expression_tree expression_move = std::move(ptr_expression.item());

                    ptr_expression.item().lexem = default_operator;
                    ptr_expression.item().add_child(std::move(expression_move));

                    ptr_expression.add_and_become_child(std::move(read_result));

                    success = true;
                }
            }
        }
    }

    return success;
}

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
                  T_iterator const& it_end,
                  T_lexer* p = nullptr)
{
    bool result = true;

    size_t index = 0;
    auto it_copy = it_begin;
    auto const it_backup = it_begin;
    T_lexer lexer;
    for (; it_copy != it_end; ++it_copy, ++index)
    {
        beltpp::e_three_state_result lexer_res = lexer.check(*it_copy);
        if (beltpp::e_three_state_result::error == lexer_res)
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
        else if (beltpp::e_three_state_result::success == lexer_res)
        {   //  next symbol is accepted and suggested to be final
            ++it_copy;
            it_begin = it_copy;
            break;
        }
    }

    if (result &&                   //  something accepted, but not sure if its final
        it_begin == it_backup &&    //  so it_begin was not altered yet
        false == detail::scan_beyond_helper<T_lexer>::check(&lexer))
        it_begin = it_end;          //  now alter it_begin too, this does not happen by default
                                    //  alternatively the scanning goes past last "attempted" symbol accept it

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
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
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

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
    }

    beltpp::e_three_state_result check(char ch)
    {
        auto value = T_white_space_set::value;
        if (value.find(ch) != std::string::npos)
            return beltpp::e_three_state_result::attempt;
        return beltpp::e_three_state_result::error;
    }
};

template <typename T_discard_lexer, typename T_lexers>
class discard_lexer_base_flexible
{
public:
    enum { rtt = typelist::type_list_index<
                        T_discard_lexer,
                        T_lexers>::value};

    template <typename T_string,
              typename T_iterator>
    static
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
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

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
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
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
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
            result.property = lexer.property;
            it_begin = it_copy;

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
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
beltpp::e_three_state_result standard_operator_check(char ch)
{
    if (T_operator_set::value.find(ch) != std::string::npos)
        return beltpp::e_three_state_result::attempt;
    return beltpp::e_three_state_result::error;
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
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
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
            result.property = 1;
            result.value.assign(it_begin, it_copy);
            it_begin = it_copy;

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
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
    size_t index = size_t(-1);
public:
    beltpp::e_three_state_result check(char ch)
    {
        ++index;
        if (0 == index && ch == '\'')
        {
            state |= state_single_quote_open;
            return beltpp::e_three_state_result::attempt;
        }
        if (0 == index && ch == '\"')
        {
            state |= state_double_quote_open;
            return beltpp::e_three_state_result::attempt;
        }
        if (0 == index && ch != '\'' && ch != '\"')
            return beltpp::e_three_state_result::error;
        if (0 != index && ch == '\'' &&
            (state & state_single_quote_open) &&
            !(state & state_escape_symbol))
            return beltpp::e_three_state_result::success;
        if (0 != index && ch == '\"' &&
            (state & state_double_quote_open) &&
            !(state & state_escape_symbol))
            return beltpp::e_three_state_result::success;
        if (0 != index && ch == '\\' &&
            !(state & state_escape_symbol))
        {
            state |= state_escape_symbol;
            return beltpp::e_three_state_result::attempt;
        }
        state &= ~state_escape_symbol;
        return beltpp::e_three_state_result::attempt;
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
            beltpp::stoi64(v, pos);

        if (pos != v.length())
            return false;
        return true;
    }
public:
    beltpp::e_three_state_result check(char ch)
    {
        value += ch;
        if ("." == value || "-" == value || "-." == value)
            return beltpp::e_three_state_result::attempt;
        else if (_check(value))
            return beltpp::e_three_state_result::attempt;
        else
            return beltpp::e_three_state_result::error;
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
    size_t left_max = size_t(-1);
    size_t left_min = 1;
    size_t property = 1;

    enum { grow_priority = 1 };

    beltpp::e_three_state_result check(char ch)
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
typename T_expression_tree::string dump(T_expression_tree const& expression)
{
    typename T_expression_tree::string result;

    std::vector<size_t> track;
    size_t depth = 1;

    while (depth)
    {
        T_expression_tree const* p_iterator = &expression;
        for (size_t index = 0; index != depth - 1; ++index)
            p_iterator = &p_iterator->children[track[index]];

        if (track.size() == depth - 1)
        {
            for (size_t index = 0; index != depth - 1; ++index)
                result += ".";
            result += p_iterator->lexem.value;
            if (p_iterator->lexem.right > 0)
                result += " !!!! ";
            result += "\n";
        }

        size_t next_child_index = size_t(-1);
        if (false == p_iterator->children.empty())
        {
            size_t childindex;
            if (track.size() < depth)
            {
                track.push_back(0);
                childindex = 0;
            }
            else
            {
                ++track[depth - 1];
                childindex = track[depth - 1];
            }

            if (childindex < p_iterator->children.size())
                next_child_index = childindex;
        }

        if (size_t(-1) != next_child_index)
        {
            ++depth;
            if (track.size() > depth - 1)
                track.resize(depth - 1);
        }
        else
        {
            --depth;
        }
    }

    return result;
}

template <typename T_lexers, typename T_string>
expression_tree<T_lexers, T_string>* root(expression_tree_pointer<T_lexers, T_string>& ptr_expression, bool& is_value)
{
    expression_tree<T_lexers, T_string>* pexpression = nullptr;
    size_t pexpression_index = ptr_expression.stack.size() - 1;
    if (false == ptr_expression.is_empty())
        pexpression = ptr_expression.stack[pexpression_index];

    bool missing_expression = false;
    if (nullptr == pexpression)
        missing_expression = true;
    else if (false == pexpression->is_value())
        missing_expression = true;

    if (pexpression)
    while (pexpression_index)
    {
        --pexpression_index;
        auto parent = ptr_expression.stack[pexpression_index];

        if (false == parent->is_value())
            missing_expression = true;
        pexpression = parent;
    }

    is_value = !missing_expression;
    return pexpression;
}

}   //  end of namespace beltpp
