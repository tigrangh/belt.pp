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
inline void_unique_ptr make_void_unique_ptr()
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
inline void_unique_ptr make_void_unique_ptr(T const& other)
{
    void_unique_ptr result(nullptr,
                           [](void* &p)
                            {
                                T* pmc = static_cast<T*>(p);
                                delete pmc;
                                p = nullptr;
                            });

    result.reset(new T(other));
    return result;
}

inline double stod(std::string const& value, size_t& pos)
{
    double result;
    try
    {
        result = std::stod(value, &pos);
    }
    catch(...)
    {
        pos = 0;
    }

    return result;
}
inline int64_t stoll(std::string const& value, size_t& pos)
{
    int64_t result = 0;
    try
    {
        result = std::stoll(value, &pos);
    }
    catch(...)
    {
        pos = 0;
    }

    return result;
}
inline uint64_t stoull(std::string const& value, size_t& pos)
{
    uint64_t result = 0;
    try
    {
        result = std::stoull(value, &pos);
    }
    catch(...)
    {
        pos = 0;
    }

    return result;
}

template <typename Tself, typename Tother>
inline void assign(Tself& self, Tother const& other)
{
    self = static_cast<Tself>(other);
}
}

