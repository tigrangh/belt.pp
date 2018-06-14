#pragma once

#if defined _MSC_VER

#define B_OS_WINDOWS

#define UNICODE
#define _UNICODE

#define BELT_EXPORT     __declspec(dllexport)
#define BELT_IMPORT     __declspec(dllimport)
#elif defined __APPLE__

#define B_OS_MACOS

#define BELT_EXPORT
#define BELT_IMPORT

#else

#define B_OS_LINUX

#define BELT_EXPORT
#define BELT_IMPORT
#endif

#include <cstdlib>
#include <string>
#include <memory>

#define B_UNUSED(x) (void)x;

namespace beltpp
{
enum class e_three_state_result {success, attempt, error};

namespace detail
{
template <typename T>
using fptr_deleter = void(*)(T*);
}

template <typename T>
using t_unique_ptr = std::unique_ptr<T, detail::fptr_deleter<T>>;
using void_unique_ptr = t_unique_ptr<void>;

template <typename T1, typename T2, typename... Ts>
inline t_unique_ptr<T1> new_dc_unique_ptr(Ts... args)
{
    t_unique_ptr<T1> result(nullptr,
                            [](T1* p)
                            {
                                T2* pmc = dynamic_cast<T2*>(p);
                                delete pmc;
                            });

    result.reset(dynamic_cast<T1*>(new T2(args...)));
    return result;
}
template <typename T1, typename T2>
inline t_unique_ptr<T1> new_dc_unique_ptr_copy(T1 const* pother)
{
    t_unique_ptr<T1> result(nullptr,
                            [](T1* p)
                            {
                                T2* pmc = dynamic_cast<T2*>(p);
                                delete pmc;
                            });

    T2 const& other = *dynamic_cast<T2 const*>(pother);
    result.reset(dynamic_cast<T1*>(new T2(other)));
    return result;
}

template <typename T1, typename T2, typename... Ts>
inline t_unique_ptr<T1> new_sc_unique_ptr(Ts... args)
{
    t_unique_ptr<T1> result(nullptr,
                            [](T1* p)
                            {
                                T2* pmc = static_cast<T2*>(p);
                                delete pmc;
                            });

    result.reset(static_cast<T1*>(new T2(args...)));
    return result;
}
template <typename T1, typename T2>
inline t_unique_ptr<T1> new_sc_unique_ptr_copy(void const* pother)
{
    t_unique_ptr<T1> result(nullptr,
                            [](T1* p)
                            {
                                T2* pmc = static_cast<T2*>(p);
                                delete pmc;
                            });

    T2 const& other = *static_cast<T2 const*>(pother);
    result.reset(static_cast<T1*>(new T2(other)));
    return result;
}

template <typename T, typename... Ts>
inline void_unique_ptr new_void_unique_ptr(Ts... args)
{
    return new_sc_unique_ptr<void, T>(args...);
}

template <typename T>
inline void_unique_ptr new_void_unique_ptr_copy(void const* pother)
{
    return new_sc_unique_ptr_copy<void, T>(pother);
}

inline float stof(std::string const& value, size_t& pos)
{
    float result = 0;
    char const* sz = value.c_str();
    char* endptr = nullptr;
    result = std::strtof(sz, &endptr);
    pos = endptr - sz;

    return result;
}
inline double stod(std::string const& value, size_t& pos)
{
    double result = 0;
    char const* sz = value.c_str();
    char* endptr = nullptr;
    result = std::strtod(sz, &endptr);
    pos = endptr - sz;

    return result;
}
inline int64_t stoi64(std::string const& value, size_t& pos)
{
    static_assert(sizeof(int64_t) == sizeof(long long), "check type sizes");
    int64_t result = 0;
    char const* sz = value.c_str();
    char* endptr = nullptr;
    result = std::strtoll(sz, &endptr, 0);
    pos = endptr - sz;

    return result;
}
inline uint64_t stoui64(std::string const& value, size_t& pos)
{
    static_assert(sizeof(uint64_t) == sizeof(unsigned long long), "check type sizes");
    uint64_t result = 0;
    char const* sz = value.c_str();
    char* endptr = nullptr;
    result = std::strtoull(sz, &endptr, 0);
    pos = endptr - sz;

    return result;
}
inline int32_t stoi32(std::string const& value, size_t& pos)
{
    int64_t temp = stoi64(value, pos);
    int32_t result = int32_t(temp);
    if (int64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}
inline uint32_t stoui32(std::string const& value, size_t& pos)
{
    uint64_t temp = stoui64(value, pos);
    uint32_t result = uint32_t(temp);
    if (uint64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}
inline int16_t stoi16(std::string const& value, size_t& pos)
{
    int64_t temp = stoi64(value, pos);
    int16_t result = int16_t(temp);
    if (int64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}
inline uint16_t stoui16(std::string const& value, size_t& pos)
{
    uint64_t temp = stoui64(value, pos);
    uint16_t result = uint16_t(temp);
    if (uint64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}
inline int8_t stoi8(std::string const& value, size_t& pos)
{
    int64_t temp = stoi64(value, pos);
    int8_t result = int8_t(temp);
    if (int64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}
inline uint8_t stoui8(std::string const& value, size_t& pos)
{
    uint64_t temp = stoui64(value, pos);
    uint8_t result = uint8_t(temp);
    if (uint64_t(result) != temp)
    {
        pos = 0;
        result = 0;
    }

    return result;
}

template <typename Tself, typename Tother>
inline void assign(Tself& self, Tother const& other)
{
    self = static_cast<Tself>(other);
}
}

