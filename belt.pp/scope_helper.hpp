#ifndef BELT_SCOPE_HELPER_H
#define BELT_SCOPE_HELPER_H

#include <functional>

namespace beltpp
{
class scope_helper final
{
public:
    scope_helper()
    {}

    scope_helper(std::function<void()> const& init,
                 std::function<void()> const& cleanup)
    {
        if (init)
            init();

        m_cleanup = cleanup;
    }

    scope_helper(scope_helper const&) = delete;
    scope_helper(scope_helper&& other)
    {
        m_cleanup = other.m_cleanup;
        other.commit();
    }

    ~scope_helper()
    {
        if (m_cleanup)
            m_cleanup();
    }

    void operator = (scope_helper const&) = delete;
    void operator = (scope_helper&& other)
    {
        if (m_cleanup)
            m_cleanup();

        m_cleanup = other.m_cleanup;
        other.commit();
    }

    void commit() noexcept
    {
        m_cleanup = std::function<void()>();
    }

private:
    std::function<void()> m_cleanup;
};

}

#endif // BELT_SCOPE_HELPER_H
