#include "generator.hpp"

#include <cassert>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <exception>
#include <utility>
#include<iostream>
#include<string>

using std::string;
using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::pair;
using std::runtime_error;

state_holder::state_holder()
    : namespace_name()
    //  use php types below instead
    , map_types{{"String", "string"},
                {"Bool", "bool"},
                {"Int8", "int"},
                {"UInt8", "int"},
                {"Int16", "int"},
                {"UInt16", "int"},
                {"Int", "int"},
                {"Int32", "int"},
                {"UInt32", "int"},
                {"Int64", "int"},
                {"UInt64", "int"},
                {"Float32", "float"},
                {"Float64", "double"},
                {"TimePoint", "ctime"},
                {"Object", "::beltpp::packet"},
                {"Extension", "::beltpp::packet"}}
{
}

namespace
{
enum g_type_info {type_empty = 0x0,
                type_simple = 0x1,
                type_object=0x2,
                type_extension=0x4,
                type_simple_object = type_simple | type_object,
                type_simple_extension = type_simple | type_extension,
                type_object_extension = type_object | type_extension,
                type_simple_object_extension = type_simple | type_object_extension};

string convert_type(string const& type_name, state_holder& state, g_type_info& type_detail)
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
                            g_type_info& type_detail)
{
    if (member_type->lexem.rtt == identifier::rtt)
        return convert_type(member_type->lexem.value, state, type_detail);
    else if (member_type->lexem.rtt == keyword_array::rtt &&
             member_type->children.size() == 1 &&
             member_type->children.front()->lexem.rtt == identifier::rtt)
    {
        string type_name = convert_type(member_type->children.front()->lexem.value,
                                        state, type_detail);
        return "array";
    }
    else if (member_type->lexem.rtt == keyword_hash::rtt &&
             member_type->children.size() == 2 &&
             member_type->children.front()->lexem.rtt == identifier::rtt &&
             member_type->children.back()->lexem.rtt == identifier::rtt)
    {
        g_type_info type_detail_key, type_detail_value;
        string key_type_name =
                construct_type_name(member_type->children.front(),
                                    state, type_detail_key);
        string value_type_name =
                construct_type_name(member_type->children.back(),
                                    state, type_detail_value);

        type_detail = static_cast<g_type_info>(type_detail_key | type_detail_value);

        if ((type_detail & type_object) &&
            (type_detail & type_extension))
            throw runtime_error("hash object extension mix");
        return "array";
        //return "std::unordered_map<" + key_type_name  + ", " + value_type_name + ">";
    }
    else
        throw runtime_error("can't get type definition, wtf!");
}
}

string analyze(state_holder& state,
               expression_tree const* pexpression)
{
    size_t rtt = 0;
    string result=R"file_template(
    interface Validator
    {
         public function validate(stdClass $data);
    }
    class Rtt
    {
         CONST types = [)file_template";

    assert(pexpression);

    unordered_map<size_t, string> class_names, type_names;
    string resultMid;
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

                resultMid += analyze_struct(state,
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
    result+="\n";
    for (size_t index = 0; index < max_rtt + 1; ++index)
    {
            result+="\t"+std::to_string(index)+" => '"+class_names[index]+"',\n";

    }
    result += R"file_template(
        ];
        public function validate(string $jsonData)
        {
              $jsonObj = json_decode($jsonData);
              if ($jsonObj === null) {
                    return false;
              }
              if (!isset(Rtt::types[$jsonObj->rtt])) {
                    return false;
              }
              try {
                   $className = Rtt::types[$jsonObj->rtt];
                   /**
                    * @var Validator $class
                    */
                   $class = new $className;
                   $class->validate($jsonObj);
                   return true;
               } catch (Throwable $e) {
                   return $e->getMessage();
               }
            }
          }
    )file_template";

    return result+resultMid;
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

    unordered_set<string> set_object_name;
    unordered_set<string> set_extension_name;

    unordered_map<string, string> map_member_name_type;

    result += "    class " + type_name + " implements Validator \n";
    result += "    {\n";
    string params;
    string setFunction;
    string arrayCase;
    string trivialTypes;
    string objectTypes;

    for (auto member_pair : members)
    {
        auto const& member_name = member_pair.first->lexem;
        auto const& member_type = member_pair.second;
        if (member_name.rtt != identifier::rtt)
            throw runtime_error("use \"variable type\" syntax please");

        g_type_info type_detail;
        string member_type_name = construct_type_name(member_type, state, type_detail);

        if (type_detail & type_object)
            set_object_name.insert(member_name.value);
        else if (type_detail & type_extension)
            set_extension_name.insert(member_name.value);

        params +=
                "        /**\n"
                "        * @var "+ member_type_name + "\n" +
                "        */ \n" +
                "        private $" + member_name.value + ";\n";

        if(member_type_name=="array")
        {
            string item = member_name.value.substr( 0, member_name.value.length()-1);
            arrayCase +=
                        "              foreach ($data->" + member_name.value + " as $" + item + ") { \n"
                        "                  $" + item + "Obj = new " + ((char)(item.at(0)-32) + item.substr( 1, item.length()-1) ) + "() \n"
                        "                  $" + item + "Obj->validate($" + item + "); \n"
                        "               } \n";


        }
        else if (member_type_name=="int" ||
                 member_type_name=="string" ||
                 member_type_name=="bool" ||
                 member_type_name=="float" ||
                 member_type_name=="double")
        {
            trivialTypes+="            $this->set" + ((char)(member_name.value.at(0)-32) + member_name.value.substr( 1,member_name.value.length()-1) ) + "($data->" + member_name.value + "); \n";

            setFunction +=
                        "        /** \n"
                        "        * @param " + member_type_name + " $" + member_name.value + "\n"
                        "        */ \n"
                        "        public function set" + (char)( member_name.value.at(0)-32 ) + member_name.value.substr( 1,member_name.value.length()-1 ) + "(" + member_type_name +" $" + member_name.value + ") \n"
                        "        { \n"
                        "                $this->" + member_name.value + " = $" + member_name.value + "; \n"
                        "        } \n";
        }
        else
        {
            objectTypes +=
                    "            $" + member_name.value + "Obj = new "+member_type_name + "();\n"
                    "            $" + member_name.value + "Obj -> validate($data-> "+member_name.value  + ");\n";

        }

        map_member_name_type.insert(std::make_pair(member_name.value, member_type_name));
    }

    string validation=
            "        public function validate(stdClass $data) \n"
            "        { \n"+
                       objectTypes + trivialTypes + arrayCase +
            "        } \n"
            "    } \n";
    result += params + setFunction + validation;
    return result;
}
