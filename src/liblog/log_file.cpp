#include "log.hpp"

#include <belt.pp/utility.hpp>

#include <iostream>
#include <fstream>
#include <chrono>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace chrono = std::chrono;
using chrono::system_clock;

namespace beltpp
{
class log_file : public ilog
{
public:
    log_file(string const& _name, string const& _fname)
        : enabled(true)
        , fresh_line(true)
        , str_name(_name)
        , file_name(_fname)
    { }
    ~log_file() override {}

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

        fresh_line = true;
    }
    void warning(std::string const& value) override
    {
        if (false == enabled)
            return;

        warning_no_eol(value);

        fresh_line = true;
    }
    void error(std::string const& value) override
    {
        if (false == enabled)
            return;

        error_no_eol(value);

        fresh_line = true;
    }

    void message_no_eol(string const& value) override
    {
        if (false == enabled)
            return;

        my_file.open(file_name, std::ios::app);
        if(!my_file.is_open())
            return;

        //print the current system time and value into the given file
        std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
        my_file << beltpp::gm_time_t_to_lc_string(time_t_now) << "\t";

        my_file << value << "\n";
        my_file.close();
        //cout << value << " ";
        fresh_line = false;
    }
    void warning_no_eol(string const& value) override
    {
        if (false == enabled)
            return;

        my_file.open(file_name, std::ios::app);
        if(!my_file.is_open())
            return;

        if (fresh_line)
        {
            std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
            my_file << beltpp::gm_time_t_to_lc_string(time_t_now) << "\t";
            my_file << "Warning: ";
        }
        else
            my_file << "\t\t\t\t ";

        my_file << value << "\n";
        my_file.close();
        //cout << value << " ";
        fresh_line = false;
    }
    void error_no_eol(string const& value) override
    {
        if (false == enabled)
            return;

        my_file.open(file_name, std::ios::app);
        if(!my_file.is_open())
            return;

        if (fresh_line)
        {
            std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
            my_file << beltpp::gm_time_t_to_lc_string(time_t_now) << "\t";
            my_file << "Error:   ";
        }
        else
            my_file << "\t\t\t\t ";

        my_file << value << "\n";
        my_file.close();
        //cerr << value << " ";
        fresh_line = false;
    }

    bool enabled;
    bool fresh_line;
    string str_name;
    string file_name;
    std::ofstream my_file;
};

ilog_ptr file_logger(string const& name, string const& fname)
{
    return beltpp::new_dc_unique_ptr<ilog, log_file>(name, fname);
}
}
