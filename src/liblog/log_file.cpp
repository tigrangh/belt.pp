#include "log.hpp"

#include <belt.pp/utility.hpp>

#include <iostream>
#include <fstream>
#include <chrono>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;

namespace chrono = std::chrono;
using chrono::system_clock;

namespace beltpp
{
class log_file : public ilog
{
public:
    log_file(string const& name, string const& file_name)
        : enabled(true)
        , str_name(name)
        , file_name(file_name)
    {
        ofstream of;
        of.open(file_name, std::ios_base::app);
        if (!of)
            throw std::runtime_error("cannot open file: " + file_name);
    }
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

        ofstream of;
        of.open(file_name, std::ios_base::app);
        if (!of)
            return;

        std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
        of << beltpp::gm_time_t_to_lc_string(time_t_now) << " - ";
        of << value << endl;
    }
    void warning(std::string const& value) override
    {
        if (false == enabled)
            return;

        ofstream of;
        of.open(file_name, std::ios_base::app);
        if (!of)
            return;

        std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
        of << beltpp::gm_time_t_to_lc_string(time_t_now) << " - ";
        of << "Warning: " << value << endl;
    }
    void error(std::string const& value) override
    {
        if (false == enabled)
            return;

        ofstream of;
        of.open(file_name, std::ios_base::app);
        if (!of)
            return;

        std::time_t time_t_now = system_clock::to_time_t(system_clock::now());
        of << beltpp::gm_time_t_to_lc_string(time_t_now) << " - ";
        of << "Error:   " << value << endl;
    }

    bool enabled;
    string str_name;
    string file_name;
};

ilog_ptr file_logger(string const& name, string const& fname)
{
    return beltpp::new_dc_unique_ptr<ilog, log_file>(name, fname);
}
}
