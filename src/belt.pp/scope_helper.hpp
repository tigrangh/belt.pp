#pragma once

#include <functional>

namespace beltpp
{
class on_failure final
{
public:
    on_failure()
        : m_cleanup()
    {}

    on_failure(std::function<void()> const& cleanup)
        : m_cleanup(cleanup)
    {}
    on_failure(std::function<void()>&& cleanup)
        : m_cleanup(std::move(cleanup))
    {}

    on_failure(on_failure const&) = delete;
    on_failure(on_failure&& other)
        : m_cleanup(std::move(other.m_cleanup))
    {
        other.dismiss();
    }

    ~on_failure()
    {
        if (m_cleanup)
            m_cleanup();
    }

    void operator = (on_failure const&) = delete;
    void operator = (on_failure&& other)
    {
        if (m_cleanup)
            m_cleanup();

        m_cleanup = std::move(other.m_cleanup);
        other.dismiss();
    }

    void dismiss() noexcept
    {
        m_cleanup = std::function<void()>();
    }

private:
    std::function<void()> m_cleanup;
};

class finally final
{
public:
    finally()
    {}

    finally(std::function<void()> const& cleanup)
        : m_cleanup(cleanup)
    {}
    finally(std::function<void()>&& cleanup)
        : m_cleanup(std::move(cleanup))
    {}

    finally(finally const&) = delete;
    finally(finally&& other)
        : m_cleanup(std::move(other.m_cleanup))
    {
        other.m_cleanup = std::function<void()>();
    }

    ~finally()
    {
        if (m_cleanup)
            m_cleanup();
    }

    void operator = (finally const&) = delete;
    void operator = (finally&& other)
    {
        if (m_cleanup)
            m_cleanup();

        m_cleanup = std::move(other.m_cleanup);
        other.m_cleanup = std::function<void()>();
    }

private:
    std::function<void()> m_cleanup;
};

}
