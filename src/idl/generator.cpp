#include "generator.hpp"

#include <cassert>
#include <vector>
#include <exception>

using std::string;
using std::vector;
using std::runtime_error;

string analyze(expression_tree const* pexpression)
{
    size_t rtt = 0;
    string result;
    assert(pexpression);

    vector<string> class_names;
    string class_name;

    if (pexpression->lexem.rtt != keyword_namespace::rtt)
        throw runtime_error("use one and only one namespace");
    else if (pexpression->children.size() != 2 ||
             pexpression->children.front()->lexem.rtt != identifier::rtt ||
             pexpression->children.back()->lexem.rtt != scope_brace::rtt)
        throw runtime_error("namespace syntax error");
    else
    {
        for (auto item : pexpression->children.back()->children)
        {
            result += analyze_class(item, rtt++, class_name);
            class_names.push_back(class_name);
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
    result += "                                            make_void_unique_ptr<" + class_name + ">(),\n";
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

string analyze_class(expression_tree const* pexpression,
                     size_t rtt,
                     string& class_name)
{
    string result;
    assert(pexpression);

    if (pexpression->children.size() != 2 ||
        pexpression->children.front()->lexem.rtt != identifier::rtt ||
        pexpression->children.back()->lexem.rtt != scope_brace::rtt)
        throw runtime_error("class syntax error");

    class_name = pexpression->children.front()->lexem.value;

    result += "class " + class_name + ";\n";
    result += "namespace detail\n";
    result += "{\n";
    result += "std::string saver(" + class_name + " const& self);\n";
    result += "}\n";

    result += "class " + class_name + "\n";
    result += "{\npublic:\n";

    vector<expression_tree const*> members;
    auto pexpression_brace = pexpression->children.back();

    for (auto item : pexpression_brace->children)
    {
        if (item->lexem.rtt == operator_colon::rtt)
            members.push_back(item);
        else
            throw runtime_error("inside class syntax error, wtf");
    }

    result += "    enum {rtt = " + std::to_string(rtt) + "};\n";

    for (auto member_colon : members)
    {
        assert(member_colon->children.size() == 2);

        auto const& member_name = member_colon->children.front()->lexem;
        auto const& member_type = member_colon->children.back()->lexem;
        if (member_name.rtt !=
                identifier::rtt ||
            member_type.rtt !=
                identifier::rtt)
            throw runtime_error("use \"variable : type\" syntax please");

        result += "    " + member_type.value + " " +
                member_name.value + ";\n";
    }

    {
    bool first = true;
    for (auto member_colon : members)
    {
        auto const& member_name = member_colon->children.front()->lexem;
        //auto const& member_type = member_colon->children.back()->lexem;
        if (first)
            result += "    " + class_name + "()\n"
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
        result += "    explicit " + class_name + "(T const& other)\n";
        result += "    {\n";
        result += "        assign(*this, other);\n";
        result += "    }\n";

        result += "    template <typename T>\n";
        result += "    " + class_name + "& operator = (T const& other)\n";
        result += "    {\n";
        result += "        assign(*this, other);\n";
        result += "        return *this;\n";
        result += "    }\n";
    }

    result += "    static std::vector<char> saver(void* p)\n";
    result += "    {\n";
    result += "        " + class_name + "* pmc = static_cast<" + class_name + "*>(p);\n";
    result += "        std::vector<char> result;\n";
    result += "        std::string str_value = detail::saver(*pmc);\n";
    result += "        std::copy(str_value.begin(), str_value.end(), std::back_inserter(result));\n";
    result += "        return result;\n";
    result += "    }\n";

    result += "};\n";

    if (false == members.empty())
    {
        result += "template <typename T>\n";
        result += "void assign(" + class_name + "& self, T const& other)\n";
        result += "{\n";
        for (auto member_colon : members)
        {
        auto const& member_name = member_colon->children.front()->lexem;
        //auto const& member_type = member_colon->children.back()->lexem;
        result += "    assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";

        result += "template <typename T>\n";
        result += "void assign(T& self, " + class_name + " const& other)\n";
        result += "{\n";
        for (auto member_colon : members)
        {
        auto const& member_name = member_colon->children.front()->lexem;
        //auto const& member_type = member_colon->children.back()->lexem;
        result += "    assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";
    }

    result += "namespace detail\n";
    result += "{\n";
    result += "bool analyze_json(" + class_name + "& msgcode,\n";
    result += "                  beltpp::json::expression_tree* pexp,\n";
    result += "                  utils const& utl)\n";
    result += "{\n";
    result += "    bool code = true;\n";
    result += "    std::unordered_map<std::string, beltpp::json::expression_tree*> members;\n";
    result += "    size_t rtt = -1;\n";
    result += "    if (false == analyze_json_common(rtt, pexp, members) ||\n";
    result += "        rtt != " + class_name + "::rtt)\n";
    result += "        code = false;\n";
    result += "    else\n";
    result += "    {\n";
    for (auto member_colon : members)
    {
    auto const& member_name = member_colon->children.front()->lexem;
    //auto const& member_type = member_colon->children.back()->lexem;
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

    result += "std::string saver(" + class_name + " const& self)\n";
    result += "{\n";
    result += "    std::string result;\n";
    result += "    result += \"{\";\n";
    result += "    result += \"\\\"rtt\\\" : \" + saver(" + class_name + "::rtt);\n";
    for (auto member_colon : members)
    {
    auto const& member_name = member_colon->children.front()->lexem;
    //auto const& member_type = member_colon->children.back()->lexem;
    result += "    result += \",\\\"" + member_name.value + "\\\" : \" + saver(self." + member_name.value + ");\n";
    }
    result += "    result += \"}\";\n";
    result += "    return result;\n";
    result += "}\n";
    result += "}\n";
    result += "\n";
    result += "\n";

    return result;
}
