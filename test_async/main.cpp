#include <iostream>
#include <chrono>

#include <async/multithreading.h>
#include <delegate.h>

void task(int i)
{
    if (i > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(i));
    /*volatile int count = i;
    for (int a = 0; a < count; ++a)
    {
        ++a;
    }*/
}

class ctask
{
public:
    void mem() const
    {
        task(i);
    }

    void operator()() const
    {
        task(i);
    }

    int i;
};

//using task_t = std::function<void()>;
//using task_t = delegate<void>;
using task_t = ctask;

void test_async_processor(size_t count,
                          size_t threads,
                          int unitWeight,
                          bool bUseWaits,
                          long& mswait,
                          long& msdestruct)
{
    /*
     * test with 100 mln tasks and 100 mln waits
     * takes 59-63 seconds
     * without waits its about 54 seconds
     *
     * a similar test on qt with 10 mln signal-slot <=> signal-slot takes 49 seconds
     *
     */
    namespace chrono = std::chrono;
    using steady_clock = chrono::steady_clock;
    using time_point = steady_clock::time_point;
    time_point tp_start = steady_clock::now();
    time_point tp_wait;
    time_point tp_destruct;
    {
        async::processor<task_t> proc(threads);

        ctask ttob;
        ttob.i = unitWeight;
        task_t tt;
        //tt.bind_cmfptr<ctask, &ctask::mem>(ttob);
        tt = ttob;

        for (size_t i = 0; i < count; ++i)
        {
            //proc.run([unitWeight]{task(unitWeight);});
            proc.run(tt);

            if (bUseWaits &&
                0 == i % threads)
            {
                proc.wait();
            }
        }

        proc.wait();
        tp_wait = steady_clock::now();
        steady_clock::duration elapsed = tp_wait - tp_start;
        chrono::milliseconds ms_elapsed = chrono::duration_cast<chrono::milliseconds>(elapsed);
        mswait = ms_elapsed.count();
    }

    tp_destruct = steady_clock::now();
    {
        steady_clock::duration elapsed = tp_destruct - tp_wait;
        chrono::milliseconds ms_elapsed = chrono::duration_cast<chrono::milliseconds>(elapsed);
        msdestruct = ms_elapsed.count();
    }
}

int main()
{
    int const sleep = -1;
    size_t count = 1e6;
    long ms_wait, ms_destruct;

    for (size_t threads = 1; threads <= 10; ++threads)
    {
        test_async_processor(count,
                             threads,
                             sleep,
                             false,
                             ms_wait,
                             ms_destruct);
        std::cout << ms_wait << '\t';// << ms_destruct << std::endl;
        test_async_processor(count,
                             threads,
                             sleep,
                             true,
                             ms_wait,
                             ms_destruct);
        std::cout << ms_wait << /*'\t' << ms_destruct << */std::endl;
    }

    return 0;
}
