#include "utility.hpp"

#include <random>
#include <ctime>
#include <cassert>
#include <cstdio>
#include <exception>
#include <stdexcept>

namespace
{
std::string format_tm(std::tm const& t)
{
    char buffer[80];
    if (0 == strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &t))
    {
        assert(false);
        throw std::logic_error("format_tm");
    }

    return std::string(buffer);
}

bool scan_tm(std::string const& strt, std::tm& st)
{
    if (6 !=
    #ifdef B_OS_WINDOWS
        sscanf_s(
    #else
        sscanf(
    #endif
            strt.c_str(),
            "%d-%d-%d %d:%d:%d",
            &st.tm_year,
            &st.tm_mon,
            &st.tm_mday,
            &st.tm_hour,
            &st.tm_min,
            &st.tm_sec))
        return false;

    st.tm_year -= 1900;
    --st.tm_mon;

    if (format_tm(st) != strt)
        return false;

    return true;
}
}

namespace beltpp
{
tm gm_time_t_to_gm_tm(time_t t)
{
    tm result;
#if defined B_OS_WINDOWS
    gmtime_s(&result, &t);
#else
    gmtime_r(&t, &result);
#endif
    return result;
}
tm gm_time_t_to_lc_tm(time_t t)
{
    tm result;
#if defined B_OS_WINDOWS
    localtime_s(&result, &t);
#else
    localtime_r(&t, &result);
#endif
    return result;
}
time_t gm_tm_to_gm_time_t(std::tm const& t)
{
    std::tm temp = t;
#if defined B_OS_WINDOWS
    return _mkgmtime(&temp);
#else
    return timegm(&temp);
#endif
}
time_t lc_tm_to_gm_time_t(std::tm const& t)
{
    std::tm temp = t;
    return mktime(&temp);
}
std::string gm_time_t_to_gm_string(time_t t)
{
    std::tm st = gm_time_t_to_gm_tm(t);
    return format_tm(st);
}
std::string gm_time_t_to_lc_string(time_t t)
{
    std::tm st = gm_time_t_to_lc_tm(t);
    return format_tm(st);
}
bool gm_string_to_gm_time_t(std::string const& strt, time_t& t)
{
    std::tm st;
    if (false == scan_tm(strt, st))
        return false;

    t = gm_tm_to_gm_time_t(st);
    return true;
}
bool lc_string_to_gm_time_t(std::string const& strt, time_t& t)
{
    std::tm st;
    if (false == scan_tm(strt, st))
        return false;

    t = lc_tm_to_gm_time_t(st);
    return true;
}

double random_in_range(double from, double to)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(from, to);

    return dist(mt);
}

bool chance_one_of(uint32_t count)
{
    if (0 == count)
        return false;
    if (1 == count)
        return true;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(0, count - 1);

    if (0 == dist(mt))
        return true;
    return false;
}
}


