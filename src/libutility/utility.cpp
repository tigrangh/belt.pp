#include "utility.hpp"

#include <sstream>
#include <iomanip>

namespace beltpp
{
tm gm_time_t_to_gm_tm(time_t t)
{
    tm result;
#if defined B_OS_WIN
    gmtime_s(&t, &result);
#else
    gmtime_r(&t, &result);
#endif
    return result;
}
tm gm_time_t_to_lc_tm(time_t t)
{
    tm result;
#if defined B_OS_WIN
    localtime_s(&t, &result);
#else
    localtime_r(&t, &result);
#endif
    return result;
}
time_t gm_tm_to_gm_time_t(std::tm const& t)
{
    std::tm temp = t;
#if defined B_OS_WIN
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
    std::stringstream ss;

    ss << std::put_time(&st, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}
std::string gm_time_t_to_lc_string(time_t t)
{
    std::tm st = gm_time_t_to_lc_tm(t);
    std::stringstream ss;

    ss << std::put_time(&st, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}
bool gm_string_to_gm_time_t(std::string const& strt, time_t& t)
{
    std::tm st;
    std::stringstream ss(strt);

    ss >> std::get_time(&st, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
        return false;

    t = gm_tm_to_gm_time_t(st);
    return true;
}
bool lc_string_to_gm_time_t(std::string const& strt, time_t& t)
{
    std::tm st;
    std::stringstream ss(strt);

    ss >> std::get_time(&st, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
        return false;

    t = lc_tm_to_gm_time_t(st);
    return true;
}
}


