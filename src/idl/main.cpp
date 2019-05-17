#include "generator.hpp"
#include "resources.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <exception>
#include <streambuf>
#include <cassert>

using std::cout;
using std::endl;
using std::string;
using std::runtime_error;
using std::ofstream;
using std::ifstream;
using std::vector;

using ptr_expression_tree = std::unique_ptr<expression_tree>;

string replace(string const& source,
               string const& lookup,
               string const& replace)
{
    string result = source;
    size_t pos = source.find(lookup);
    if (pos != string::npos)
        result.replace(pos, lookup.length(), replace);

    return result;
}

string replace_all(string const& source,
                   string const& lookup,
                   string const& replace)
{
    string result = source;
    while (true)
    {
        size_t pos = result.find(lookup);
        if (pos != string::npos)
            result.replace(pos, lookup.length(), replace);
        else
            break;
    }

    return result;
}

int main(int argc, char* argv[])
{
    string definition;
    ptr_expression_tree ptr_expression;
    try
    {
        if (argc >= 2)
        {
            ifstream file_definition(argv[1]);
            if (file_definition)
            {
                file_definition.seekg(0, std::ios::end);
                definition.reserve(size_t(file_definition.tellg()));
                file_definition.seekg(0, std::ios::beg);

                definition.assign((std::istreambuf_iterator<char>(file_definition)),
                                  std::istreambuf_iterator<char>());
                file_definition.close();
            }
        }

        if (definition.empty())
        {
            definition =
                    R"text(
                    module beltpp
                    {
                        class test
                        {
                            Array Array Int hello
                        }
                        class message_join {}
                        class message_drop {}
                        class message_error {}
                        class message_timer_out {}
                    }
                    )text";
        }

        auto it_begin = definition.begin();
        auto it_end = definition.end();
        auto it_begin_keep = it_begin;
        while (beltpp::e_three_state_result::success ==
               beltpp::parse(ptr_expression, it_begin, it_end))
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
            throw runtime_error("syntax error, maybe: " +
                                string(it_begin, it_end));

        if (ptr_expression->depth() > 30)
            throw runtime_error("expected tree max depth 30 is exceeded");

        state_holder state;
        //cout << beltpp::dump(ptr_expression.get()) << endl;
        string file_contents_declarations = resources::file_declarations;
        string file_contents_template_definitions = resources::file_template_definitions;
        string file_contents_definitions = resources::file_definitions;
        generated_code generated = analyze(state, ptr_expression.get());
        file_contents_declarations = replace_all(file_contents_declarations,
                                                 "{namespace_name}",
                                                 state.namespace_name);
        file_contents_template_definitions = replace_all(file_contents_template_definitions,
                                                         "{namespace_name}",
                                                         state.namespace_name);
        file_contents_definitions = replace_all(file_contents_definitions,
                                                "{namespace_name}",
                                                state.namespace_name);

        file_contents_declarations = replace(file_contents_declarations,
                                             "{expand_message_classes_declarations}",
                                             generated.declarations);
        file_contents_template_definitions = replace(file_contents_template_definitions,
                                                     "{expand_message_classes_template_definitions}",
                                                     generated.template_definitions);
        file_contents_definitions = replace(file_contents_definitions,
                                            "{expand_message_classes_definitions}",
                                            generated.definitions);

        bool generation_success = false;
        bool splitting = false, exporting = false;

        if (argc >= 5)
            exporting = true;
        if (argc >= 4)
            splitting = true;

        if (false == splitting)
            exporting = false;

        string keyword;
        if (exporting)
            keyword = string(argv[4]) + " ";

        if (splitting)
        {
            file_contents_declarations = replace_all(file_contents_declarations, "inline ", "");
            file_contents_declarations = replace_all(file_contents_declarations, "inline\n", "");
            file_contents_template_definitions = replace_all(file_contents_template_definitions, "inline ", "");
            file_contents_template_definitions = replace_all(file_contents_template_definitions, "inline\n", "");
        }

        file_contents_declarations = replace_all(file_contents_declarations, "{export} ", keyword);
        file_contents_template_definitions = replace_all(file_contents_template_definitions, "{export} ", keyword);

        if (argc >= 3)
        {
            string file_name = argv[2];
            if (false == splitting)
            {
                ofstream file_generate(file_name + ".hpp");
                if (file_generate)
                {
                    file_generate << file_contents_declarations
                                  << file_contents_template_definitions
                                  << file_contents_definitions;
                    file_generate.close();
                    generation_success = true;
                }
            }
            else
            {
                ofstream file_declarations(file_name + ".hpp");
                ofstream file_template_definitions(file_name + ".tmpl.hpp");
                ofstream file_definitions(file_name + ".cpp.hpp");
                if (file_declarations &&
                    file_template_definitions &&
                    file_definitions)
                {
                    file_declarations << file_contents_declarations;
                    file_template_definitions << file_contents_template_definitions;
                    file_definitions << file_contents_definitions;

                    file_declarations.close();
                    file_template_definitions.close();
                    file_definitions.close();

                    generation_success = true;
                }
            }
        }
        if (false == generation_success)
            cout << file_contents_declarations << file_contents_definitions;

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
