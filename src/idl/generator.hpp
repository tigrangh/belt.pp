#pragma once

#include "parser.hpp"

#include <string>

using expression_tree = beltpp::expression_tree<lexers, std::string>;

std::string analyze(expression_tree const* pexpression);

std::string analyze_class(expression_tree const* pexpression,
                          size_t rtt,
                          std::string& class_name);
