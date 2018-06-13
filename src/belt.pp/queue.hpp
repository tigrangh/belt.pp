#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include <iterator>

namespace beltpp
{

namespace detail
{
    uint64_t capacity2pow(uint64_t capacity);
    template <typename T, bool _CONST>
    class iterator_helper;
}

template <typename T>
class queue
{
public:
    class iterator;
    class const_iterator;

    queue(size_t capacity = 0,
          size_t end_index = 0,
          size_t size = 0)
        : m_i_start(0)
        , m_i_size(size)
        , m_i_loops(0)
        , m_vec_queue(detail::capacity2pow(capacity))
    {
        capacity = m_vec_queue.size();
        assert(size <= capacity);
        if (size > capacity)
            throw std::runtime_error("queue()");

        //  1. the biggest possible capacity will be 2^63 for 64 bit size_t
        //  2. we allow end_index to overflow here, for that reason it's
        //      essential that capacity be power of two

        //  need to set m_i_start and m_i_loop so that
        //  m_i_size + m_i_start + m_i_loop * m_vec_queue.size() = end_index
        if (m_vec_queue.size() && end_index)
        {
            m_i_start = (end_index - m_i_size) % capacity;
            m_i_loops = (end_index - m_i_start - m_i_size) / capacity;
        }
    }

    queue(queue const& ob) = default;

    queue(queue&& ob)
        : m_i_start(ob.m_i_start)
        , m_i_size(ob.m_i_size)
        , m_i_loops(ob.m_i_loops)
        , m_vec_queue(std::move(ob.m_vec_queue))
    {}

    inline queue& operator = (queue const& ob) = default;

    inline queue& operator = (queue&& ob) noexcept
    {
        m_i_loops = ob.m_i_loops;
        m_i_size = ob.m_i_size;
        m_i_start = ob.m_i_start;
        m_vec_queue = std::move(ob.m_vec_queue);

        ob.m_i_loops = 0;
        ob.m_i_size = 0;
        ob.m_i_start = 0;

        return *this;
    }

    inline size_t begin_index() const
    {
        return m_i_start + m_i_loops * m_vec_queue.size();
    }

    inline size_t end_index() const
    {
        return m_i_size + m_i_start + m_i_loops * m_vec_queue.size();
    }

    inline iterator begin()
    {
        return iterator(this, begin_index());
    }

    inline const_iterator begin() const
    {
        return const_iterator(this, begin_index());
    }

    inline const_iterator cbegin() const
    {
        return begin();
    }

    inline iterator end()
    {
        return iterator(this, end_index());
    }

    inline const_iterator end() const
    {
        return const_iterator(this, end_index());
    }

    inline const_iterator cend() const
    {
        return end();
    }

    inline bool valid_index(size_t index) const noexcept
    {
        if (begin_index() < end_index() &&
            index >= begin_index() &&
            index < end_index())
            return true;

        if (begin_index() > end_index() &&
            (
                index >= begin_index() ||
                index < end_index()
            ))
            return true;

        return false;
    }

    inline void push(T const& task)
    {
        reserve();

        size_t i_end = m_i_start + m_i_size;
        if (i_end >= m_vec_queue.size())
            i_end -= m_vec_queue.size();
        assert(i_end < m_vec_queue.size());

        m_vec_queue[i_end] = task;
        ++m_i_size;
    }

    inline void pop() noexcept
    {
        assert(m_i_size);
        if (0 == m_i_size)
            std::terminate();

        --m_i_size;
        ++m_i_start;
        if (m_i_start == m_vec_queue.size())
        {
            m_i_start = 0;
            ++m_i_loops;
        }
    }

    inline bool empty() const
    {
        return 0 == m_i_size;
    }

    inline bool full() const
    {
        return m_i_size == m_vec_queue.size();
    }

    inline size_t size() const
    {
        return m_i_size;
    }

    inline T& front() noexcept
    {
        assert(m_i_size);
        if (0 == m_i_size)
            std::terminate();

        return m_vec_queue[m_i_start];
    }

    inline T const& front() const noexcept
    {
        assert(m_i_size);
        if (0 == m_i_size)
            std::terminate();

        return m_vec_queue[m_i_start];
    }

    inline void reserve()
    {
        if (m_i_size == m_vec_queue.size())
        {
            size_t newsize = m_i_size;
            if (0 == newsize)
                newsize = 16;
            newsize *= 2;

            beltpp::queue<T> newqueue(newsize, end_index(), m_i_size);
            assert(newqueue.end_index() == end_index());
            assert(newqueue.begin_index() == begin_index());

            for (size_t index = begin_index(); index != end_index(); ++index)
            {
                newqueue[index] = operator[](index);
            }

            *this = std::move(newqueue);
        }
    }

    inline void get_free_range(T* &p_begin, size_t& size)
    {
        p_begin = nullptr;
        size = 0;

        if (m_i_size == m_vec_queue.size())
            throw std::runtime_error("queue<>::get_free_range()");

        size_t i_free_buffer_start = m_i_start + m_i_size;
        if (i_free_buffer_start >= m_vec_queue.size())
        {
            i_free_buffer_start -= m_vec_queue.size();
            p_begin = &m_vec_queue[i_free_buffer_start];
            size = m_i_start - i_free_buffer_start;
        }
        else
        {
            p_begin = &m_vec_queue[i_free_buffer_start];
            size = m_vec_queue.size() - i_free_buffer_start;
        }
        assert(i_free_buffer_start < m_vec_queue.size());
    }

    inline T& operator [](size_t index) noexcept
    {
        assert(valid_index(index));

        if (false == valid_index(index))
            std::terminate();

        return m_vec_queue[index % m_vec_queue.size()];
    }

    inline T const& operator [](size_t index) const noexcept
    {
        assert(valid_index(index));

        if (false == valid_index(index))
            std::terminate();

        return m_vec_queue[index % m_vec_queue.size()];
    }


private:
    size_t m_i_start;
    size_t m_i_size;
    size_t m_i_loops;
    std::vector<T> m_vec_queue;
    //
    //  iterator classes
    //
public:
    template <bool _CONST>
    class iterator_template :
            public std::iterator<std::bidirectional_iterator_tag, T>
    {
        using parent_ptr =
            typename detail::iterator_helper<T, _CONST>::parent_ptr;
        using ref = typename detail::iterator_helper<T, _CONST>::ref;
        using ptr = typename detail::iterator_helper<T, _CONST>::ptr;

        friend class queue<T>;
    private:
        iterator_template(parent_ptr parent,
                          size_t index)
            : m_index(index)
            , m_parent(parent) {}

        size_t m_index;
        parent_ptr m_parent;

    public:
        bool operator != (iterator_template const& other) const noexcept
        {
            if (m_parent != other.m_parent ||
                m_index != other.m_index)
                return true;
            else
                return false;
        }

        bool operator == (iterator_template const& other) const noexcept
        {
            return !this->operator !=(other);
        }

        iterator_template& operator ++ () noexcept
        {
            if (this->operator !=(m_parent->end()))
                ++m_index;

            return *this;
        }

        ref operator* () noexcept
        {
            if (false == (this->operator != (m_parent->end())))
            {
                assert(false);
                std::terminate();
            }

            return m_parent->operator [](m_index);
        }

        ptr operator-> () noexcept
        {
            if (false == (this->operator != (m_parent->end())))
            {
                assert(false);
                std::terminate();
            }

            return &m_parent->operator [](m_index);
        }
    };

    class iterator : public iterator_template<false>
    {
    public:
        using parent_ptr =
            typename detail::iterator_helper<T, false>::parent_ptr;
        iterator(parent_ptr parent,
                  size_t index) :
            iterator_template<false>(parent, index) {}
    };

    class const_iterator : public iterator_template<true>
    {
    public:
        using parent_ptr =
            typename detail::iterator_helper<T, true>::parent_ptr;
        const_iterator(parent_ptr parent,
                       size_t index) :
            iterator_template<true>(parent, index) {}

    };
};

namespace detail
{
    inline uint64_t next2pow(uint64_t number)
    {
        uint64_t p2 = 1;
        while (number > p2)
            p2 *= 2;

        return p2;
    }

    inline uint64_t capacity2pow(uint64_t capacity)
    {
        if (0 != capacity)
            capacity = next2pow(capacity);

        return capacity;
    }

    template <typename T, bool _CONST>
    class iterator_helper
    {
    public:
        using parent_ptr = queue<T>*;
        using ref = T&;
        using ptr = T*;
    };

    template <typename T>
    class iterator_helper<T, true>
    {
    public:
        using parent_ptr = queue<T> const*;
        using ref = T const&;
        using ptr = T const*;
    };
}

}

