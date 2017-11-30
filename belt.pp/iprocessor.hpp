#ifndef BELT_IPROCESSOR_H
#define BELT_IPROCESSOR_H

#include <cstddef>

namespace beltpp
{
template <typename task_t>
class iprocessor
{
public:
    iprocessor() = default;
    iprocessor(size_t count) = delete;
    iprocessor(iprocessor const&) = delete;
    iprocessor(iprocessor&&) = delete;
    virtual ~iprocessor() = default;

    iprocessor& operator = (iprocessor const&) = delete;
    iprocessor& operator = (iprocessor&&) = delete;

    virtual void setCoreCount(size_t count) = 0;
    virtual size_t getCoreCount() = 0;
    virtual void run(task_t const& th) = 0;  // add
    virtual void wait(size_t i_task_count = 0) = 0;
};

}

#endif // BELT_IPROCESSOR_H
