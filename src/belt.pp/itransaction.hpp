#pragma once

namespace beltpp
{
class itransaction
{
public:
    virtual ~itransaction() {}

    virtual void commit() noexcept = 0;
    virtual void rollback() noexcept = 0;
};
}
