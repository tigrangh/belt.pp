#include "log.hpp"

#include <belt.pp/utility.hpp>

#include <iostream>
#include <chrono>

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
namespace chrono = std::chrono;
using chrono::system_clock;

namespace beltpp
{
namespace
{
    std::string time_now()
    {
        std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
        string str = beltpp::gm_time_t_to_lc_string(time_t_now);
        return str.substr(string("0000-00-00 ").length());
    }
}
class log_console : public ilog
{
public:
    log_console(string const& name, bool print_time)
        : enabled_(true)
        , print_time(print_time)
        , str_name(name)
    {}
    ~log_console() override {}

    std::string name() const noexcept override
    {
        return str_name;
    }
    bool enabled() const noexcept override
    {
        return enabled_;
    }
    void enable() noexcept override
    {
        enabled_ = true;
    }
    void disable() noexcept override
    {
        enabled_ = false;
    }

    void message(std::string const& value) override
    {
        if (false == enabled_)
            return;
        if (print_time)
            cout << time_now() << " - ";

        cout << value << endl;
    }
    void warning(std::string const& value) override
    {
        if (false == enabled_)
            return;
        if (print_time)
            cout << time_now() << " - ";

        cout << "Warning: " << value << endl;
    }
    void error(std::string const& value) override
    {
        if (false == enabled_)
            return;
        if (print_time)
            cout << time_now() << " - ";

        cerr << "Error:   " << value << endl;
    }

    bool enabled_;
    bool print_time;
    string str_name;
};

ilog_ptr console_logger(string const& name, bool print_time)
{
    log_console* p = new log_console(name, print_time);
    return beltpp::ilog_ptr(p);
}
}
