#pragma once

#include <belt.pp/idl_parser.hpp>

#include <string>
#include <unordered_map>

using expression_tree = beltpp::expression_tree<lexers, std::string>;
using expression_tree_pointer = beltpp::expression_tree_pointer<lexers, std::string>;

class state_holder
{
public:
    state_holder();
    std::string optional_type;
    std::string namespace_name;
    std::unordered_map<std::string, std::string> map_types;
};

struct generated_code
{
    std::string declarations;
    std::string template_definitions;
    std::string definitions;
};

generated_code analyze(state_holder& state,
                       expression_tree const* pexpression);

generated_code analyze_struct(state_holder& state,
                              expression_tree const* pexpression,
                              size_t rtt,
                              std::string const& type_name,
                              std::unordered_multimap<std::string, std::string>& dependencies,
                              std::string& class_members);

generated_code analyze_enum(state_holder& state,
                            expression_tree const* pexpression,
                            std::string const& type_name,
                            std::string& enum_members);
