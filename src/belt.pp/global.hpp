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

namespace beltpp
{
enum class e_three_state_result {success, attempt, error};

double stod(std::string const& value, size_t& pos)
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
int64_t stoll(std::string const& value, size_t& pos)
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
uint64_t stoull(std::string const& value, size_t& pos)
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
}

