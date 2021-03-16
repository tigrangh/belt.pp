#pragma once

#include <cstddef>

namespace beltpp
{
namespace typelist
{
template <typename T1, typename T2>
class two_types_same
{
public:
    enum {value = false};
};

template <typename T>
class two_types_same<T, T>
{
public:
    enum {value = true};
};

template <typename... Ts>
class type_list
{
public:
    enum {count = sizeof...(Ts)};
};

template <typename Tfind, typename...>
class type_list_index;

template <typename Tfind,
          template <typename...> class Tlist,
          typename... Ts>
class type_list_index<Tfind, Tlist<Tfind, Ts...>>
{
public:
    enum {value = 0};
};

template <typename Tfind,
          template <typename...> class Tlist,
          typename T0,
          typename... Ts>
class type_list_index<Tfind, Tlist<T0, Ts...>>
{
public:
    enum {value = 1 + type_list_index<Tfind, Tlist<Ts...>>::value};
};

template <size_t INDEX, typename...>
class type_list_get;

template <template <typename...> class Tlist,
          typename Tfirst,
          typename... Ts>
class type_list_get<0, Tlist<Tfirst, Ts...>>
{
public:
    using type = Tfirst;
};

template <size_t INDEX,
          template <typename...> class Tlist,
          typename Tfirst,
          typename... Ts>
class type_list_get<INDEX, Tlist<Tfirst, Ts...>>
{
public:
    using type = typename type_list_get<INDEX - 1, Tlist<Ts...>>::type;
};
} //  end namespace typelist

namespace static_set
{
template <int64_t... values>
class set
{
public:
    enum {count = sizeof...(values)};
};

template <int64_t Vfind,
          typename Tset,
          typename...>
class index;

template <int64_t Vfind,
          template <int64_t...> class Tset>
class index<Vfind, Tset<>>
{
public:
    enum {value = -1};
};

template <int64_t Vfind,
          template <int64_t...> class Tset,
          int64_t... Vvalues>
class index<Vfind, Tset<Vfind, Vvalues...>>
{
public:
    enum {value = 0};
};

template <int64_t Vfind,
          template <int64_t...> class Tset,
          int64_t Vvalue0,
          int64_t... Vvalues>
class index<Vfind, Tset<Vvalue0, Vvalues...>>
{
public:
    enum {next_value = index<Vfind, Tset<Vvalues...>>::value};
    enum {value = next_value == -1 ? -1 : 1 + next_value};
};

template <typename Tset,
          typename...>
class find;

template <template <int64_t...> class Tset>
class find<Tset<>>
{
public:
    static int64_t check(int64_t)
    {
        return -1;
    }
};

template <template <int64_t...> class Tset,
          int64_t Vvalue0,
          int64_t... Vvalues>
class find<Tset<Vvalue0, Vvalues...>>
{
public:
    static int64_t check(int64_t value)
    {
        if (value == Vvalue0)
            return 0;

        int64_t next_check = find<Tset<Vvalues...>>::check(value);
        return next_check == -1 ? -1 : 1 + next_check;
    }
};
} //  end namespace static_set

#define DECLARE_INTEGER_INSPECTION(int_name)                                \
template <typename T>                                                       \
class has_integer_ ## int_name                                              \
{                                                                           \
template <size_t N>                                                         \
using charray = char[N];                                                    \
protected:                                                                  \
    template <typename TT, size_t check_stn = TT::int_name>                 \
    static charray<1>& test(TT*);                                           \
    template <typename = T>                                                 \
    static charray<2>& test(...);                                           \
public:                                                                     \
    enum {value = sizeof(test(static_cast<T*>(nullptr))) == 1 ? 1 : 0};     \
};                                                                          \

#define DECLARE_TD_INSPECTION(typedef_name)                                 \
template <typename T>                                                       \
class has_type_definition_ ## typedef_name                                  \
{                                                                           \
template <size_t N>                                                         \
using charray = char[N];                                                    \
protected:                                                                  \
    template <typename TT, typename check_td = typename TT::typedef_name>   \
    static charray<1>& test(TT*);                                           \
    template <typename = T>                                                 \
    static charray<2>& test(...);                                           \
public:                                                                     \
    enum {value = sizeof(test(static_cast<T*>(nullptr))) == 1 ? 1 : 0};     \
};                                                                          \

#define DECLARE_MF_INSPECTION(mf_name, _T, mf_signature)                    \
template <typename T>                                                       \
class has_ ## mf_name                                                       \
{                                                                           \
template <size_t N>                                                         \
using charray = char[N];                                                    \
protected:                                                                  \
    template <typename  _T>                                                 \
    using TTmember = mf_signature;                                          \
                                                                            \
    template <typename TT, TTmember<TT> fptr = &TT::mf_name>                \
    static charray<1>& test(TT*);                                           \
    template <typename = T>                                                 \
    static charray<2>& test(...);                                           \
public:                                                                     \
    enum {value = sizeof(test(static_cast<T*>(nullptr))) == 1 ? 1 : 0};     \
};                                                                          \

using test_type_list = typelist::type_list<int, char, class test_type>;
static_assert((typelist::type_list_index<int, test_type_list>::value ==
              0), "type list check");
static_assert((typelist::type_list_index<char, test_type_list>::value ==
              1), "type list check");
static_assert((typelist::type_list_index<class test_type, test_type_list>::value ==
              2), "type list check");
//
//  the following one should be error
/*static_assert(typelist::type_list_index<double, test_type_list>::value ==
              3, "type list check");*/

}
