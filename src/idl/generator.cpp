#include "generator.hpp"

#include <cassert>
#include <vector>
#include <exception>
#include <utility>

using std::string;
using std::vector;
using std::pair;
using std::runtime_error;

state_holder::state_holder()
    : namespace_name()
    , map_types{{"string", "std::string"},
                {"bool", "bool"},
                {"int8", "int8_t"},
                {"uint8", "uint8_t"},
                {"int16", "int16_t"},
                {"uint16", "uint16_t"},
                {"int", "uint32_t"},
                {"int32", "int32_t"},
                {"uint32", "uint32_t"},
                {"int64", "int64_t"},
                {"uint64", "uint64_t"},
                {"float32", "float"},
                {"float64", "double"}}
{
}

namespace
{

string convert_type (string const& type_name, state_holder& state)
{
    auto it_type_name = state.map_types.find(type_name);
    if (it_type_name != state.map_types.end())
        return it_type_name->second;
    return type_name;
};

string construct_type_name (expression_tree const* member_type, state_holder& state)
{
    if (member_type->lexem.rtt == identifier::rtt)
        return convert_type(member_type->lexem.value, state);
    else if (member_type->lexem.rtt == scope_bracket::rtt &&
             member_type->children.size() == 1 &&
             member_type->children.front()->lexem.rtt == identifier::rtt)
    {
        string type_name = convert_type(member_type->children.front()->lexem.value,
                                        state);
        return "std::vector<" + type_name + ">";
    }
    else if (member_type->lexem.rtt == keyword_map::rtt &&
             member_type->children.size() == 1 &&
             member_type->children.front()->lexem.rtt == scope_bracket::rtt &&
             member_type->children.front()->children.size() == 2)
    {
        string key_type_name =
                construct_type_name(member_type->children.front()->children.front(),
                                    state);
        string value_type_name =
                construct_type_name(member_type->children.front()->children.back(),
                                    state);
        return "std::unordered_map<" + key_type_name  + ", " + value_type_name + ">";
    }
    else
        throw runtime_error("can't get type definition, wtf!");
}
}
string analyze(state_holder& state,
               expression_tree const* pexpression)
{
    size_t rtt = 0;
    string result;
    assert(pexpression);

    vector<string> class_names;

    if (pexpression->lexem.rtt != operator_semicolon::rtt ||
        pexpression->children.empty() ||
        pexpression->children.front()->lexem.rtt != keyword_package::rtt)
        throw runtime_error("wtf");
    else
    {
        string package_name;
        for (auto item : pexpression->children)
        {
            if (item->lexem.rtt == keyword_package::rtt)
            {
                if (item->children.size() != 1 ||
                    item->children.front()->lexem.rtt != identifier::rtt)
                    throw runtime_error("use \"package name\"");
                string get_package_name = item->children.front()->lexem.value;
                if (package_name.empty())
                    package_name = get_package_name;
                else
                    throw runtime_error("can specify the package name only once");
            }
            else if (item->lexem.rtt == keyword_type::rtt)
            {
                if (item->children.size() != 2 ||
                    item->children.front()->lexem.rtt != identifier::rtt)
                    throw runtime_error("type syntax is wrong");

                string type_name = item->children.front()->lexem.value;
                auto pexpression_type = item->children.back();

                if (pexpression_type->lexem.rtt == keyword_struct::rtt)
                {
                    if (pexpression_type->children.size() != 1 ||
                        pexpression_type->children.front()->lexem.rtt !=
                            scope_brace::rtt)
                        throw runtime_error("struct syntax error");

                    state.namespace_name = package_name;
                    result += analyze_struct(state,
                                             pexpression_type->children.front(),
                                             rtt++,
                                             type_name);
                    class_names.push_back(type_name);
                }
                else
                    throw runtime_error("type syntax error");
            }
        }
    }

    if (class_names.empty())
        throw runtime_error("wtf, nothing to do");

    result += "namespace detail\n";
    result += "{\n";
    result += "bool analyze_json_object(beltpp::json::expression_tree* pexp,\n";
    result += "                         beltpp::detail::pmsg_all& return_value,\n";
    result += "                         utils const& utl)\n";
    result += "{\n";
    result += "    bool code = false;\n";
    result += "    switch (return_value.rtt)\n";
    result += "    {\n";
    for (auto const& class_name : class_names)
    {
    result += "    case " + class_name + "::rtt:\n";
    result += "    {\n";
    result += "        return_value =\n";
    result += "                beltpp::detail::pmsg_all(   " + class_name + "::rtt,\n";
    result += "                                            ::beltpp::make_void_unique_ptr<" + class_name + ">(),\n";
    result += "                                            &" + class_name + "::saver);\n";
    result += "        " + class_name + "* pmsgcode =\n";
    result += "                static_cast<" + class_name + "*>(return_value.pmsg.get());\n";
    result += "        code = analyze_json(*pmsgcode, pexp, utl);\n";
    result += "    }\n";
    result += "        break;\n";
    }
    result += "    default:\n";
    result += "        code = false;\n";
    result += "        break;\n";
    result += "    }\n";
    result += "\n";
    result += "    return code;\n";
    result += "}\n";
    result += "}\n";

    return result;
}

string analyze_struct(state_holder& state,
                      expression_tree const* pexpression,
                      size_t rtt,
                      string const& type_name)
{
    if (state.namespace_name.empty())
        throw runtime_error("please specify package name");
    string result;
    assert(pexpression);

    result += "class " + type_name + ";\n";
    result += "namespace detail\n";
    result += "{\n";
    result += "std::string saver(" + type_name + " const& self);\n";
    result += "}\n";

    result += "class " + type_name + "\n";
    result += "{\npublic:\n";

    vector<pair<expression_tree const*, expression_tree const*>> members;

    if (pexpression->children.size() % 2 != 0)
        throw runtime_error("inside class syntax error, wtf");

    auto it = pexpression->children.begin();
    for (size_t index = 0;
         it != pexpression->children.end();
         ++index, ++it)
    {
        auto const* member_name = *it;
        ++it;
        auto const* member_type = *it;

        if (member_name->lexem.rtt != identifier::rtt)
            throw runtime_error("inside class syntax error, wtf");

        members.push_back(std::make_pair(member_name, member_type));
    }

    result += "    enum {rtt = " + std::to_string(rtt) + "};\n";

    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;
        auto const& member_type = member_pair.second;
        if (member_name.rtt != identifier::rtt)
            throw runtime_error("use \"variable type\" syntax please");

        result += "    " + construct_type_name(member_type, state) + " " +
                member_name.value + ";\n";
    }

    {
    bool first = true;
    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;
        if (first)
            result += "    " + type_name + "()\n"
                    "      : " + member_name.value + "()\n";
        else
            result += "      , " + member_name.value + "()\n";

        first = false;
    }
    }
    if (false == members.empty())
        result += "    {}\n";

    if (false == members.empty())
    {
        result += "    template <typename T>\n";
        result += "    explicit " + type_name + "(T const& other)\n";
        result += "    {\n";
        result += "        ::beltpp::assign(*this, other);\n";
        result += "    }\n";

        result += "    template <typename T>\n";
        result += "    " + type_name + "& operator = (T const& other)\n";
        result += "    {\n";
        result += "        ::beltpp::assign(*this, other);\n";
        result += "        return *this;\n";
        result += "    }\n";
    }

    result += "    bool operator == (" + type_name + " const& other) const\n";
    result += "    {\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result += "        if (" + member_name.value + " != other." + member_name.value + ")\n";
    result += "            return false;\n";
    }
    result += "        return true;\n";
    result += "    }\n";

    result += "    bool operator != (" + type_name + " const& other) const\n";
    result += "    {\n";
    result += "        return false == (operator == (other));\n";
    result += "    }\n";

    result += "    bool operator < (" + type_name + " const& other) const\n";
    result += "    {\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result += "        if (false == detail::less(" + member_name.value + ", other." + member_name.value + "))\n";
    result += "            return false;\n";
    }
    if (false == members.empty())
    result += "        return true;\n";
    else
    result += "        return false;\n";
    result += "    }\n";

    result += "    bool operator > (" + type_name + " const& other) const\n";
    result += "    {\n";
    result += "        return other < *this;\n";
    result += "    }\n";

    result += "    static std::vector<char> saver(void* p)\n";
    result += "    {\n";
    result += "        " + type_name + "* pmc = static_cast<" + type_name + "*>(p);\n";
    result += "        std::vector<char> result;\n";
    result += "        std::string str_value = detail::saver(*pmc);\n";
    result += "        std::copy(str_value.begin(), str_value.end(), std::back_inserter(result));\n";
    result += "        return result;\n";
    result += "    }\n";

    result += "};  // end of class\n";

    result += "}   // end of namespace " + state.namespace_name + "\n";

    result += "namespace beltpp\n";
    result += "{\n";
    if (false == members.empty())
    {
        result += "template <typename T>\n";
        result += "void assign(" + state.namespace_name + "::" + type_name + "& self, T const& other)\n";
        result += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        result += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";

        result += "template <typename T>\n";
        result += "void assign(T& self, " + state.namespace_name + "::" + type_name + " const& other)\n";
        result += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        result += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";
    }
    result += "}   // end of namespace beltpp\n";

    result += "namespace " + state.namespace_name + "\n";
    result += "{\n";
    result += "namespace detail\n";
    result += "{\n";
    result += "bool analyze_json(" + type_name + "& msgcode,\n";
    result += "                  beltpp::json::expression_tree* pexp,\n";
    result += "                  utils const& utl)\n";
    result += "{\n";
    result += "    bool code = true;\n";
    result += "    std::unordered_map<std::string, beltpp::json::expression_tree*> members;\n";
    result += "    size_t rtt = -1;\n";
    result += "    if (false == analyze_json_common(rtt, pexp, members) ||\n";
    result += "        rtt != " + type_name + "::rtt)\n";
    result += "        code = false;\n";
    result += "    else\n";
    result += "    {\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result += "        if (code)\n";
    result += "        {\n";
    result += "            auto it_find = members.find(\"\\\"" + member_name.value + "\\\"\");\n";
    result += "            if (it_find != members.end())\n";
    result += "            {\n";
    result += "                beltpp::json::expression_tree* item = it_find->second;\n";
    result += "                assert(item);\n";
    result += "                code = analyze_json(msgcode." + member_name.value + ", item, utl);\n";
    result += "            }\n";
    result += "        }\n";
    }
    result += "    }\n";
    result += "    return code;\n";
    result += "}\n";

    result += "std::string saver(" + type_name + " const& self)\n";
    result += "{\n";
    result += "    std::string result;\n";
    result += "    result += \"{\";\n";
    result += "    result += \"\\\"rtt\\\":\" + saver(" + type_name + "::rtt);\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result += "    result += \",\\\"" + member_name.value + "\\\":\" + saver(self." + member_name.value + ");\n";
    }
    result += "    result += \"}\";\n";
    result += "    return result;\n";
    result += "}\n";
    result += "}\n //  end of namespace detail";
    result += "\n";

    result += "} //  end of namespace " + state.namespace_name + "\n";
    result += "namespace std\n";
    result += "{\n";
    result += "//  provide a simple hash, required by std::unordered_map\n";
    result += "template <>\n";
    result += "class hash<" + state.namespace_name + "::" + type_name + ">\n";
    result += "{\n";
    result += "    public:\n";
    result += "        size_t operator()(" + state.namespace_name + "::" + type_name + " const& value) const noexcept\n";
    result += "        {\n";
    result += "            std::hash<string> hasher;\n";
    result += "            return hasher(" + state.namespace_name + "::detail::saver(value));\n";
    result += "        }\n";
    result += "};\n";
    result += "}   //end of namespace std\n";
    result += "\n";
    result += "namespace " + state.namespace_name + "\n";
    result += "{\n";

    return result;
}
