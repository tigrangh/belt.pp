#pragma once

#include "global.hpp"
#include "meta.hpp"
#include "queue.hpp"
#include "scope_helper.hpp"

#include <utility>
#include <memory>
#include <vector>
#include <type_traits>
#include <string>
#include <stdexcept>
#include <cstring>
//
namespace beltpp
{

namespace detail
{
template <typename T_lexers>
class expression_data;
}

// expression tree
//
template <typename T_lexers>
class expression_tree
{
public:
    //  getting rid of this, and instead reallocating before each push_back() call
    //  makes clang happier for performance. gcc optimizes away this wrapper
    class memory_wrapper
    {
        std::vector<detail::expression_data<T_lexers>> memory;
    public:
        inline detail::expression_data<T_lexers>& operator[](size_t index)  noexcept { return memory[index]; }
        inline detail::expression_data<T_lexers> const& operator[](size_t index) const noexcept { return memory[index]; }
        inline size_t size() const noexcept { return memory.size(); }
        inline bool empty() const noexcept { return memory.empty(); }
        inline void push_back(detail::expression_data<T_lexers>&& item)
        {
            if (memory.capacity() == memory.size())
                memory.reserve(memory.size() * 4);
            return memory.emplace_back(std::move(item));
        }
        inline void reserve(size_t size) { memory.reserve(size); }
    };
    using types = typename T_lexers::types;

    std::vector<typename T_lexers::types::T_size_type> node_address;
    memory_wrapper memory;
    std::vector<char> expression_string;

    expression_tree() = default;
    expression_tree(expression_tree&& other) noexcept = default;
    expression_tree(expression_tree const& other) = delete;
    inline ~expression_tree() noexcept = default;

    inline expression_tree& operator = (expression_tree const& other) = delete;
    inline expression_tree& operator = (expression_tree&& other) noexcept = delete;

    inline size_t depth() const noexcept;
    inline std::string dump() const;
    inline bool empty() const noexcept;
    inline void create(detail::expression_data<T_lexers>&& data);
    
    inline bool complete() const noexcept;
    inline detail::expression_data<T_lexers>& item() noexcept;
    inline detail::expression_data<T_lexers>* item2(size_t address_index) noexcept;
    inline typename T_lexers::types::T_size_type child(typename T_lexers::types::T_size_type address,
                                                       typename T_lexers::types::T_size_type child_index) const noexcept;
};

namespace detail
{
// expression data
//
template <typename T_lexers>
class expression_data
{
    class operator_stuff
    {
    public:
        typename T_lexers::types::T_priority_type priority;
        typename T_lexers::types::T_size_type right;
        typename T_lexers::types::T_property_type property;

        typename T_lexers::types::T_size_type children_count;
        typename T_lexers::types::T_size_type last_child_address;
    };
    class value_stuff
    {
    public:
        typename T_lexers::types::T_size_type value_start_index;
        typename T_lexers::types::T_size_type value_size;
    };
public:
    typename T_lexers::types::T_rtt_type rtt;

    bool union_is_value;
    typename T_lexers::types::T_size_type previous_peer_address;
    union
    {
        operator_stuff info_operator;
        value_stuff info_value;
    };

public:
    expression_data();
    expression_data(typename T_lexers::types::T_rtt_type rtt,
                    typename T_lexers::types::T_priority_type priority,
                    typename T_lexers::types::T_size_type right,
                    typename T_lexers::types::T_property_type property);
    expression_data(typename T_lexers::types::T_rtt_type rtt,
                    typename T_lexers::types::T_size_type value_size);
    expression_data(expression_data const& other) = delete;
    expression_data(expression_data&& other) noexcept = default;
    inline ~expression_data() noexcept = default;


    inline expression_data& operator = (expression_data const& other) = delete;
    inline expression_data& operator = (expression_data&& other) noexcept = default;

    inline bool complete() const noexcept;
    inline size_t child_count() const noexcept;

    static inline bool property_check(size_t property_first, size_t property_second) noexcept;
    static inline size_t property_combine(size_t property_first, size_t property_second) noexcept;
    static inline bool property_final(size_t property) noexcept;
};
}   //  end detail

// expression tree implementation
//
template <typename T_lexers>
inline
size_t expression_tree<T_lexers>::depth() const noexcept
{
    if (empty())
        return 0;

    size_t max_depth = 1;
    std::vector<size_t> track;
    size_t depth = 1;

    while (depth)
    {
        auto iterator_address = node_address.back();
        for (size_t index = 0; index != depth - 1; ++index)
            iterator_address = child(iterator_address, track[index]);
        auto const* p_iterator = &memory[iterator_address];

        size_t next_child_index = size_t(-1);
        if (0 != p_iterator->child_count())
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

            if (childindex < p_iterator->child_count())
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

    return max_depth - 1;
}
template <typename T_lexers>
inline
std::string expression_tree<T_lexers>::dump() const
{
    std::string result;

    if (empty())
        return " !!!! \n";

    std::vector<size_t> track;
    size_t depth = 1;

    while (depth)
    {
        auto iterator_address = node_address.back();
        for (size_t index = 0; index != depth - 1; ++index)
            iterator_address = child(iterator_address, track[index]);
        auto const* p_iterator = &memory[iterator_address];

        if (track.size() == depth - 1)
        {
            for (size_t index = 0; index != depth - 1; ++index)
                result += ".";
            if (false == p_iterator->union_is_value)
                result += std::to_string(p_iterator->rtt) + "|" + std::to_string(p_iterator->info_operator.property);
            else if (p_iterator->union_is_value)
                result.insert(result.end(),
                              expression_string.begin() + p_iterator->info_value.value_start_index,
                              expression_string.begin() + p_iterator->info_value.value_start_index +
                                                            p_iterator->info_value.value_size);

            if (false == p_iterator->complete())
                result += " !!!! ";
            result += "\n";
        }

        size_t next_child_index = size_t(-1);
        if (0 != p_iterator->child_count())
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

            if (childindex < p_iterator->child_count())
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
template <typename T_lexers>
inline
detail::expression_data<T_lexers>&
expression_tree<T_lexers>::item() noexcept
{
    assert(false == empty());
    return memory[node_address.back()];
}
template <typename T_lexers>
inline
detail::expression_data<T_lexers>*
expression_tree<T_lexers>::item2(size_t address_index) noexcept
{
    assert(false == empty());
    return address_index != size_t(-1) ? &memory[node_address[address_index]] : nullptr;
}
template <typename T_lexers>
inline
typename T_lexers::types::T_size_type
expression_tree<T_lexers>::child(typename T_lexers::types::T_size_type address,
                                 typename T_lexers::types::T_size_type child_index) const noexcept
{
    auto children_count = memory[address].info_operator.children_count;

    address = memory[address].info_operator.last_child_address;

    typename T_lexers::types::T_size_type index;
    for (index = child_index + 1; index != children_count; ++index)
        address = memory[address].previous_peer_address;

    return address;
}
template <typename T_lexers>
inline
bool expression_tree<T_lexers>::empty() const noexcept
{
    return memory.empty() || node_address.empty();
}
template <typename T_lexers>
inline
void expression_tree<T_lexers>::create(detail::expression_data<T_lexers>&& data)
{
    assert(empty());
    memory.push_back(std::move(data));
    node_address.push_back(0);
}
template <typename T_lexers>
inline
bool expression_tree<T_lexers>::complete() const noexcept
{
    if (empty())
        return false;

    for (size_t index = node_address.size() - 1; index <= node_address.size() - 1; --index)
    {
        if (false == memory[node_address[index]].complete())
            return false;
    }

    return true;
}

namespace detail
{
// expressoin data implementation
//
template <typename T_lexers>
expression_data<T_lexers>::expression_data()
    : rtt(typename T_lexers::types::T_rtt_type(-1))
    , union_is_value(false)
    , previous_peer_address(typename T_lexers::types::T_size_type(-1))
{}
template <typename T_lexers>
expression_data<T_lexers>::expression_data(typename T_lexers::types::T_rtt_type rtt,
                                           typename T_lexers::types::T_priority_type priority,
                                           typename T_lexers::types::T_size_type right,
                                           typename T_lexers::types::T_property_type property)
    : rtt(rtt)
    , union_is_value(false)
    , previous_peer_address(typename T_lexers::types::T_size_type(-1))
    , info_operator(operator_stuff{priority, right, property, 0, typename T_lexers::types::T_size_type(-1)})
{}
template <typename T_lexers>
expression_data<T_lexers>::expression_data(typename T_lexers::types::T_rtt_type rtt,
                                           typename T_lexers::types::T_size_type value_size)
    : rtt(rtt)
    , union_is_value(true)
    , previous_peer_address(typename T_lexers::types::T_size_type(-1))
    , info_value(value_stuff{0, value_size})
{}
template <typename T_lexers>
inline bool expression_data<T_lexers>::complete() const noexcept
{
    if (union_is_value ||
        (
            info_operator.right == 0 &&
            property_final(info_operator.property)
        ))
        return true;
    return false;
}
template <typename T_lexers>
inline size_t expression_data<T_lexers>::child_count() const noexcept
{
    return union_is_value ?
        0 :
        info_operator.children_count;
}
template <typename T_lexers>
inline bool expression_data<T_lexers>::property_check(size_t property_first, size_t property_second) noexcept
{
    if (0 == property_first)
        return false;

    if (0 == property_second % property_first &&
        property_first * property_first >= property_second)
        return true;
    return false;
}
template <typename T_lexers>
inline size_t expression_data<T_lexers>::property_combine(size_t property_first, size_t property_second) noexcept
{
    return property_second / property_first;
}
template <typename T_lexers>
inline bool expression_data<T_lexers>::property_final(size_t property) noexcept
{
    bool result = false;
    switch (property)
    {
    case 0: case 1: case 2:
        result = true;
        break;
    default:
        result = false;
    }
    return result;
}

enum class e_place_as { e_discard, e_value, e_operator };

// default_operator_helper
//
template <typename T_lexers,
          typename TEST = bool>
class default_operator_helper
{
public:
    enum { default_operator_index = T_lexers::list::count };
    inline static expression_data<T_lexers> default_operator()
    {
        return expression_data<T_lexers>();
    }
};
template <typename T_lexers>
class default_operator_helper<T_lexers, typename std::enable_if<size_t(T_lexers::default_operator) < T_lexers::list::count, bool>::type>
{
public:
    enum { default_operator_index = T_lexers::default_operator };
    inline static expression_data<T_lexers> default_operator()
    {
        using T_lexer = typename beltpp::typelist::type_list_get<T_lexers::default_operator, typename T_lexers::list>::type;

        bool place_as_operator;
        typename T_lexers::types::T_property_type property;
        typename T_lexers::types::T_size_type right;

        T_lexer::get_default(place_as_operator,
                             right,
                             property);

        return expression_data<T_lexers>(T_lexer::rtt,
                                         T_lexer::priority,
                                         right,
                                         property);
    }
};
#define PARSE_OLD 0

#if PARSE_OLD
// storage
//
template <typename T_lexers, typename T_iterator>
class storage
{
public:
    using fptr_reader = beltpp::e_three_state_result (*)(T_iterator&,
                                                         T_iterator const&,
                                                         expression_data<T_lexers>&,
                                                         e_place_as&);
    enum { count = T_lexers::list::count };
public:
    storage() {++s_initializer;}
    ~storage() {--s_initializer;}
    static int s_initializer;
public:
    static fptr_reader s_readers[T_lexers::list::count];
};

template <typename T_lexers>
inline
bool parse_helper(expression_tree<T_lexers>& expression,
                  expression_data<T_lexers>&& read_result,
                  e_place_as place_as);

// dummy
//
template <typename T_lexers, typename T_iterator, typename INDEX>
class dummy
{
public:
    inline static
    int list_readers()
    {
        using T_lexer = typename beltpp::typelist::type_list_get<0, typename T_lexers::list>::type;
        T_lexer s_l;
        B_UNUSED(s_l);

        if (INDEX::value < T_lexers::list::count)
        {
            using storage =
                detail::storage<T_lexers, T_iterator>;
            using T_lexer2 =
                typename typelist::type_list_get<INDEX::value, typename T_lexers::list>::type;
            storage::s_readers[INDEX::value] =
                    &T_lexer2::template internal_read<T_iterator>;

            using cdummy = dummy<T_lexers, T_iterator,
            std::integral_constant<size_t, INDEX::value + 1>>;

            return cdummy::list_readers();
        }
        return 0;
    }
};
template <typename T_lexers, typename T_iterator>
class dummy<T_lexers, T_iterator,
        std::integral_constant<size_t, T_lexers::list::count>>
{
public:
    inline static
    int list_readers()
    {
        return 0;
    }
};

template <typename T_lexers, typename T_iterator>
typename storage<T_lexers, T_iterator>::fptr_reader
storage<T_lexers, T_iterator>::s_readers[T_lexers::list::count];
template <typename T_lexers, typename T_iterator>
int storage<T_lexers, T_iterator>::s_initializer =
        dummy<T_lexers, T_iterator, std::integral_constant<size_t, 0>>::list_readers();

template <size_t not_yet_parsed_size_limit,
          size_t depth_limit,
          size_t expression_size_limit,
          size_t buffer_size_limit,
          typename T_iterator,
          typename T_lexers>
inline
bool parse_old(expression_tree<T_lexers>& expression,
               T_iterator& it_begin,
               T_iterator const& it_end)
{
    if (it_begin == it_end)
        return true;

    using storage = detail::storage<T_lexers, T_iterator>;
    //  fake, as if using the following member
    //  force template compiler to use it too
    auto storage_initialized = storage::s_initializer;
    B_UNUSED(storage_initialized);  //  avoid warning/error
    auto const& readers = storage::s_readers;

    auto read_result = detail::expression_data<T_lexers>();
    detail::e_place_as place_as;

    auto it_copy = it_begin;

    bool code_on_not_parsed = false;

    for (size_t reader_index = 0; reader_index < storage::count; ++reader_index)
    {
        e_three_state_result read_op_code =
                readers[reader_index](it_copy, it_end, read_result, place_as);

        if (e_three_state_result::success == read_op_code &&
            it_copy == it_begin)
        {
            assert(false);
            read_op_code = e_three_state_result::error;
        }

        if (e_three_state_result::success == read_op_code)
        {   //  try to place the operator token in the expression tree
            //  on success and checks - update it_begin and return true

            if (detail::parse_helper(expression,
                                     std::move(read_result),
                                     place_as) &&
                expression.node_address.size() <= depth_limit &&
                expression.memory.size() <= expression_size_limit &&
                expression.expression_string.size() + (it_copy - it_begin) <= buffer_size_limit)
            {
                expression.expression_string.insert(expression.expression_string.end(),
                                                    it_begin,
                                                    it_copy);
                it_begin = it_copy;
                return true;
            }
            else
                //  otherwise don't forget to reset it_copy to it_begin
                it_copy = it_begin;
        }
        else if (e_three_state_result::attempt == read_op_code &&
                 size_t(it_end - it_begin) <= not_yet_parsed_size_limit)
            //  if at least one lexer returns attempt, and not one success
            //  will return attempt
            code_on_not_parsed = true;
    }
    //  will return
    //  error if all lexers return
    //      error or
    //      success without parse_helper success or
    //      junk limit is passed
    return code_on_not_parsed;
}

#else // PARSE_OLD

// readers parse
//
template <typename T_lexers,
          typename T_iterator,
          typename INDEX = std::integral_constant<size_t, 0>>
class readers
{
public:
    template <size_t not_yet_parsed_size_limit,
              size_t depth_limit,
              size_t expression_size_limit,
              size_t buffer_size_limit>
    inline static
    bool parse(expression_tree<T_lexers>& expression,
               T_iterator& it_begin,
               T_iterator const& it_end,
               expression_data<T_lexers>& read_result,
               detail::e_place_as& place_as,
               T_iterator& it_copy,
               bool& code_on_not_parsed)
    {
        using T_lexer =
            typename typelist::type_list_get<INDEX::value, typename T_lexers::list>::type;
        e_three_state_result read_op_code =
                T_lexer::template internal_read<T_iterator>(it_copy, it_end, read_result, place_as);

        if (e_three_state_result::success == read_op_code &&
            it_copy == it_begin)
        {
            assert(false);
            read_op_code = e_three_state_result::error;
        }

        if (e_three_state_result::success == read_op_code)
        {   //  try to place the operator token in the expression tree
            //  on success and checks - update it_begin and return true

            if (parse_helper(expression,
                             std::move(read_result),
                             place_as) &&
                expression.node_address.size() <= depth_limit &&
                expression.memory.size() <= expression_size_limit &&
                expression.expression_string.size() + (it_copy - it_begin) <= buffer_size_limit)
            {
                expression.expression_string.insert(expression.expression_string.end(),
                                                    it_begin,
                                                    it_copy);
                it_begin = it_copy;
                return true;
            }
            else
                //  otherwise don't forget to reset it_copy to it_begin
                it_copy = it_begin;
        }
        else if (e_three_state_result::attempt == read_op_code &&
                 size_t(it_end - it_begin) <= not_yet_parsed_size_limit)
            //  if at least one lexer returns attempt, and not one success
            //  will return attempt
            code_on_not_parsed = true;

        return readers<T_lexers,
                       T_iterator,
                       std::integral_constant<size_t, INDEX::value + 1>>::template parse<not_yet_parsed_size_limit,
                                                                                         depth_limit,
                                                                                         expression_size_limit,
                                                                                         buffer_size_limit>
                                                                                             (expression,
                                                                                              it_begin,
                                                                                              it_end,
                                                                                              read_result,
                                                                                              place_as,
                                                                                              it_copy,
                                                                                              code_on_not_parsed);
    }
};
template <typename T_lexers,
          typename T_iterator>
class readers<T_lexers,
              T_iterator,
              std::integral_constant<size_t, T_lexers::list::count>>
{
public:
    template <size_t not_yet_parsed_size_limit,
              size_t depth_limit,
              size_t expression_size_limit,
              size_t buffer_size_limit>
    inline static
    bool parse(expression_tree<T_lexers>& /*expression*/,
               T_iterator& /*it_begin*/,
               T_iterator const& /*it_end*/,
               expression_data<T_lexers>& /*read_result*/,
               e_place_as& /*place_as*/,
               T_iterator& /*it_copy*/,
               bool& code_on_not_parsed)
    {
        return code_on_not_parsed;
    }
};
#endif // PARSE_OLD

template <typename T_lexers>
inline
bool parse_helper(expression_tree<T_lexers>& expression,
                  expression_data<T_lexers>&& read_result,
                  e_place_as place_as)
{
    using expression_data = expression_data<T_lexers>;

    using default_operator_helper =
        detail::default_operator_helper<T_lexers>;

    bool success = false;

    if (e_place_as::e_discard == place_as)
        success = true; //  discard the result
    else if (expression.empty() &&
             e_place_as::e_value == place_as)
    {
        if (read_result.union_is_value)
            read_result.info_value.value_start_index = expression.expression_string.size();
        expression.create(std::move(read_result));
        success = true;
    }
    else if (false == expression.empty())
    {
        static expression_data const const_default_operator = default_operator_helper::default_operator();
        expression_data const* const_act_for_operator = nullptr;

        if (e_place_as::e_operator == place_as)
        {
            assert(false == read_result.union_is_value);

            const_act_for_operator = &read_result;
        }
        else if (size_t(default_operator_helper::default_operator_index) != T_lexers::list::count)
        {
            assert(false == const_default_operator.union_is_value);

            const_act_for_operator = &const_default_operator;
        }

        size_t const address_index_max = expression.node_address.size() - 1;
        size_t current_address_index;
        for (current_address_index = address_index_max; current_address_index <= address_index_max; --current_address_index)
        {
            if (false == expression.item2(current_address_index)->complete() ||
                (
                    const_act_for_operator &&
                    false == expression.item2(current_address_index)->union_is_value &&
                    expression.item2(current_address_index)->info_operator.priority < const_act_for_operator->info_operator.priority
                ))
                break;
        }

        if (const_act_for_operator)
        {
            if (current_address_index != address_index_max)
                ++current_address_index; // move to the child
            assert(expression.item2(current_address_index));

            //  now address at current_address_index points either to the same expression that we started with
            //  or to the weakest priority operator, that is still stronger
            //  than read_result/?default_operator but it cannot go as weak as the enclosing operator
            //  that still needs one or more arguments/keywords and is different than read_result/?default_operator

            //  now will try to scan up to see if there is an
            //  enclosing operator that needs one or more arguments/keywords and
            //  is actually the same as read_result/?default_operator
            for (size_t address_index = current_address_index; address_index <= current_address_index; --address_index)
            {
                if (false == expression.item2(address_index)->complete())
                {
                    if (expression.item2(address_index)->rtt == const_act_for_operator->rtt)
                        current_address_index = address_index;

                    break;
                }
            }

            auto current_node = expression.item2(current_address_index);

            bool fix_the_same_possible = false;
            if (current_node->rtt == const_act_for_operator->rtt &&
                false == current_node->union_is_value &&
                // this is tricky with [], for example //0 == current_node->info_operator->right && // new condition! beware, temporarily
                expression_data::property_check(current_node->info_operator.property,
                                                const_act_for_operator->info_operator.property))
                fix_the_same_possible = true;

            if (fix_the_same_possible || current_node->complete())
            {
                // make expression point to current_node if it is not
                if (&expression.item() != current_node)
                    expression.node_address.resize(current_address_index + 1);

                if (fix_the_same_possible)
                {
                    auto const& read_result_or_default_operator = *const_act_for_operator;

                    assert(false == current_node->union_is_value);

                    current_node->info_operator.right = read_result_or_default_operator.info_operator.right;
                    current_node->info_operator.property =
                        expression_data::property_combine(current_node->info_operator.property,
                                                          read_result_or_default_operator.info_operator.property);
                }
                else
                {
                    expression_data read_result_or_default_operator(e_place_as::e_operator == place_as ?
                                                                    std::move(read_result) :
                                                                    default_operator_helper::default_operator());

                    // insert?
                    read_result_or_default_operator.info_operator.last_child_address = expression.node_address[current_address_index];
                    ++read_result_or_default_operator.info_operator.children_count;

                    read_result_or_default_operator.previous_peer_address = expression.item().previous_peer_address;
                    expression.item().previous_peer_address = typename T_lexers::types::T_size_type(-1);

                    expression.node_address[current_address_index] = expression.memory.size();
                    if (current_address_index != 0)
                        expression.item2(current_address_index - 1)->info_operator.last_child_address = expression.memory.size();
                    expression.memory.push_back(std::move(read_result_or_default_operator));
                }

                success = true;
            }
        }

        if (e_place_as::e_value == place_as &&
            expression.item2(current_address_index) &&
            expression.item2(current_address_index)->info_operator.right > 0)
        {
            // current_address_index being not -1 is enough for below assert
            // thus it is safe to use expression.item2(current_address_index)->info_operator in above condition
            assert(false == expression.item2(current_address_index)->complete());

            // make expression point to current_node if it is not
            if (&expression.item() != expression.item2(current_address_index))
                expression.node_address.resize(current_address_index + 1);

            // append?
            if (read_result.union_is_value)
                read_result.info_value.value_start_index = expression.expression_string.size();

            read_result.previous_peer_address = expression.item().info_operator.last_child_address;
            expression.item().info_operator.last_child_address = expression.memory.size();
            ++expression.item().info_operator.children_count;
            expression.node_address.push_back(expression.memory.size());
            expression.memory.push_back(std::move(read_result));

            --expression.item2(current_address_index)->info_operator.right;

            success = true;
        }
    }

    return success;
}
}  //  detail

template <size_t not_yet_parsed_size_limit,
          size_t depth_limit,
          size_t expression_size_limit,
          size_t buffer_size_limit,
          typename T_iterator,
          typename T_lexers>
inline
bool parse(expression_tree<T_lexers>& expression,
           T_iterator& it_begin,
           T_iterator const& it_end)
{
#if PARSE_OLD
    return detail::parse_old<not_yet_parsed_size_limit,
                             depth_limit,
                             expression_size_limit,
                             buffer_size_limit>
                                (expression,
                                 it_begin,
                                 it_end);
#else
    if (it_begin == it_end)
        return true;

    auto read_result = detail::expression_data<T_lexers>();
    detail::e_place_as place_as;

    auto it_copy = it_begin;

    bool code_on_not_parsed = false;

    return detail::readers<T_lexers,
                           T_iterator>::template parse<not_yet_parsed_size_limit,
                                                       depth_limit,
                                                       expression_size_limit,
                                                       buffer_size_limit>
                                                (expression,
                                                 it_begin,
                                                 it_end,
                                                 read_result,
                                                 place_as,
                                                 it_copy,
                                                 code_on_not_parsed);
#endif
}

namespace detail
{
template <typename T_discard_lexer, typename T_lexers, typename T_white_space_set>
class discard_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<T_discard_lexer, typename T_lexers::list>::value };

    template <typename T_iterator>
    static inline
    bool parse(T_iterator& it_begin,
               T_iterator const& it_end)
    {
        bool code = false;
        while (it_begin != it_end &&
               T_white_space_set::value.find(*it_begin) != std::string::npos)
        {
            ++it_begin;
            code = true;
        }
        
        return code;
    }

    template <typename T_iterator>
    static
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
                                       detail::expression_data<T_lexers>& result,
                                       detail::e_place_as& place_as)
    {
        auto it_copy = it_begin;
        bool code = parse(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            //result = expression_data<T_lexers>();
            //result.rtt = T_discard_lexer::rtt;

            place_as = detail::e_place_as::e_discard;

            it_begin = it_copy;

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
    }
};

template <typename T_discard_lexer, typename T_lexers>
class discard_lexer_base_flexible
{
public:
    enum { rtt = typelist::type_list_index<T_discard_lexer, typename T_lexers::list>::value };

    template <typename T_iterator>
    static
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
                                       detail::expression_data<T_lexers>& result,
                                       detail::e_place_as& place_as)
    {
        auto it_copy = it_begin;
        bool code = T_discard_lexer::parse(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            //result = expression_data<T_lexers>();
            //result.rtt = T_discard_lexer::rtt;

            place_as = detail::e_place_as::e_discard;

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
    enum { rtt = typelist::type_list_index<T_operator_lexer, typename T_lexers::list>::value };

    template <typename T_iterator>
    static
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
                                       detail::expression_data<T_lexers>& result,
                                       detail::e_place_as& place_as)
    {
        bool place_as_operator;
        typename T_lexers::types::T_size_type right;
        typename T_lexers::types::T_property_type property;

        auto it_copy = it_begin;
        bool code = T_operator_lexer::parse(it_copy, it_end, place_as_operator, right, property);

        if (code && it_copy != it_begin)
        {
            result = detail::expression_data<T_lexers>(T_operator_lexer::rtt,
                                                       T_operator_lexer::priority,
                                                       right,
                                                       property);

            place_as = place_as_operator ?
                            detail::e_place_as::e_operator :
                            detail::e_place_as::e_value;

            it_begin = it_copy;

            return e_three_state_result::success;
        }
        else if (code)
            return e_three_state_result::attempt;
        else
            return e_three_state_result::error;
    }
};

template <typename T_value_lexer, typename T_lexers>
class value_lexer_base
{
public:
    enum { rtt = typelist::type_list_index<T_value_lexer, typename T_lexers::list>::value };

    template <typename T_iterator>
    static
    e_three_state_result internal_read(T_iterator& it_begin,
                                       T_iterator const& it_end,
                                       detail::expression_data<T_lexers>& result,
                                       detail::e_place_as& place_as)
    {
        auto it_copy = it_begin;
        bool code = T_value_lexer::parse(it_copy, it_end);

        if (code && it_copy != it_begin)
        {
            result = detail::expression_data<T_lexers>(T_value_lexer::rtt,
                                                       it_copy - it_begin);

            place_as = detail::e_place_as::e_value;

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
//  the case when these are not templetized - leads to severe segfault on app exit
//
template <typename N = void>
struct standard_white_space_set { public: static const std::string value; };
template <typename N>
std::string const standard_white_space_set<N>::value = " \t\r\n";
template <typename N = void>
struct standard_operator_set { public: static const std::string value; };
template <typename N>
std::string const standard_operator_set<N>::value = "+-*&/|\\=!$@^%,;:<>";
} // detail

template <size_t not_yet_parsed_size_limit,
          size_t depth_limit,
          size_t expression_size_limit,
          size_t buffer_size_limit,
          typename T_lexers,
          typename T_iterator>
bool parse_stream(expression_tree<T_lexers>& expression,
                  T_iterator& it_begin,
                  T_iterator const& it_end)
{
    assert(it_end != it_begin);
    
    expression.expression_string.reserve(expression.expression_string.size() + (it_end - it_begin));
    //bool was_advanced = false;

    T_iterator it_begin_track;
    while (true)
    {
        it_begin_track = it_begin;
        if (false == beltpp::parse<not_yet_parsed_size_limit,
                                   depth_limit,
                                   expression_size_limit,
                                   buffer_size_limit>
                                    (expression,
                                     it_begin,
                                     it_end))
        {
            it_begin = it_end;
            return false;
        }
        
        if (it_begin_track == it_begin)
            break;

        //was_advanced = true;
    }

    // if (was_advanced && expression.complete())
    //     expression.node_address.resize(1);

    return true;
}

template <//size_t not_yet_parsed_size_limit,
          size_t depth_limit,
          size_t expression_size_limit,
          size_t buffer_size_limit,
          typename T_lexers,
          typename T_iterator>
bool parse_full(expression_tree<T_lexers>& expression,
                T_iterator const& it_begin,
                T_iterator const& it_end)
{
    beltpp::finally guard([&expression]
    {
        if (expression.node_address.size() > 1)
            expression.node_address.resize(1);
    });

    auto it_begin_copy = it_begin;
    return parse_stream<0,
                        depth_limit,
                        expression_size_limit,
                        buffer_size_limit>
                            (expression,
                             it_begin_copy,
                             it_end) &&
           expression.complete();
}

template <typename T_iterator>
inline
bool check(T_iterator& it_begin,
           T_iterator const& it_end,
           char const* str)
{
    T_iterator it_copy = it_begin;
    for (; it_copy != it_end && *str != '\0'; ++it_copy, ++str)
    {
        if (*it_copy != *str)
            return false;
    }

    if (*str == '\0')
        it_begin = it_copy;

    return true;
}

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

template <bool stream_mode = true, typename T_iterator>
static inline
bool check_number(T_iterator& it_begin,
                  T_iterator const& it_end)
{
    // ?0.    |sign
    // ?1.    |0x
    // ? 2.   0|x
    // !3.    [sign][0x]|digits, dot
    // ! 4.   digits, ?dot|digits, ?dot
    // ?4.    |E
    // ? 5.   E|sign
    // !  6.  E[sign]|digits

    auto is_digit = [](char ch, bool hex)
    {
        if ('0' <= ch && ch <= '9')
            return true;
        if (hex && 'a' <= ch && ch <= 'f')
            return true;
        if (hex && 'A' <= ch && ch <= 'F')
            return true;
        return false;
    };

    uint8_t state = 0;
    bool hex = false;
    bool dot = false;

    auto it_copy = it_begin;
    auto it_number_end = it_begin;

    for (; it_copy != it_end; ++it_copy)
    {
        if (1 > state && ('-' == *it_copy || '+' == *it_copy))
            state = 1;
        else if (2 > state && '0' == *it_copy)
        {
            state = 2;
            it_number_end = it_copy;
            ++it_number_end;
        }
        else if (2 == state && ('x' == *it_copy || 'X' == *it_copy))
        {
            state = 3;
            hex = true;
        }
        else if (5 > state && '.' == *it_copy && false == dot)
        {
            dot = true;
            if (2 == state || 4 == state)
            {
                state = 4;
                it_number_end = it_copy;
                ++it_number_end;
            }
            else
                state = 3;
        }
        else if (5 > state && is_digit(*it_copy, hex))
        {
            state = 4;
            it_number_end = it_copy;
            ++it_number_end;
        }
        else if (4 == state && ('e' == *it_copy || 'E' == *it_copy))
        {
            assert(false == hex);
            state = 5;
        }
        else if (5 == state && ('-' == *it_copy || '+' == *it_copy))
            state = 6;
        else if ((6 == state || 5 == state) && is_digit(*it_copy, hex))
        {
            it_number_end = it_copy;
            ++it_number_end;
        }
        else
            break;
    }

    if (it_number_end == it_begin)
        return it_copy == it_end;

    if (false == stream_mode || it_copy != it_end)
        it_begin = it_number_end;
    return true;
}


}   //  end of namespace beltpp

#if 0
tests for 1313554 bytes json
1.25 Mb parsing repeated 100 times
125 Mb parsed in 0.474 seconds
it makes 264 Mb per second
    about 15x improvement for parsing only

if object reconstruction done in 2.5 seconds as has been measured before
it will make 42 Mb per second - this means at least 3x overall improvement
if object reconstruction done in 2 seconds
it will make 50 Mb per second
if object reconstruction done in 1 seconds
it will make 85 Mb per second - this will mean 6.5x overall improvement
if object reconstruction done in 0.5 seconds
it will make 125 Mb per second - this will mean about 10x overall improvement and allow 1Gbps json traffic

=============
earlier tests showed
        1.25 Mb parsed in 0.095 seconds (0.07+0.025)

        parsing only was 125 / 7 = 17.9 Mb per second
        object reconstruction was 125 / 9.5 = 13 Mb per second
=============

expr string
    dont reserve, insert - 50
    reserve, insert - 41
size checks - 15
parse helper - 306
lexers - 112

474 = 306 + 112 + 41 + 15
and reserving the memory allows -20, to get about 461


full flow time 483
    parse stream costs 6445
    parse stream self costs 2757
    unknown costs 392
    parse helper self costs 3295

full flow / expression string reserve time 474
    parse stream costs 6333
    parse stream self costs 2756
    unknown costs 281
    parse helper self costs 3294

full flow / expression string 0/0 time 433
    parse stream costs 5716
    parse stream self costs 2421
    unknown costs 0
    parse helper self costs 3294

full flow / expression string reserve, no size check time 459
    parse stream costs 6079
    parse stream self costs 2503
    unknown costs 281
    parse helper self costs 3294

full flow / no size check, no expression string, no reserve time 418
    parse stream costs 5462
    parse stream self costs 2167
    unknown costs 0
    parse helper self costs 3295

full flow / no size check, no parser, no expression string, no reserve time 112
    parse stream costs 1719
    parse stream self costs 1719
    unknown costs 0
    parse helper self costs 0

###### below is obsolete code

class memory_wrapper2
{
    static const size_t item_size = 8192;
    std::vector<std::vector<detail::expression_data<T_lexers>>> memory;
public:
//        memory_wrapper2()
//        {
//            memory.resize(1);
//            memory.back().reserve(item_size);
//        }
    inline detail::expression_data<T_lexers>& operator[](size_t index)  noexcept
    {
        return memory[index / item_size][index % item_size];
    }
    inline detail::expression_data<T_lexers> const& operator[](size_t index) const noexcept
    {
        return memory[index / item_size][index % item_size];
    }
    inline size_t size() const noexcept
    {
        return memory.empty() ? 0 : item_size * (memory.size() - 1) + memory.back().size();
        //return item_size * (memory.size() - 1) + memory.back().size();
    }
    inline bool empty() const noexcept
    {
        return memory.empty();
        //return memory.size() == 1 && memory.back().empty();
    }
    inline void push_back(detail::expression_data<T_lexers>&& item)
    {
        if (memory.empty() || memory.back().size() == item_size)
        //if (memory.back().size() == item_size)
        {
            memory.push_back(std::vector<detail::expression_data<T_lexers>>());
            memory.back().reserve(item_size);
        }
        return memory.back().emplace_back(std::move(item));
    }
    inline void reserve(size_t size)
    {
        memory.reserve(size / item_size);
    }
};
class memory_wrapper3
{
    std::vector<std::vector<detail::expression_data<T_lexers>>> memory;
public:
    inline detail::expression_data<T_lexers>& operator[](size_t index)  noexcept
    {
        size_t fill_size = 1;
        for (size_t n = 1; n != 63; ++n)
        {
            if (index < ((fill_size << 1) - 1))
                return memory[n - 1][index - (fill_size - 1)];
            fill_size = fill_size << 1;
        }
        std::terminate();
    }
    inline detail::expression_data<T_lexers> const& operator[](size_t index) const noexcept
    {
        size_t fill_size = 1;
        for (size_t n = 1; n != 63; ++n)
        {
            if (index < ((fill_size << 1) - 1))
                return memory[n - 1][index - (fill_size - 1)];
            fill_size = fill_size << 1;
        }
        std::terminate();
    }
    inline size_t size() const noexcept
    {
        if (memory.empty())
            return 0;
        if (memory.size() == 1)
            return memory.back().size();
        size_t fill_size = 1;
        for (size_t n = 1; n != memory.size() - 1; ++n)
            fill_size = fill_size << 1;
        return (fill_size << 1) - 1 + memory.back().size();
    }
    inline bool empty() const noexcept
    {
        return memory.empty();
    }
    inline void push_back(detail::expression_data<T_lexers>&& item)
    {
        if (memory.empty())
        {
            memory.push_back(std::vector<detail::expression_data<T_lexers>>());
            memory.back().push_back(std::move(item));
            return;
        }
        size_t limit = 1;
        for (size_t n = 1; n != memory.size(); ++n)
            limit = limit << 1;
        if (memory.back().size() == limit)
        {
            memory.push_back(std::vector<detail::expression_data<T_lexers>>());
            memory.back().reserve(limit << 1);
        }
        return memory.back().emplace_back(std::move(item));
    }
    inline void reserve(size_t size)
    {
    }
};
#endif

