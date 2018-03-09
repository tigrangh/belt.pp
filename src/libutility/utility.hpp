#pragma once

#include "global.hpp"
#include <ctime>

namespace beltpp
{
std::tm UTILITYSHARED_EXPORT gm_time_t_to_gm_tm(time_t t);
std::tm UTILITYSHARED_EXPORT gm_time_t_to_lc_tm(time_t t);
time_t UTILITYSHARED_EXPORT gm_tm_to_gm_time_t(std::tm const& t);
time_t UTILITYSHARED_EXPORT lc_tm_to_gm_time_t(std::tm const& t);
std::string UTILITYSHARED_EXPORT gm_time_t_to_gm_string(time_t t);
std::string UTILITYSHARED_EXPORT gm_time_t_to_lc_string(time_t t);
bool UTILITYSHARED_EXPORT gm_string_to_gm_time_t(std::string const& strt, time_t& t);
bool UTILITYSHARED_EXPORT lc_string_to_gm_time_t(std::string const& strt, time_t& t);
}
