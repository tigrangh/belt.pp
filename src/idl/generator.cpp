#include "generator.hpp"

#include <cassert>
#include <vector>
#include <unordered_set>
#include <exception>
#include <utility>

using std::string;
using std::vector;
using std::unordered_map;
using std::unordered_multimap;
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
enum g_type_info
{
    type_empty = 0x0,
    type_simple = 0x1,
    type_object = 0x2,
    type_extension = 0x4,
    type_simple_object = type_simple | type_object,
    type_simple_extension = type_simple | type_extension,
    type_object_extension = type_object | type_extension,
    type_simple_object_extension = type_simple | type_object_extension
};

string convert_type(string const& type_name,
                    state_holder& state,
                    g_type_info& type_detail,
                    unordered_set<string>& type_names)
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

    if (type_detail == type_simple)
        type_names.insert(type_name);

    return type_name;
}

string construct_type_name(expression_tree const* member_type,
                           state_holder& state,
                           g_type_info& type_detail,
                           unordered_set<string>& type_names)
{
    if (member_type->lexem.rtt == identifier::rtt)
        return convert_type(member_type->lexem.value,
                            state,
                            type_detail,
                            type_names);
    else if (member_type->lexem.rtt == keyword_array::rtt &&
             member_type->children.size() == 1)
    {
        string type_name;
        if (member_type->children.front()->lexem.rtt == identifier::rtt)
            type_name = convert_type(member_type->children.front()->lexem.value,
                                     state, type_detail, type_names);
        else
            type_name = construct_type_name(member_type->children.front(),
                                            state, type_detail, type_names);

        return "std::vector<" + type_name + ">";
    }
    else if (member_type->lexem.rtt == keyword_set::rtt &&
             member_type->children.size() == 1 &&
             member_type->children.front()->lexem.rtt == identifier::rtt)
    {
        string type_name = convert_type(member_type->children.front()->lexem.value,
                                        state, type_detail, type_names);

        return "std::unordered_set<" + type_name + ">";
    }
    else if (member_type->lexem.rtt == keyword_hash::rtt &&
             member_type->children.size() == 2 &&
             member_type->children.front()->lexem.rtt == identifier::rtt &&
             member_type->children.back()->lexem.rtt == identifier::rtt)
    {
        g_type_info type_detail_key, type_detail_value;
        string key_type_name =
                construct_type_name(member_type->children.front(),
                                    state, type_detail_key, type_names);
        string value_type_name =
                construct_type_name(member_type->children.back(),
                                    state, type_detail_value, type_names);

        type_detail = static_cast<g_type_info>(type_detail_key | type_detail_value);

        if ((type_detail & type_object) &&
            (type_detail & type_extension))
            throw runtime_error("hash object extension mix");

        return "std::unordered_map<" + key_type_name  + ", " + value_type_name + ">";
    }
    else
        throw runtime_error("can't get type definition, wtf!");
}
}

generated_code analyze(state_holder& state,
                       expression_tree const* pexpression)
{
    size_t rtt = 0;
    generated_code result;
    assert(pexpression);

    unordered_map<size_t, string> class_names;
    unordered_map<string, generated_code> name_to_code;
    unordered_multimap<string, string> dependencies;
    std::string json_schema;

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
        size_t remain_count = pexpression->children.back()->children.size();
        for (auto item : pexpression->children.back()->children)
        {
            remain_count--;
            if (item->lexem.rtt == keyword_class::rtt)
            {
                if (item->children.size() != 2 ||
                    item->children.front()->lexem.rtt != identifier::rtt ||
                    item->children.back()->lexem.rtt != scope_brace::rtt)
                    throw runtime_error("class syntax is wrong");

                string type_name = item->children.front()->lexem.value;
                std::string class_members;
                generated_code code = analyze_struct(state,
                                                     item->children.back(),
                                                     rtt,
                                                     type_name,
                                                     dependencies,
                                                     class_members);

                if (0 == remain_count % 10)
                {
                    json_schema += ")foo\" R\"foo(\n";
                }

                json_schema += class_members;
                if (remain_count > 0)
                    json_schema += ",\n\n";
                else
                    json_schema += "\n";

                class_names.insert(std::make_pair(rtt, type_name));
                name_to_code.insert(std::make_pair(type_name, std::move(code)));
                ++rtt;
            }
            else if (item->lexem.rtt == keyword_enum::rtt)
            {
                if (item->children.size() != 2 ||
                    item->children.front()->lexem.rtt != identifier::rtt ||
                    item->children.back()->lexem.rtt != scope_brace::rtt)
                    throw runtime_error("enum syntax is wrong");

                string enum_name = item->children.front()->lexem.value;
                std::string enum_members;
                generated_code code = analyze_enum(state,
                                                   item->children.back(),
                                                   enum_name,
                                                   enum_members);
                json_schema += enum_members;
                if (remain_count > 0)
                    json_schema += ",\n\n";
                else
                    json_schema += "\n";

                result.declarations += code.declarations;
                result.template_definitions += code.template_definitions;
                result.definitions += code.definitions;
            }
        }
    }

    if (class_names.empty())
        throw runtime_error("wtf, nothing to do");

    size_t max_rtt = 0;
    for (auto const& class_item : class_names)
    {
        if (max_rtt < class_item.first)
            max_rtt = class_item.first;
    }

    //  clean up dependencies from unknown names
    //  leave class_names only
    auto it_dependencies = dependencies.begin();
    while (it_dependencies != dependencies.end())
    {
        if (name_to_code.find(it_dependencies->second) == name_to_code.end())
            it_dependencies = dependencies.erase(it_dependencies);
        else
            ++it_dependencies;
    }

    //  now order classes codes, so that each will
    //  meet it's dependency requirements
    while (false == name_to_code.empty())
    {
        bool no_progress = true;

        auto it_code = name_to_code.begin();
        while (it_code != name_to_code.end())
        {
            string const type_name = it_code->first;
            if (dependencies.find(type_name) == dependencies.end())
            {
                result.declarations += it_code->second.declarations;
                result.template_definitions += it_code->second.template_definitions;
                result.definitions += it_code->second.definitions;
                it_code = name_to_code.erase(it_code);
                no_progress = false;

                it_dependencies = dependencies.begin();
                while (it_dependencies != dependencies.end())
                {
                    if (it_dependencies->second == type_name)
                        it_dependencies = dependencies.erase(it_dependencies);
                    else
                        ++it_dependencies;
                }
            }
            else
                ++it_code;
        }
        if (no_progress)
            throw std::runtime_error("most probably there is a cyclic dependency"
                                     " in the model definition");
    }

    result.declarations += R"foo(
namespace detail
{
template <typename = void>
class storage
{
public:
    storage()
    {
        (void)(s_arr_fptr);
        (void)(json_schema);
    }
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

    static std::vector<storage_item> const s_arr_fptr;
    static std::string const json_schema;
};
}  // end of namespace detail
)foo";

result.definitions += R"foo(
namespace detail
{
template <typename T_message_type>
bool analyze_json_template(void* pvalue,
                           ::beltpp::json::expression_tree* pexp,
                           ::beltpp::message_loader_utility const& utl)
{
  T_message_type* pmsgcode =
          static_cast<T_message_type*>(pvalue);
  return analyze_json(*pmsgcode, pexp, utl);
}

template <typename T>
std::vector<typename storage<T>::storage_item> const storage<T>::s_arr_fptr =
{
)foo";
    for (size_t index = 0; index < max_rtt + 1; ++index)
    {
        auto it = class_names.find(index);
        if (it == class_names.end())
            result.definitions += "    {nullptr, nullptr, nullptr, nullptr}";
        else
        {
            auto const& class_name = it->second;
            result.definitions += "    {\n";
            result.definitions += "        &" + class_name + "::pvoid_saver,\n";
            result.definitions += "        &analyze_json_template<" + class_name + ">,\n";
            result.definitions += "        storage<>::fptr_new_void_unique_ptr(&::beltpp::new_void_unique_ptr<" + class_name + ">),\n";
            result.definitions += "        storage<>::fptr_new_void_unique_ptr_copy(&::beltpp::new_void_unique_ptr_copy<" + class_name + ">)\n";
            result.definitions += "    }";
        }
        if (index != max_rtt)
            result.definitions += ",\n";
    }
    result.definitions += "\n};\n";

    result.definitions += "template <typename T>\n";
    result.definitions += "std::string const storage<T>::json_schema = R\"foo(\n";
    result.definitions += "{\n\n";
    result.definitions += json_schema;
    result.definitions += "\n}\n";
    result.definitions += ")foo\";\n";

    result.definitions += "template class storage<void>;\n";

    result.definitions += R"foo(
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
}  // end of namespace detail
)foo";

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

generated_code analyze_struct(state_holder& state,
                              expression_tree const* pexpression,
                              size_t rtt,
                              string const& type_name,
                              unordered_multimap<string, string>& dependencies,
                              string& class_members)
{
    if (state.namespace_name.empty())
        throw runtime_error("please specify package name");
    assert(pexpression);

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

    generated_code result;

    result.declarations += "class " + type_name + "\n";
    result.declarations += "{\npublic:\n";

    result.declarations += "    enum {rtt = " + std::to_string(rtt) + "};\n";

    unordered_set<string> set_object_name;
    unordered_set<string> set_extension_name;

    unordered_map<string, string> map_member_name_type;

    unordered_set<string> member_type_names;

    class_members += "    \"" + type_name + "\": {\n";
    class_members += "        \"type\": \"object\",\n";
    class_members += "        \"rtt\": " + std::to_string(rtt) + ",\n";
    class_members += "        \"properties\": {\n";

    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;
        auto const& member_type = member_pair.second;
        if (member_name.rtt != identifier::rtt)
            throw runtime_error("use \"variable type\" syntax please");

        g_type_info type_detail;
        string member_type_name = construct_type_name(member_type,
                                                      state,
                                                      type_detail,
                                                      member_type_names);

        result.declarations += "    " + member_type_name + " " + member_name.value + ";\n";

        if (type_detail & type_object)
            set_object_name.insert(member_name.value);
        else if (type_detail & type_extension)
            set_extension_name.insert(member_name.value);

        map_member_name_type.insert(std::make_pair(member_name.value, member_type_name));

        if (member_pair.second->lexem.rtt == keyword_array::rtt)
            class_members += "            \"" + member_name.value + "\": { \"type\": \"" + member_pair.second->lexem.value  + " " + member_type->children.front()->lexem.value  + "\"},\n";
        else
            class_members += "            \"" + member_name.value + "\": { \"type\": \"" + member_pair.second->lexem.value + "\"},\n";
    }

    class_members += "        }\n";
    class_members += "    }";

    for (string const& dependency_type_name : member_type_names)
        dependencies.insert(std::make_pair(type_name, dependency_type_name));

    {   //  default constructor
    if (false == members.empty())
        result.declarations += "    inline " + type_name + "();\n";

    bool first = true;
    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;

        if (first)
            result.definitions += type_name + "::" + type_name + "()\n"
                    "  : " + member_name.value + "()\n";
        else
            result.definitions += "  , " + member_name.value + "()\n";

        first = false;
    }
    if (false == members.empty())
        result.definitions += "{}\n";
    }

    {   //  copy/move constructors
    if (false == members.empty())
    {
        result.declarations += "    inline " + type_name + "(" + type_name + " const& other);\n";
        result.declarations += "    inline " + type_name + "(" + type_name + "&& other);\n";
    }

    string copy_constructor, move_constructor;

    bool first = true;
    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;

        string copy_expr = member_name.value + "(other." + member_name.value + ")\n";
        string move_expr = member_name.value + "(std::move(other." + member_name.value + "))\n";

        if (set_contains(member_name.value, set_object_name) ||
            set_contains(member_name.value, set_extension_name))
        {
            copy_expr = member_name.value + "()\n";
            move_expr = member_name.value + "()\n";
        }

        if (first)
        {
            copy_constructor = type_name + "::" + type_name + "(" + type_name + " const& other)\n"
                    "  : " + copy_expr;
            move_constructor = type_name + "::" + type_name + "(" + type_name + "&& other)\n"
                    "  : " + move_expr;
        }
        else
        {
            copy_constructor += "  , " + copy_expr;
            move_constructor += "  , " + move_expr;
        }

        first = false;
    }
    if (false == members.empty())
    {
        if (set_object_name.empty() &&
            set_extension_name.empty())
        {
            copy_constructor += "{}\n";
            move_constructor += "{}\n";
        }
        else
        {
            copy_constructor += "{\n";
            move_constructor += "{\n";
            for (auto const& item : set_object_name)
            {
                copy_constructor += "    detail::assign_packet(" + item + ", other." + item + ");\n";
                move_constructor += "    detail::assign_packet(" + item + ", std::move(other." + item + "));\n";
            }
            for (auto const& item : set_extension_name)
            {
                copy_constructor += "    detail::assign_extension(" + item + ", other." + item + ");\n";
                move_constructor += "    detail::assign_extension(" + item + ", std::move(other." + item + "));\n";
            }
            copy_constructor += "}\n";
            move_constructor += "}\n";
        }
    }

    result.definitions += copy_constructor + move_constructor;
    }

    if (false == members.empty())
    {
        result.declarations += "    template <typename T>\n";
        result.declarations += "    explicit " + type_name + "(T&& other);\n";

        result.declarations += "    inline " + type_name + "& operator = (" + type_name + " const& other);\n";

        result.definitions += type_name + "& " + type_name + "::operator = (" + type_name + " const& other)\n";
        result.definitions += "{\n";
        result.definitions += "    ::beltpp::assign(*this, other);\n";
        result.definitions += "    return *this;\n";
        result.definitions += "}\n";

        result.declarations += "    inline " + type_name + "& operator = (" + type_name + "&& other);\n";

        result.definitions += type_name + "& " + type_name + "::operator = (" + type_name + "&& other)\n";
        result.definitions += "{\n";
        result.definitions += "    ::beltpp::assign(*this, std::move(other));\n";
        result.definitions += "    return *this;\n";
        result.definitions += "}\n";

        result.declarations += "    template <typename T>\n";
        result.declarations += "    " + type_name + "& operator = (T&& other);\n";
    }

    string const placeholder_other = members.empty() ? string() : " other";

    result.declarations += "    inline bool operator == (" + type_name + " const&" + placeholder_other + ") const;\n";
    result.definitions += "bool " + type_name + "::operator == (" + type_name + " const&" + placeholder_other + ") const\n";
    result.definitions += "{\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    if (set_object_name.find(member_name.value) != set_object_name.end() ||
        set_extension_name.find(member_name.value) != set_extension_name.end())
        result.definitions += "    if (detail::saver(" + member_name.value + ") != detail::saver(other." + member_name.value + "))\n";
    else
        result.definitions += "    if (" + member_name.value + " != other." + member_name.value + ")\n";
    result.definitions += "        return false;\n";
    }
    result.definitions += "    return true;\n";
    result.definitions += "}\n";

    result.declarations += "    inline bool operator != (" + type_name + " const& other) const;\n";

    result.definitions += "bool " + type_name + "::operator != (" + type_name + " const& other) const\n";
    result.definitions += "{\n";
    result.definitions += "    return false == (operator == (other));\n";
    result.definitions += "}\n";

    result.declarations += "    inline bool operator < (" + type_name + " const&" + placeholder_other + ") const;\n";
    result.definitions += "bool " + type_name + "::operator < (" + type_name + " const&" + placeholder_other + ") const\n";
    result.definitions += "{\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result.definitions += "    if (false == detail::less(" + member_name.value + ", other." + member_name.value + "))\n";
    result.definitions += "        return false;\n";
    }
    if (false == members.empty())
    result.definitions += "    return true;\n";
    else
    result.definitions += "    return false;\n";
    result.definitions += "}\n";

    result.declarations += "    inline bool operator > (" + type_name + " const& other) const;\n";
    result.definitions += "bool " + type_name + "::operator > (" + type_name + " const& other) const\n";
    result.definitions += "{\n";
    result.definitions += "    return other < *this;\n";
    result.definitions += "}\n";

    result.declarations += "    inline bool operator >= (" + type_name + " const& other) const;\n";
    result.definitions += "bool " + type_name + "::operator >= (" + type_name + " const& other) const\n";
    result.definitions += "{\n";
    result.definitions += "    return false == (*this < other);\n";
    result.definitions += "}\n";

    result.declarations += "    inline bool operator <= (" + type_name + " const& other) const;\n";
    result.definitions += "bool " + type_name + "::operator <= (" + type_name + " const& other) const\n";
    result.definitions += "{\n";
    result.definitions += "    return false == (*this > other);\n";
    result.definitions += "}\n";

    result.declarations += "    inline static std::string pvoid_saver(void* p);\n";
    result.definitions += "std::string " + type_name + "::pvoid_saver(void* p)\n";
    result.definitions += "{\n";
    result.definitions += "    " + type_name + "* pmc = static_cast<" + type_name + "*>(p);\n";
    result.definitions += "    return detail::saver(*pmc);\n";
    result.definitions += "}\n";

    result.declarations += "    inline std::string to_string() const;\n";
    result.definitions += "std::string " + type_name + "::to_string() const\n";
    result.definitions += "{\n";
    result.definitions += "    return detail::saver(*this);\n";
    result.definitions += "}\n";

    result.declarations += "    inline void from_string(std::string const& encoded, void* putl = nullptr);\n";
    result.definitions += "void " + type_name + "::from_string(std::string const& encoded, void* putl/* = nullptr*/)\n";
    result.definitions += "{\n";
    result.definitions += "    if (false == detail::loader(*this, encoded, putl))\n";
    result.definitions += "        throw std::runtime_error(\"cannot parse " + type_name + " data\");\n";
    result.definitions += "}\n";

    result.declarations += "};\n";  //  end of class

    if (false == members.empty())
    {
        result.template_definitions += "namespace beltpp\n";
        result.template_definitions += "{\n";
        result.template_definitions += "template <typename T>\n";
        result.template_definitions += "void assign(" + state.namespace_name + "::" + type_name + "& self, T const& other)\n";
        result.template_definitions += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", other." + member_name.value + ");\n";
        else if (set_contains(member_name.value, set_extension_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", other." + member_name.value + ");\n";
        else
        result.template_definitions += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result.template_definitions += "}\n";

        result.template_definitions += "template <typename T,\n";
        result.template_definitions += "          typename = typename std::enable_if<0 == " + state.namespace_name + "::detail::has_integer_rtt<T>::value>::type>\n";
        result.template_definitions += "void assign(T& self, " + state.namespace_name + "::" + type_name + " const& other)\n";
        result.template_definitions += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", other." + member_name.value + ");\n";
        else if (set_contains(member_name.value, set_extension_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", other." + member_name.value + ");\n";
        else
        result.template_definitions += "    ::beltpp::assign(self." + member_name.value + ", other." + member_name.value + ");\n";
        }
        result.template_definitions += "}\n";

        result.template_definitions += "template <typename T,\n";
        result.template_definitions += "          typename = typename std::enable_if<!std::is_reference<T>::value>::type>\n";
        result.template_definitions += "void assign(" + state.namespace_name + "::" + type_name + "& self, T&& other)\n";
        result.template_definitions += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        else if (set_contains(member_name.value, set_extension_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        else
        result.template_definitions += "    ::beltpp::assign(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        }
        result.template_definitions += "}\n";

        result.template_definitions += "template <typename T,\n";
        result.template_definitions += "          typename = typename std::enable_if<!std::is_reference<T>::value && 0 == " + state.namespace_name + "::detail::has_integer_rtt<T>::value>::type>\n";
        result.template_definitions += "void assign(T& self, " + state.namespace_name + "::" + type_name + "&& other)\n";
        result.template_definitions += "{\n";
        for (auto member_pair : members)
        {
        auto const& member_name = member_pair.first->lexem;
        if (set_contains(member_name.value, set_object_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_packet(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        else if (set_contains(member_name.value, set_extension_name))
            result.template_definitions += "    " + state.namespace_name + "::detail::assign_extension(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        else
        result.template_definitions += "    ::beltpp::assign(self." + member_name.value + ", std::move(other." + member_name.value + "));\n";
        }
        result.template_definitions += "}\n";

        result.template_definitions += "}  //  end of namespace beltpp\n";
    }

    result.template_definitions += "namespace " + state.namespace_name + "\n";
    result.template_definitions += "{\n";

    if (false == members.empty())
    {
        result.template_definitions += "template <typename T>\n";
        result.template_definitions += type_name + "::" + type_name + "(T&& other)\n";
        result.template_definitions += "{\n";
        result.template_definitions += "    ::beltpp::assign(*this, std::forward<T>(other));\n";
        result.template_definitions += "}\n";

        result.template_definitions += "template <typename T>\n";
        result.template_definitions += type_name + "& " + type_name + "::operator = (T&& other)\n";
        result.template_definitions += "{\n";
        result.template_definitions += "    ::beltpp::assign(*this, std::forward<T>(other));\n";
        result.template_definitions += "    return *this;\n";
        result.template_definitions += "}\n";
    }

    result.template_definitions += "namespace detail\n";
    result.template_definitions += "{\n";
    result.template_definitions += "inline std::string saver(" + type_name + " const& self);\n";
    result.template_definitions += "}\n";

    result.template_definitions += "}   //  end of namespace " + state.namespace_name + "\n";

    string const message_placeholder = members.empty() ? string() : " message";
    string const utl_placeholder = members.empty() ? string() : " utl";
    string const self_placeholder = members.empty() ? string() : " self";

    result.definitions += "namespace detail\n";
    result.definitions += "{\n";
    result.definitions += "inline\n";
    result.definitions += "bool analyze_json(" + type_name + "&" + message_placeholder + ",\n";
    result.definitions += "                  beltpp::json::expression_tree* pexp,\n";
    result.definitions += "                  ::beltpp::message_loader_utility const&" + utl_placeholder + ")\n";
    result.definitions += "{\n";
    result.definitions += "    bool code = true;\n";
    result.definitions += "    std::unordered_map<std::string, beltpp::json::expression_tree*> members;\n";
    result.definitions += "    size_t rtt = size_t(-1);\n";
    result.definitions += "    if (false == analyze_json_common(rtt, pexp, members) ||\n";
    result.definitions += "        rtt != " + type_name + "::rtt)\n";
    result.definitions += "        code = false;\n";
    result.definitions += "    else\n";
    result.definitions += "    {\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result.definitions += "        if (code)\n";
    result.definitions += "        {\n";
    string utl_var_name = "utl";
    bool is_extension = false;
    if (set_extension_name.find(member_name.value) != set_extension_name.end())
        is_extension = true;
    if (is_extension)
    {
    utl_var_name = "utl2";
    result.definitions += "            auto utl2 = utl;\n";
    result.definitions += "            if (utl2.m_arr_fp_message_list_load_helper.empty() ||\n";
    result.definitions += "                nullptr == utl2.m_arr_fp_message_list_load_helper.front())\n";
    result.definitions += "                code = false;\n";
    result.definitions += "            else\n";
    result.definitions += "            {\n";
    result.definitions += "            utl2.m_fp_message_list_load_helper = utl2.m_arr_fp_message_list_load_helper.front();\n";
    result.definitions += "            utl2.m_arr_fp_message_list_load_helper.pop_front();\n";
    }
    result.definitions += "            auto it_find = members.find(\"\\\"" + member_name.value + "\\\"\");\n";
    result.definitions += "            if (it_find == members.end())\n";
    result.definitions += "                code = false;\n";
    result.definitions += "            else\n";
    result.definitions += "            {\n";
    result.definitions += "                beltpp::json::expression_tree* item = it_find->second;\n";
    result.definitions += "                assert(item);\n";
    result.definitions += "                code = analyze_json(message." + member_name.value + ", item, " + utl_var_name + ");\n";
    result.definitions += "            }\n";
    if (is_extension)
    {
    result.definitions += "            }\n";
    }
    result.definitions += "        }\n";
    }
    result.definitions += "    }\n";
    result.definitions += "    return code;\n";
    result.definitions += "}\n";

    result.definitions += "std::string saver(" + type_name + " const&" + self_placeholder + ")\n";
    result.definitions += "{\n";
    result.definitions += "    std::string result;\n";
    result.definitions += "    result += \"{\\\"rtt\\\":\" + saver(" + type_name + "::rtt);\n";
    for (auto member_pair : members)
    {
    auto const& member_name = member_pair.first->lexem;
    result.definitions += "    result += \",\\\"" + member_name.value + "\\\":\" + saver(self." + member_name.value + ");\n";
    }
    result.definitions += "    result += \"}\";\n";
    result.definitions += "    return result;\n";
    result.definitions += "}\n";
    result.definitions += "}   //  end of namespace detail\n";
    result.definitions += "\n";

    result.template_definitions += "namespace std\n";
    result.template_definitions += "{\n";
    result.template_definitions += "//  provide a simple hash, required by std::unordered_map\n";
    result.template_definitions += "template <>\n";
    result.template_definitions += "struct hash<" + state.namespace_name + "::" + type_name + ">\n";
    result.template_definitions += "{\n";
    result.template_definitions += "    size_t operator()(" + state.namespace_name + "::" + type_name + " const& value) const noexcept\n";
    result.template_definitions += "    {\n";
    result.template_definitions += "        std::hash<string> hasher;\n";
    result.template_definitions += "        return hasher(" + state.namespace_name + "::detail::saver(value));\n";
    result.template_definitions += "    }\n";
    result.template_definitions += "};\n";
    result.template_definitions += "}   //  end of namespace std\n";
    result.template_definitions += "\n";

    return result;
}

generated_code analyze_enum(state_holder& state,
                            expression_tree const* pexpression,
                            string const& enum_name,
                            string& enum_members)
{
    if (state.namespace_name.empty())
        throw runtime_error("please specify package name");

    generated_code result;

    if (pexpression->children.empty())
        throw runtime_error("inside enum syntax error, wtf - " + enum_name);

    result.declarations = "enum class " + enum_name + "\n";

    bool first = true;

    size_t remain_count = pexpression->children.size();
    enum_members += "    \"" + enum_name + "\": {\n";
    enum_members += "        \"type\": \"enum\",\n";
    enum_members += "        \"values\": [\n";

    for (auto const& item : pexpression->children)
    {
        enum_members += "            \"" + item->lexem.value + "\"";
        remain_count--;
        if (remain_count > 0)
            enum_members += ",\n";
        else
            enum_members += "\n";

        if (first)
            result.declarations += "{";
        else
            result.declarations += ",";
        result.declarations += "\n    " + item->lexem.value;

        first = false;
    }

    enum_members += "        ]\n";
    enum_members += "    }";

    result.declarations += "\n};\n";

    result.declarations += "inline void from_string(std::string const& string_value, " + enum_name + "& value);\n";

    result.definitions += "void from_string(std::string const& string_value,\n";
    result.definitions += "                 " + enum_name + "& value)\n";
    result.definitions += "{\n";
    for (auto const& item : pexpression->children)
    {
        result.definitions += "    if (\"" + item->lexem.value + "\" == string_value)\n";
        result.definitions += "    {\n";
        result.definitions += "        value = " + enum_name + "::" + item->lexem.value + ";\n";
        result.definitions += "        return;\n";
        result.definitions += "    }\n";
    }
    result.definitions += "    throw std::runtime_error(string_value + \": is not recognized by enum " + enum_name + " {";
    first = true;
    for (auto const& item : pexpression->children)
    {
        if (false == first)
            result.definitions += ", ";
        result.definitions += item->lexem.value;
        first = false;
    }
    result.definitions += "}\");\n";
    result.definitions += "}\n";    //  end of void from_string

    result.definitions += "namespace detail\n";
    result.definitions += "{\n";
    result.definitions += "inline\n";
    result.definitions += "bool analyze_json(" + enum_name + "& value,\n";
    result.definitions += "                  beltpp::json::expression_tree* pexp,\n";
    result.definitions += "                  ::beltpp::message_loader_utility const& utl)\n";
    result.definitions += "{\n";
    result.definitions += "    std::string string_value;\n";
    result.definitions += "    if (false == analyze_json(string_value, pexp, utl))\n";
    result.definitions += "        return false;\n";

    for (auto const& item : pexpression->children)
    {
        result.definitions += "    if (\"" + item->lexem.value + "\" == string_value)\n";
        result.definitions += "    {\n";
        result.definitions += "        value = " + enum_name + "::" + item->lexem.value + ";\n";
        result.definitions += "        return true;\n";
        result.definitions += "    }\n";
    }
    result.definitions += "    return false;\n";
    result.definitions += "}\n";

    result.definitions += "inline\n";
    result.definitions += "std::string saver(" + enum_name + " const& value)\n";
    result.definitions += "{\n";
    result.definitions += "    switch (value)\n";
    result.definitions += "    {\n";
    for (auto const& item : pexpression->children)
    {
        result.definitions += "    case " + enum_name + "::" + item->lexem.value + ":\n";
        result.definitions += "        return saver(std::string(\"" + item->lexem.value + "\"));\n";
    }
    result.definitions += "    }\n";
    result.definitions += "    //  msvc thinks this is an execution path that needs to be covered\n";
    result.definitions += "    assert(false);\n";
    result.definitions += "    throw std::runtime_error(\"case that must never happen\");\n";
    result.definitions += "}\n";
    result.definitions += "}   //  end of namespace detail\n";

    result.declarations += "inline std::string to_string(const " + enum_name + "& value);\n";

    result.definitions += "std::string to_string(const " + enum_name + "& value)\n";
    result.definitions += "{\n";
    result.definitions += "    return detail::saver(value);\n";
    result.definitions += "}\n";    //  end of to_string

    result.definitions += "}   //  end of namespace " + state.namespace_name + "\n";

    result.definitions += "\n";
    result.definitions += "namespace " + state.namespace_name + "\n";
    result.definitions += "{\n";

    return result;
}
