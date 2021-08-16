#pragma once

#include "global.hpp"

#include <belt.pp/ilog.hpp>

#include <string>

namespace beltpp
{
LOGSHARED_EXPORT ilog_ptr console_logger(std::string const& name, bool print_time);
LOGSHARED_EXPORT ilog_ptr file_logger(std::string const& name, std::string const& fname);
}
