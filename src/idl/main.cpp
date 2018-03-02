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
    string result;
    size_t pos = source.find(lookup);
    if (pos != string::npos)
    {
        result = source;
        result.replace(pos, lookup.length(), replace);
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
                definition.reserve(file_definition.tellg());
                file_definition.seekg(0, std::ios::beg);

                definition.assign((std::istreambuf_iterator<char>(file_definition)),
                                  std::istreambuf_iterator<char>());
                file_definition.close();
            }
        }

        if (definition.empty())
        {
            definition = "package beltpp "
                         "type message_join struct {}"
                         "type message_drop struct {}"
                         "type message_error struct {}"
                         "type message_timer_out struct {}";
        }

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
            throw runtime_error("syntax error, maybe: " +
                                string(it_begin, it_end));

        if (ptr_expression->depth() > 30)
            throw runtime_error("expected tree max depth 30 is exceeded");

        state_holder state;
        //cout << beltpp::dump(ptr_expression.get()) << endl;
        string file_contents = resources::file_template;
        string generated = analyze(state, ptr_expression.get());
        file_contents = replace(file_contents, "{namespace_name}", "beltpp");
        file_contents = replace(file_contents, "{expand_message_classes}", generated);

        bool generation_success = false;
        if (argc >= 3)
        {
            ofstream file_generate(argv[2]);
            if (file_generate)
            {
                file_generate << file_contents;
                file_generate.close();
                generation_success = true;
            }
        }
        if (false == generation_success)
            cout << file_contents;

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
