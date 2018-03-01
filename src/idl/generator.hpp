#pragma once

#include "parser.hpp"

#include <string>
#include <unordered_map>

using expression_tree = beltpp::expression_tree<lexers, std::string>;

class state_holder
{
public:
    state_holder();
    std::unordered_map<std::string, std::string> const map_types;
};

std::string analyze(state_holder& state,
                    expression_tree const* pexpression);

std::string analyze_class(state_holder& state,
                          expression_tree const* pexpression,
                          size_t rtt,
                          std::string& class_name);
