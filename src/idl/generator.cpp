#include "generator.hpp"

#include <cassert>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <utility>

using std::string;
using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::pair;
using std::runtime_error;

state_holder::state_holder()
    : namespace_name()
    , map_types{{"String", "std::string"},
                {"Bool", "bool"},
                {"Int8", "int8_t"},
                {"UInt8", "uint8_t"},
                {"Int16", "int16_t"},
                {"UInt16", "uint16_t"},
                {"Int", "uint32_t"},
                {"Int32", "int32_t"},
                {"UInt32", "uint32_t"},
                {"Int64", "int64_t"},
                {"UInt64", "uint64_t"},
                {"Float32", "float"},
                {"Float64", "double"},
                {"TimePoint", "ctime"},
                {"Object", "::beltpp::packet"},
                {"Extension", "::beltpp::packet"}}
{
}

namespace
{
enum type_info {type_empty = 0x0,
                type_simple = 0x1,
                type_object=0x2,
                type_extension=0x4,
                type_simple_object = type_simple | type_object,
                type_simple_extension = type_simple | type_extension,
                type_object_extension = type_object | type_extension,
                type_simple_object_extension = type_simple | type_object_extension};

string convert_type(string const& type_name, state_holder& state, type_info& type_detail)
{
    type_detail = type_empty;
    if ("Object" == type_name)
        type_detail = type_object;
    else if ("Extension" == type_name)
        type_detail = type_extension;
    else
        type_detail = type_simple;

    auto it_type_name = state.map_types.find(type_name);
    if (it_type_name != state.map_types.end())
        return it_type_name->second;
    return type_name;
}

string construct_type_name (expression_tree const* member_type,
                            state_holder& state,
                            type_info& type_detail)
{
    if (member_type->lexem.rtt == identifier::rtt)
        return convert_type(member_type->lexem.value, state, type_detail);
    else if (member_type->lexem.rtt == keyword_array::rtt &&
             member_type->children.size() == 1 &&
             member_type->children.front()->lexem.rtt == identifier::rtt)
    {
        string type_name = convert_type(member_type->children.front()->lexem.value,
                                        state, type_detail);
        return "std::vector<" + type_name + ">";
    }
    else if (member_type->lexem.rtt == keyword_hash::rtt &&
             member_type->children.size() == 2 &&
             member_type->children.front()->lexem.rtt == identifier::rtt &&
             member_type->children.back()->lexem.rtt == identifier::rtt)
    {
        type_info type_detail_key, type_detail_value;
        string key_type_name =
                construct_type_name(member_type->children.front(),
                                    state, type_detail_key);
        string value_type_name =
                construct_type_name(member_type->children.back(),
                                    state, type_detail_value);

        type_detail = static_cast<type_info>(type_detail_key | type_detail_value);

        if ((type_detail & type_object) &&
            (type_detail & type_extension))
            throw runtime_error("hash object extension mix");

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

    unordered_map<size_t, string> class_names, type_names;

    if (pexpression->lexem.rtt != keyword_module::rtt ||
        pexpression->children.size() != 2 ||
        pexpression->children.front()->lexem.rtt != identifier::rtt ||
        pexpression->children.back()->lexem.rtt != scope_brace::rtt ||
        pexpression->children.back()->children.empty())
        throw runtime_error("wtf");
    else
    {
        string module_name = pexpression->children.front()->lexem.value;
        state.namespace_name = module_name;

        for (auto item : pexpression->children.back()->children)
        {
            if (item->lexem.rtt == keyword_type::rtt ||
                item->lexem.rtt == keyword_class::rtt)
            {
                bool serializable = (item->lexem.rtt == keyword_class::rtt);

                if (item->children.size() != 2 ||
                    item->children.front()->lexem.rtt != identifier::rtt ||
                    item->children.back()->lexem.rtt != scope_brace::rtt)
                    throw runtime_error("type syntax is wrong");

                string type_name = item->children.front()->lexem.value;

                result += analyze_struct(state,
                                         item->children.back(),
                                         rtt,
                                         type_name,
                                         serializable);

                if (serializable)
                    class_names.insert(std::make_pair(rtt, type_name));
                else
                    type_names.insert(std::make_pair(rtt, type_name));
                ++rtt;
            }
        }
    }

    if (class_names.empty() && type_names.empty())
        throw runtime_error("wtf, nothing to do");

    size_t max_rtt = 0;
    for (auto const& class_item : class_names)
    {
        if (max_rtt < class_item.first)
            max_rtt = class_item.first;
    }
    for (auto const& type_item : type_names)
    {
        if (max_rtt < type_item.first)
            max_rtt = type_item.first;
    }

    result += R"foo(
namespace detail
{
template <typename = void>
class storage
{
public:
    using fptr_saver = ::beltpp::detail::fptr_saver;
    using fptr_analyze_json = bool (*)(void*, beltpp::json::expression_tree*, ::beltpp::message_loader_utility const&);
    using fptr_new_void_unique_ptr = ::beltpp::void_unique_ptr (*)();
    using fptr_new_void_unique_ptr_copy = ::beltpp::void_unique_ptr (*)(void const*);

    class storage_item
    {
    public:
        fptr_saver fp_saver;
        fptr_analyze_json fp_analyze_json;
        fptr_new_void_unique_ptr fp_new_void_unique_ptr;
        fptr_new_void_unique_ptr_copy fp_new_void_unique_ptr_copy;
    };

    template <typename T_message_type>
    static bool analyze_json_template(void* pvalue,
                                      ::beltpp::json::expression_tree* pexp,
                                      ::beltpp::message_loader_utility const& utl)
    {
        T_message_type* pmsgcode =
                static_cast<T_message_type*>(pvalue);
        return analyze_json(*pmsgcode, pexp, utl);
    }
    static std::vector<storage_item> const s_arr_fptr;
};
template <typename T>
std::vector<typename storage<T>::storage_item> const storage<T>::s_arr_fptr =
{
)foo";
    for (size_t index = 0; index < max_rtt + 1; ++index)
    {
        auto it = class_names.find(index);
        if (it == class_names.end())
            result += "     {nullptr, nullptr, nullptr, nullptr}";
        else
        {
            auto const& class_name = it->second;
            result += "    {\n";
            result += "        &" + class_name + "::saver,\n";
            result += "        &storage<>::analyze_json_template<" + class_name + ">,\n";
            result += "        (storage<>::fptr_new_void_unique_ptr)&::beltpp::new_void_unique_ptr<" + class_name + ">,\n";
            result += "        (storage<>::fptr_new_void_unique_ptr_copy)&::beltpp::new_void_unique_ptr<" + class_name + ">\n";
            result += "    }";
        }
        if (index != max_rtt)
            result += ",\n";
    }
    result += R"foo(
};

bool analyze_json_object(beltpp::json::expression_tree* pexp,
                         beltpp::detail::pmsg_all& return_value,
                         ::beltpp::message_loader_utility const& utl)
{
    bool code = false;

    if (storage<>::s_arr_fptr.size() > return_value.rtt)
    {
        auto const& item = storage<>::s_arr_fptr[return_value.rtt];

        if (item.fp_analyze_json && item.fp_new_void_unique_ptr &&
            item.fp_new_void_unique_ptr_copy && item.fp_saver)
        {
            return_value =
                    beltpp::detail::pmsg_all(   return_value.rtt,
                                                item.fp_new_void_unique_ptr(),
                                                item.fp_saver);

            code = item.fp_analyze_json(return_value.pmsg.get(), pexp, utl);
        }
    }

    return code;
}
})foo";

    return result;
}
template <typename T>
inline bool set_contains(T const& value, unordered_set<T> const& set)
{
    auto it = set.find(value);
    if (it == set.end())
        return false;
    return true;
}

string analyze_struct(state_holder& state,
                      expression_tree const* pexpression,
                      size_t rtt,
                      string const& type_name,
                      bool serializable)
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
        throw runtime_error("inside class syntax error, wtf - " + type_name);

    auto it = pexpression->children.begin();
    for (size_t index = 0;
         it != pexpression->children.end();
         ++index, ++it)
    {
        auto const* member_type = *it;
        ++it;
        auto const* member_name = *it;

        if (member_name->lexem.rtt != identifier::rtt)
            throw runtime_error("inside class syntax error, wtf, still " + type_name);

        members.push_back(std::make_pair(member_name, member_type));
    }

    result += "    enum {rtt = " + std::to_string(rtt) + "};\n";

    unordered_set<string> set_object_name;
    unordered_set<string> set_extension_name;

    unordered_map<string, string> map_member_name_type;

    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;
        auto const& member_type = member_pair.second;
        if (member_name.rtt != identifier::rtt)
            throw runtime_error("use \"variable type\" syntax please");

        type_info type_detail;
        string member_type_name = construct_type_name(member_type, state, type_detail);
        result += "    " + member_type_name + " " + member_name.value + ";\n";

        if (type_detail & type_object)
            set_object_name.insert(member_name.value);
        else if (type_detail & type_extension)
            set_extension_name.insert(member_name.value);

        map_member_name_type.insert(std::make_pair(member_name.value, member_type_name));
    }

    auto member_type_by_name = [&map_member_name_type](string const& name)
    {
        return map_member_name_type[name];
    };

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
    if (false == members.empty())
        result += "    {}\n";
    }

    {
    bool first = true;
    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;

        string copy_expr = member_name.value + "(other." + member_name.value + ")\n";

        if (set_contains(member_name.value, set_object_name) ||
            set_contains(member_name.value, set_extension_name))
            copy_expr = member_name.value + "()\n";

        if (first)
            result += "    " + type_name + "(" + type_name + " const& other)\n"
                    "      : " + copy_expr;
        else
            result += "      , " + copy_expr;

        first = false;
    }
    if (false == members.empty())
    {
        if (set_object_name.empty() &&
            set_extension_name.empty())
            result += "    {}\n";
        else
        {
            result += "    {\n";
            for (auto const& item : set_object_name)
                result += "        detail::assign_packet(" + item + ", other." + item + ");\n";
            for (auto const& item : set_extension_name)
                result += "        detail::assign_extension(" + item + ", other." + item + ");\n";
            result += "    }\n";
        }
    }
    }

    if (false == members.empty())
    {
        result += "    template <typename T>\n";
        result += "    explicit " + type_name + "(T const& other)\n";
        result += "    {\n";
        for (auto member_pair : members)
        {
            auto const& member_name = member_pair.first->lexem;
            auto const& mn = member_name.value;
            auto const mt = member_type_by_name(mn);
            if (set_contains(member_name.value, set_object_name))
                result += "        detail::assign_packet(" + member_name.value + ", other." + member_name.value + ");\n";
            else if (set_contains(member_name.value, set_extension_name))
                result += "        detail::assign_extension(" + member_name.value + ", other." + member_name.value + ");\n";
            else
                result += "        ::beltpp::assign(" + mn + ", other." + mn + ");\n";
        }
        result += "    }\n";

        result += "    " + type_name + "& operator = (" + type_name + " const& other)\n";
        result += "    {\n";
        for (auto member_pair : members)
        {
            auto const& member_name = member_pair.first->lexem;
            if (set_contains(member_name.value, set_object_name))
                result += "        detail::assign_packet(" + member_name.value + ", other." + member_name.value + ");\n";
            else if (set_contains(member_name.value, set_extension_name))
                result += "        detail::assign_extension(" + member_name.value + ", other." + member_name.value + ");\n";
            else
                result += "        ::beltpp::assign(" + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "        return *this;\n";
        result += "    }\n";

        result += "    " + type_name + "& operator = (" + type_name + "&& other)\n";
        result += "    {\n";
        for (auto member_pair : members)
        {
            auto const& member_name = member_pair.first->lexem;
            if (set_contains(member_name.value, set_object_name))
                result += "        detail::assign_packet(" + member_name.value + ", other." + member_name.value + ");\n";
            else if (set_contains(member_name.value, set_extension_name))
                result += "        detail::assign_extension(" + member_name.value + ", other." + member_name.value + ");\n";
            else
                result += "        ::beltpp::assign(" + member_name.value + ", std::move(other." + member_name.value + "));\n";
        }
        result += "        return *this;\n";
        result += "    }\n";

        result += "    template <typename T>\n";
        result += "    " + type_name + "& operator = (T const& other)\n";
        result += "    {\n";
        for (auto member_pair : members)
        {
            auto const& member_name = member_pair.first->lexem;
            auto const& mn = member_name.value;
            auto const mt = member_type_by_name(mn);
            if (set_contains(member_name.value, set_object_name))
                result += "        detail::assign_packet(" + member_name.value + ", other." + member_name.value + ");\n";
            else if (set_contains(member_name.value, set_extension_name))
                result += "        detail::assign_extension(" + member_name.value + ", other." + member_name.value + ");\n";
            else
                result += "        ::beltpp::assign(" + mn + ", other." + mn + ");\n";
        }
        result += "        return *this;\n";
        result += "    }\n";
    }

    result += "    bool operator == (" + type_name + " const& other) const\n";
    result += "    {\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    if (set_object_name.find(member_name.value) != set_object_name.end() ||
        set_extension_name.find(member_name.value) != set_extension_name.end())
        result += "        if (detail::saver(" + member_name.value + ") != detail::saver(other." + member_name.value + "))\n";
    else
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

    result += "    bool operator >= (" + type_name + " const& other) const\n";
    result += "    {\n";
    result += "        return false == (*this < other);\n";
    result += "    }\n";

    result += "    bool operator <= (" + type_name + " const& other) const\n";
    result += "    {\n";
    result += "        return false == (*this > other);\n";
    result += "    }\n";

    result += "    static std::vector<char> saver(void* p)\n";
    result += "    {\n";
    if (serializable)
    {
    result += "        " + type_name + "* pmc = static_cast<" + type_name + "*>(p);\n";
    result += "        std::vector<char> result;\n";
    result += "        std::string str_value = detail::saver(*pmc);\n";
    result += "        std::copy(str_value.begin(), str_value.end(), std::back_inserter(result));\n";
    result += "        return result;\n";
    }
    else
    {
    result += "        return std::vector<char>();\n";
    }
    result += "    }\n";

    if (serializable)
    {
    result += "    static std::string string_saver(" + type_name + " const& ob)\n";
    result += "    {\n";
    result += "        return detail::saver(ob);\n";
    result += "    }\n";
    result += "    static void string_loader(" + type_name + "& ob, std::string const& encoded)\n";
    result += "    {\n";
    result += "        if (false == detail::loader(ob, encoded, nullptr))\n";
    result += "            throw std::runtime_error(\"cannot parse " + type_name + " data\");\n";
    result += "    }\n";
    }

    result += "};  // end of class\n";

    result += "}   // end of namespace " + state.namespace_name + "\n";

    if (false == members.empty())
    {
        result += "namespace beltpp\n";
        result += "{\n";
        result += "template <typename T>\n";
        result += "void assign(" + state.namespace_name + "::" + type_name + "& self, T const& other)\n";
        result += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", other." + member_name.value + ");\n";
        else if (set_contains(member_name.value, set_extension_name))
            result += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", other." + member_name.value + ");\n";
        else
        result += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";

        result += "template <typename T, typename = typename std::enable_if<!std::is_same<T, " +
                    state.namespace_name + "::" + type_name + ">::value, void>::type>\n";
        result += "void assign(T& self, " + state.namespace_name + "::" + type_name + " const& other)\n";
        result += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", other." + member_name.value + ");\n";
        else if (set_contains(member_name.value, set_extension_name))
            result += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", other." + member_name.value + ");\n";
        else
        result += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result += "}\n";

        result += "template <typename T>\n";
        result += "void assign(T& self, " + state.namespace_name + "::" + type_name + "&& other)\n";
        result += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", other." + member_name.value + ");\n";
        else if (set_contains(member_name.value, set_extension_name))
            result += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", other." + member_name.value + ");\n";
        else
        result += "    ::beltpp::assign(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        }
        result += "}\n";
        result += "}   // end of namespace beltpp\n";
    }
    if (serializable)
    {
    result += "namespace " + state.namespace_name + "\n";
    result += "{\n";
    result += "namespace detail\n";
    result += "{\n";
    result += "inline\n";
    result += "bool analyze_json(" + type_name + "& msgcode,\n";
    result += "                  beltpp::json::expression_tree* pexp,\n";
    result += "                  ::beltpp::message_loader_utility const& utl)\n";
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
    string utl_var_name = "utl";
    bool is_extension = false;
    if (set_extension_name.find(member_name.value) != set_extension_name.end())
        is_extension = true;
    if (is_extension)
    {
    utl_var_name = "utl2";
    result += "            auto utl2 = utl;\n";
    result += "            if (utl2.m_arr_fp_message_list_load_helper.empty() ||\n";
    result += "                nullptr == utl2.m_arr_fp_message_list_load_helper.front())\n";
    result += "                code = false;\n";
    result += "            else\n";
    result += "            {\n";
    result += "            utl2.m_fp_message_list_load_helper = utl2.m_arr_fp_message_list_load_helper.front();\n";
    result += "            utl2.m_arr_fp_message_list_load_helper.pop_front();\n";
    }
    result += "            auto it_find = members.find(\"\\\"" + member_name.value + "\\\"\");\n";
    result += "            if (it_find != members.end())\n";
    result += "            {\n";
    result += "                beltpp::json::expression_tree* item = it_find->second;\n";
    result += "                assert(item);\n";
    result += "                code = analyze_json(msgcode." + member_name.value + ", item, " + utl_var_name + ");\n";
    result += "            }\n";
    if (is_extension)
    {
    result += "            }\n";
    }
    result += "        }\n";
    }
    result += "    }\n";
    result += "    return code;\n";
    result += "}\n";

    result += "inline\n";
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
    result += "}   //  end of namespace detail\n";
    result += "\n";

    result += "}   //  end of namespace " + state.namespace_name + "\n";
    }   //  if serializable
    result += "namespace std\n";
    result += "{\n";
    result += "//  provide a simple hash, required by std::unordered_map\n";
    result += "template <>\n";
    result += "struct hash<" + state.namespace_name + "::" + type_name + ">\n";
    result += "{\n";
    result += "    size_t operator()(" + state.namespace_name + "::" + type_name + " const& value) const noexcept\n";
    result += "    {\n";
    result += "        std::hash<string> hasher;\n";
    result += "        return hasher(" + state.namespace_name + "::detail::saver(value));\n";
    result += "    }\n";
    result += "};\n";
    result += "}   //end of namespace std\n";
    result += "\n";
    result += "namespace " + state.namespace_name + "\n";
    result += "{\n";

    return result;
}
