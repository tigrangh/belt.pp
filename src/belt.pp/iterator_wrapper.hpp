#pragma once

#include <iterator>
#include <memory>

namespace beltpp
{
template <typename T>
class iterator_wrapper : public std::iterator<std::input_iterator_tag, T>
{
public:
    template <typename Iter>
    iterator_wrapper(Iter child)
        : m_fpclone([](void* iterator)
                        {
                            return static_cast<void*>(
                                        new Iter(*static_cast<Iter*>(iterator)));
                        })
        , m_fpequal([](void* iterator1, void* iterator2)
                        {
                            return *static_cast<Iter*>(iterator1) ==
                                    *static_cast<Iter*>(iterator2);
                        })
        , m_fpincrement([](void* iterator)
                        {
                            ++(*static_cast<Iter*>(iterator));
                        })
        , m_fpassign([](void* iterator1, void* iterator2)
                        {
                            *static_cast<Iter*>(iterator1) = *static_cast<Iter*>(iterator2);
                        })
        , m_fpdelete([](void* iterator)
                        {
                            delete static_cast<Iter*>(iterator);
                        })
        , m_fpderef([](void* iterator) -> T&
                        {
                            return *(*static_cast<Iter*>(iterator));
                        })
        , m_iterator(new Iter(child), [](void* p)
                        {
                            delete static_cast<Iter*>(p);
                        })
    {}

    iterator_wrapper(iterator_wrapper const& other)
        : m_fpclone(other.m_fpclone)
        , m_fpequal(other.m_fpequal)
        , m_fpincrement(other.m_fpincrement)
        , m_fpassign(other.m_fpassign)
        , m_fpdelete(other.m_fpdelete)
        , m_fpderef(other.m_fpderef)
        , m_iterator(m_fpclone(other.m_iterator.get()), other.m_fpdelete)
    {
    }
    ~iterator_wrapper() {}

    iterator_wrapper& operator = (iterator_wrapper const& other)
    {
        m_fpassign(m_iterator.get(), other.m_iterator.get());
        return *this;
    }

    bool operator != (iterator_wrapper const& other) const noexcept
    {
        return false == m_fpequal(m_iterator.get(), other.m_iterator.get());
    }
    bool operator == (iterator_wrapper const& other) const noexcept
    {
        return m_fpequal(m_iterator.get(), other.m_iterator.get());
    }
    iterator_wrapper& operator ++ () noexcept
    {
        m_fpincrement(m_iterator.get());
        return *this;
    }
    T& operator* () noexcept
    {
        return m_fpderef(m_iterator.get());
    }
    T const& operator* () const noexcept
    {
        return m_fpderef(m_iterator.get());
    }
    T* operator-> () noexcept
    {
        return &m_fpderef(m_iterator.get());
    }
    T const* operator-> () const noexcept
    {
        return &m_fpderef(m_iterator.get());
    }
protected:
    void*(*m_fpclone)(void*);
    bool(*m_fpequal)(void*, void*);
    void(*m_fpincrement)(void*);
    void(*m_fpassign)(void*, void*);
    void(*m_fpdelete)(void*);
    T&(*m_fpderef)(void*);
    std::unique_ptr<void, void(*)(void*)> m_iterator;
};
}
