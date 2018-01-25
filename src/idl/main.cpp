#include <belt.pp/parser.hpp>
#include <belt.pp/meta.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <exception>
#include <streambuf>
#include <cassert>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::runtime_error;
using std::ofstream;
using std::ifstream;
using std::vector;

using lexers = beltpp::typelist::type_list<
class operator_semicolon,
class keyword_class,
class scope_brace,
class operator_colon,
class identifier,
class discard
>;

using expression_tree = beltpp::expression_tree<lexers, std::string>;
using ptr_expression_tree = std::unique_ptr<expression_tree>;

string analyze(expression_tree const* pexpression);

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cout << "usage: idl input/definition/file.idl output/cpp/file.hpp";
        return 1;
    }

    string definition;
    ptr_expression_tree ptr_expression;
    try
    {
        ifstream file_definition(argv[1]);
        if (!file_definition)
            throw runtime_error("cannot open the input file");

        file_definition.seekg(0, std::ios::end);
        definition.reserve(file_definition.tellg());
        file_definition.seekg(0, std::ios::beg);

        definition.assign((std::istreambuf_iterator<char>(file_definition)),
                          std::istreambuf_iterator<char>());

        auto it_begin = definition.begin();
        auto it_end = definition.end();
        auto it_begin_keep = it_begin;
        while (beltpp::parse(ptr_expression, it_begin, it_end))
        {
            if (it_begin == it_begin_keep)
                break;
            else
            {
                //std::cout << std::string(it_begin_keep, it_begin) << std::endl;
                it_begin_keep = it_begin;
            }
        }

        bool is_value = false;
        auto proot = beltpp::root(ptr_expression.get(), is_value);

        ptr_expression.release();
        ptr_expression.reset(proot);

        if (false == is_value)
            throw runtime_error("missing expression, apparently");

        if (it_begin != it_end)
            throw runtime_error("syntax error, maybe" +
                                string(it_begin, it_end));

        ofstream file_generate(argv[2]);
        if (!file_generate)
            throw runtime_error("cannot open the output file");

        if (ptr_expression->depth() > 30)
            throw runtime_error("expected tree depth 30 is exceeded");

        //cout << beltpp::dump(ptr_expression.get()) << endl;
        file_generate << analyze(ptr_expression.get());

        //cout << "depth is " << ptr_expression->depth() << std::endl;
    }
    catch(std::exception const& ex)
    {
        cout << "exception: " << ex.what() << endl;

        if (ptr_expression)
        {
            cout << "=====\n";
            cout << beltpp::dump(ptr_expression.get()) << endl;
        }

        return 2;
    }
    catch(...)
    {
        cout << "that was an exception" << endl;
        return 3;
    }
    return 0;
}

class keyword_class : public beltpp::operator_lexer_base<keyword_class, lexers>
{
public:
    size_t right = 2;
    size_t left_max = 0;
    size_t left_min = 0;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z'))
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return std::string(it_begin, it_end) == "class";
    }
};

class scope_brace : public beltpp::operator_lexer_base<scope_brace, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 0;
    size_t left_min = 0;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        if (ch == '{')
        {
            right = -1;
            left_min = 0;
            left_max = 0;
            return std::make_pair(true, true);
        }
        if (ch == '}')
        {
            right = 0;
            left_min = 0;
            left_max = 1;
            return std::make_pair(true, true);
        }
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        string temp(it_begin, it_end);
        return (temp == "{" || temp == "}");
    }
};

class operator_semicolon : public beltpp::operator_lexer_base<operator_semicolon, lexers>
{
public:
    size_t right = 1;
    size_t left_max = -1;
    size_t left_min = 1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return beltpp::standard_operator_check<beltpp::standard_operator_set<void>>(ch);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return string(it_begin, it_end) == ";";
    }
};

class operator_colon : public beltpp::operator_lexer_base<operator_colon, lexers>
{
public:
    size_t right = 1;
    size_t left_max = 1;
    size_t left_min = 1;
    enum { grow_priority = 1 };

    std::pair<bool, bool> check(char ch)
    {
        return beltpp::standard_operator_check<beltpp::standard_operator_set<void>>(ch);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return string(it_begin, it_end) == ":";
    }
};

class identifier : public beltpp::value_lexer_base<identifier, lexers>
{
public:
    std::pair<bool, bool> check(char ch)
    {
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            ch == '_')
            return std::make_pair(true, false);
        return std::make_pair(false, false);
    }

    template <typename T_iterator>
    bool final_check(T_iterator const& it_begin,
                     T_iterator const& it_end) const
    {
        return it_begin != it_end;
    }

    bool scan_beyond() const
    {
        return false;
    }
};
class discard :
        public beltpp::discard_lexer_base<discard,
                                            lexers,
                                            beltpp::standard_white_space_set<void>>
{
public:
    bool scan_beyond() const
    {
        return false;
    }
};

string analyze_class(expression_tree const* pexpression,
                     size_t rtt,
                     string& class_name);

string analyze(expression_tree const* pexpression)
{
    size_t rtt = 0;
    string result;
    assert(pexpression);

    vector<string> class_names;
    string class_name;

    if (pexpression->lexem.rtt == keyword_class::rtt)
    {
        result += analyze_class(pexpression, rtt++, class_name);
        class_names.push_back(class_name);
    }
    else if (pexpression->lexem.rtt == operator_semicolon::rtt)
    {
        for (auto item : pexpression->children)
        {
            result += analyze_class(item, rtt++, class_name);
            class_names.push_back(class_name);
        }
    }
    else
        throw runtime_error("wtf");

    if (class_names.empty())
        throw runtime_error("wtf, nothing to do");

    result += "namespace detail\n";
    result += "{\n";
    result += "bool analyze_json_object(beltpp::json::expression_tree* pexp,\n";
    result += "                         beltpp::detail::pmsg_all& return_value)\n";
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
    result += "        code = analyze_json(*pmsgcode, pexp);\n";
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
    assert(pexpression_brace->children.size() <= 1);

    if (pexpression_brace->children.empty())
    {
        //  that's ok
    }
    else if (pexpression_brace->children.front()->lexem.rtt ==
            operator_colon::rtt)
        members.push_back(pexpression_brace->children.front());
    else if (pexpression_brace->children.front()->lexem.rtt ==
             operator_semicolon::rtt)
    {
        for (auto item : pexpression_brace->children.front()->children)
        {
            if (item->lexem.rtt == operator_colon::rtt)
                members.push_back(item);
            else
                throw runtime_error("inside class syntax error, wtf");
        }

        assert(false == members.empty());
    }
    else
        throw runtime_error("inside class syntax error, wtf, still");

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
    result += "                  beltpp::json::expression_tree* pexp)\n";
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
    result += "                code = analyze_json(msgcode." + member_name.value + ", item);\n";
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
