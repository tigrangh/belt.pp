#pragma once

#include <belt.pp/idl_parser.hpp>

#include <string>
#include <unordered_map>

class idl_parser_types
{
public:
    using T_rtt_type = uint8_t;
    using T_priority_type = uint8_t;
    using T_size_type = uint8_t;
    using T_property_type = uint8_t;
    using T_string_type = std::string;
};

using expression_tree = beltpp::expression_tree<lexers<idl_parser_types>, idl_parser_types>;
using expression_tree_pointer = beltpp::expression_tree_pointer<lexers<idl_parser_types>, idl_parser_types>;

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
