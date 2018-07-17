#include <belt.pp/json.hpp>

#include <iostream>
#include <string>

#include <vector>
#include <numeric>
#include <iterator>

using std::cout;
using std::endl;
using std::string;
using std::vector;

void json_parse(string const& buf)
{
    bool isValid{false};

    beltpp::iterator_wrapper<char const> iter_scan_begin(buf.begin());
    beltpp::iterator_wrapper<char const> iter_scan_end(buf.end());


    ::beltpp::json::ptr_expression_tree pexp;
    ::beltpp::json::expression_tree* proot = nullptr;

    auto code = ::beltpp::json::parse_stream(pexp,
                                             iter_scan_begin,
                                             iter_scan_end,
                                             1024*1024,
                                             proot);

    if (::beltpp::e_three_state_result::success == code &&
        nullptr == pexp)
    {
        assert(false);
        //  this should not happen, but since this is potentially a
        //  network protocol code, let's assume there is a bug
        //  and not crash, not throw, only report an error
        code = ::beltpp::e_three_state_result::error;
    }
    else if (::beltpp::e_three_state_result::success == code)
    {
        isValid = true;
        //cout << "The parsing SUCCESS..." << endl;
    }
    //
    //
    if (::beltpp::e_three_state_result::error == code)
    {
        isValid = false;
        //cout << "The parsing is FAILED!" << endl;
    }
    else if (::beltpp::e_three_state_result::attempt == code)
    {
        isValid = false;
        //cout << "ATTEMPT to parsing..." << endl;
    }

    if(!isValid)
    {
        cout << "The JSON string is invalid..." << endl;
        cout << buf << endl;
        cout << beltpp::dump(proot);
        cout << "--------------------------------------------------------\n\n";
    }
}


int main()
{
    vector<string> strVect;

    strVect.push_back("{\"Status\": \"Inside string \"exist double\" quotes\"}");
    strVect.push_back("{\"escape_character\":\"Escape sequences 1\t2\n3\"}");
    strVect.push_back("{\"type\":\"double\", \"value\": 1e-4}");

    for(auto x:strVect)
        json_parse(x);

    //string tempStr = R"FF({"some-dbl": "10."})FF";
    //json_parse(tempStr);

    return 0;
}
