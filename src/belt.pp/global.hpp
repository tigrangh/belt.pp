#pragma once

#if defined _MSC_VER

#define B_OS_WIN

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

namespace beltpp
{
enum class e_three_state_result {success, attempt, error};

namespace detail
{
using fptr_deleter = void(*)(void*&);
}
using void_unique_ptr = std::unique_ptr<void, detail::fptr_deleter>;

template <typename T>
inline void_unique_ptr new_void_unique_ptr()
{
    void_unique_ptr result(nullptr,
                           [](void* &p)
                            {
                                T* pmc = static_cast<T*>(p);
                                delete pmc;
                                p = nullptr;
                            });

    result.reset(new T());
    return result;
}

template <typename T>
inline void_unique_ptr new_void_unique_ptr(void const* pother)
{
    void_unique_ptr result(nullptr,
                           [](void* &p)
                            {
                                T* pmc = static_cast<T*>(p);
                                delete pmc;
                                p = nullptr;
                            });

    T const& other = *static_cast<T const*>(pother);
    result.reset(new T(other));
    return result;
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
    int32_t result = temp;
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
    uint32_t result = temp;
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
    int16_t result = temp;
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
    uint16_t result = temp;
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
    int8_t result = temp;
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
    uint8_t result = temp;
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

