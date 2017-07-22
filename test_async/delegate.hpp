#ifndef DELEGATE_H
#define DELEGATE_H

template <typename ret_t, typename... types>
class delegate
{
    typedef ret_t(*fptr_t)(types...);
    typedef ret_t(*fptr_wr_t)(types...);
    typedef ret_t(*mfptr_wr_t)(void*, types...);
    typedef ret_t(*cmfptr_wr_t)(void const*, types...);
public:
    void init()
    {
        m_pob = nullptr;
        m_fptr = nullptr;
        m_mfptr = nullptr;
        m_cmfptr = nullptr;
    }

    template <fptr_t fptr>
    static ret_t fptr_wrapper(types... args)
    {
        return fptr(args...);
    }
    template <fptr_t fptr>
    void bind_fptr()
    {
        init();
        m_fptr = &delegate<ret_t, types...>::fptr_wrapper<fptr>;
    }

    template <typename class_t, ret_t(class_t::*mfptr)(types...)>
    static ret_t mfptr_wrapper(class_t* pob, types... args)
    {
        return (pob->*mfptr)(args...);
    }
    template <typename class_t, ret_t(class_t::*mfptr)(types...)>
    void bind_mfptr(class_t& ob)
    {
        init();
        ret_t(*p_mfptr_wr)(class_t*, types...);
        p_mfptr_wr = &delegate<ret_t, types...>::mfptr_wrapper<class_t, mfptr>;
        m_mfptr = reinterpret_cast<mfptr_wr_t>(p_mfptr_wr);
        m_pob = &ob;
    }

    template <typename class_t, ret_t(class_t::*cmfptr)(types...) const>
    static ret_t cmfptr_wrapper(class_t const* pob, types... args)
    {
        return (pob->*cmfptr)(args...);
    }
    template <typename class_t, ret_t(class_t::*cmfptr)(types...) const>
    void bind_cmfptr(class_t const& ob)
    {
        init();
        ret_t(*p_cmfptr_wr)(class_t const*, types...);
        p_cmfptr_wr = &delegate<ret_t, types...>::cmfptr_wrapper<class_t, cmfptr>;
        m_cmfptr = reinterpret_cast<cmfptr_wr_t>(p_cmfptr_wr);
        m_pob = &ob;
    }

    ret_t operator() (types... args)
    {
        if (m_fptr)
            return m_fptr(args...);
        else if (m_mfptr)
            return m_mfptr(const_cast<void*>(m_pob), args...);
        else if (m_cmfptr)
            return m_cmfptr(m_pob, args...);
        return ret_t();
    }

private:
    void const* m_pob;
    fptr_wr_t m_fptr;
    mfptr_wr_t m_mfptr;
    cmfptr_wr_t m_cmfptr;
};

#endif // DELEGATE_H
