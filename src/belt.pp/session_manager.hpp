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
template <typename T_key, typename T_value>
class session
{
    using packet = ::beltpp::packet;
    using time_point = ::std::chrono::steady_clock::time_point;
public:
    size_t expected_package_type;
    T_key key;
    T_value value;
    time_point discard_after;
};
}

