#pragma once

#include <exception>
#include <type_traits>

namespace beltpp
{

namespace detail
{
template <typename t_type>
class hide_void
{
public:
    typedef t_type type;
};

template <>
class hide_void<void>
{
    class internal;
public:
    typedef internal type;
};
}
/*
 *
 *  delegate
 *
 */
template <typename ret_t, typename... types>
class delegate
{
    typedef ret_t(*fptr_wr_t)(types...);
    typedef ret_t(*mfptr_wr_t)(void*, types...);
    typedef ret_t(*cmfptr_wr_t)(void const*, types...);

    using noref_ret_t = typename detail::hide_void<typename std::remove_reference<ret_t>::type>::type;

    typedef ret_t(*fptr_t)(types...);

    template <typename class_t>
    using mfptr_t = ret_t(class_t::*)(types...);
    template <typename class_t>
    using cmfptr_t = ret_t(class_t::*)(types...) const;
    template <typename class_t>
    using mptr_t = noref_ret_t (class_t::*);

public:
    void init()
    {
        m_pob = nullptr;
        m_fptr = nullptr;
        m_mfptr = nullptr;
        m_cmfptr = nullptr;
    }

private:
    template <fptr_t fptr>
    static ret_t fptr_wrapper(types... args)
    {
        return fptr(args...);
    }
public:
    template <fptr_t fptr>
    void bind_fptr()
    {
        init();
        m_fptr = &delegate<ret_t, types...>::fptr_wrapper<fptr>;
    }

private:
    template <typename class_t, mfptr_t<class_t> mfptr>
    static ret_t mfptr_wrapper(class_t* pob, types... args)
    {
        return (pob->*mfptr)(args...);
    }
public:
    template <typename class_t, mfptr_t<class_t> mfptr>
    void bind_mfptr(class_t& ob)
    {
        init();
        ret_t(*p_mfptr_wr)(class_t*, types...);
        p_mfptr_wr = &delegate<ret_t, types...>::mfptr_wrapper<class_t, mfptr>;
        m_mfptr = reinterpret_cast<mfptr_wr_t>(p_mfptr_wr);
        m_pob = &ob;
    }

private:
    template <typename class_t, mptr_t<class_t> mptr>
    static ret_t mptr_wrapper(class_t* pob, types... /*args*/)
    {
        return (pob->*mptr);
    }
public:
    template <typename class_t, mptr_t<class_t> mptr,
              typename ret_t_dup = ret_t,
              typename std::enable_if<!std::is_same<ret_t_dup, void>::value, char>::type = 0>
    void bind_mptr(class_t& ob)
    {
        init();
        ret_t(*p_mptr_wr)(class_t*);
        p_mptr_wr = &delegate<ret_t, types...>::mptr_wrapper<class_t, mptr>;
        m_mfptr = reinterpret_cast<mfptr_wr_t>(p_mptr_wr);
        m_pob = &ob;
    }

private:
    template <typename class_t, cmfptr_t<class_t> cmfptr>
    static ret_t cmfptr_wrapper(class_t const* pob, types... args)
    {
        return (pob->*cmfptr)(args...);
    }
public:
    template <typename class_t, cmfptr_t<class_t> cmfptr>
    void bind_cmfptr(class_t const& ob)
    {
        init();
        ret_t(*p_cmfptr_wr)(class_t const*, types...);
        p_cmfptr_wr = &delegate<ret_t, types...>::cmfptr_wrapper<class_t, cmfptr>;
        m_cmfptr = reinterpret_cast<cmfptr_wr_t>(p_cmfptr_wr);
        m_pob = &ob;
    }

public:
    ret_t operator() (types... args)
    {
        if (m_fptr)
            return m_fptr(args...);
        else if (m_mfptr)
            return m_mfptr(const_cast<void*>(m_pob), args...);
        else if (m_cmfptr)
            return m_cmfptr(m_pob, args...);

        throw std::logic_error("uninitialized delegate");
    }

private:
    void const* m_pob;
    fptr_wr_t m_fptr;
    mfptr_wr_t m_mfptr;
    cmfptr_wr_t m_cmfptr;
};

}

