#include "log.hpp"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace beltpp
{
class log_console : public ilog
{
public:
    log_console(string const& _name)
        : enabled(true)
        , fresh_line(true)
        , str_name(_name)
    {}
    ~log_console() {}

    std::string name() const noexcept override
    {
        return str_name;
    }
    void enable() noexcept override
    {
        enabled = true;
    }
    void disable() noexcept override
    {
        enabled = false;
    }

    void message(std::string const& value) override
    {
        if (false == enabled)
            return;
        message_no_eol(value);
        cerr << endl;
        fresh_line = true;
    }
    void warning(std::string const& value) override
    {
        if (false == enabled)
            return;
        warning_no_eol(value);
        cerr << endl;
        fresh_line = true;
    }
    void error(std::string const& value) override
    {
        if (false == enabled)
            return;
        error_no_eol(value);
        cerr << endl;
        fresh_line = true;
    }

    void message_no_eol(string const& value) override
    {
        if (false == enabled)
            return;
        cout << value << " ";
        fresh_line = false;
    }
    void warning_no_eol(string const& value) override
    {
        if (false == enabled)
            return;
        if (fresh_line)
            cout << "Warning: ";
        cout << value << " ";
        fresh_line = false;
    }
    void error_no_eol(string const& value) override
    {
        if (false == enabled)
            return;
        if (fresh_line)
            cerr << "Error: ";
        cerr << value << " ";
        fresh_line = false;
    }

    bool enabled;
    bool fresh_line;
    string str_name;
};

ilog_ptr console_logger(string const& name)
{
    return beltpp::new_dc_unique_ptr<ilog, log_console>(name);
}
}
