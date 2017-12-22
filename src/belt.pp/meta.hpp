#pragma once

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
}   //  end namespace typelist

#define DECLARE_MF_INSPECTION(mf_name, _T, mf_signature)                \
template <typename T>                                                   \
class has_ ## mf_name                                                   \
{                                                                       \
template <size_t N>                                                     \
using charray = char[N];                                                \
protected:                                                              \
    template <typename  _T>                                             \
    using TTmember = mf_signature;                                      \
                                                                        \
    template <typename TT, TTmember<TT> fptr = &TT::mf_name>            \
    static charray<1>& test(TT*);                                       \
    template <typename = T>                                             \
    static charray<2>& test(...);                                       \
public:                                                                 \
    enum {value = sizeof(test(static_cast<T*>(nullptr))) == 1 ? 1 : 0}; \
};                                                                      \

using test_type_list = typelist::type_list<int, char, class test_type>;
static_assert(typelist::type_list_index<int, test_type_list>::value ==
              0, "type list check");
static_assert(typelist::type_list_index<char, test_type_list>::value ==
              1, "type list check");
static_assert(typelist::type_list_index<class test_type, test_type_list>::value ==
              2, "type list check");
//
//  the following one should be error
/*static_assert(typelist::type_list_index<double, test_type_list>::value ==
              3, "type list check");*/

}
