#pragma once

#include "global.hpp"

#include "packet.hpp"

#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include <unordered_map>
#include <functional>

namespace beltpp
{
class session
{
    using string = std::string;
    using packet = ::beltpp::packet;
    using time_point = ::std::chrono::steady_clock::time_point;
public:
    packet package_expected;
    packet package_state;
private:
    time_point discard_time;
};

class session_manager
{
    using string = ::std::string;
    using duration = ::std::chrono::steady_clock::duration;
public:
    void set_processor(std::function<void()> const& processor);
    session& store(string const& peerid, duration const& active);
    void drop(string const& peerid);
    bool is_stored(string const& peerid);
    bool take_to_remove(string& peerid);
private:
    std::unordered_map<string, session> sessions;

};
}

