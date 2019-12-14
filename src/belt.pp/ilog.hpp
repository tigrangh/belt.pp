#pragma once

#include "global.hpp"

#include <string>
#include <memory>

namespace beltpp
{
class ilog
{
public:
    virtual ~ilog() {}
    virtual std::string name() const noexcept = 0;
    virtual bool enabled() const noexcept = 0;
    virtual void enable() noexcept = 0;
    virtual void disable() noexcept = 0;

    virtual void message(std::string const& value) = 0;
    virtual void warning(std::string const& value) = 0;
    virtual void error(std::string const& value) = 0;
};

using ilog_ptr = beltpp::t_unique_ptr<ilog>;
}
