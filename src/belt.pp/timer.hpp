#pragma once

#include <chrono>

namespace beltpp
{

class timer
{
public:
    timer()
        : is_set(false)
        , last_point_expired()
        , timer_period()
    {}

    static std::chrono::steady_clock::time_point now()
    {
        return std::chrono::steady_clock::now();
    }

    void set(std::chrono::steady_clock::duration const& period, bool delay = false)
    {
        is_set = true;
        if (delay)
            last_point_expired = now();
        else
            last_point_expired = now() - period;
        timer_period = period;
    }

    void clean()
    {
        is_set = false;
    }

    void update()
    {
        while (expired())
            last_point_expired += timer_period;
    }

    bool expired() const
    {
        return is_set && (now() - last_point_expired >= timer_period);
    }

    bool is_set;
    std::chrono::steady_clock::time_point last_point_expired;
    std::chrono::steady_clock::duration timer_period;
};
}
