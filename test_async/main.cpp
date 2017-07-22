#include <iostream>
#include <chrono>
#include <thread>

//#include <async/multithreading.h>
#include <processor.hpp>
//#include "delegate.hpp"

//using task_t = std::function<void()>;
//using task_t = delegate<void>;
using task_t = beltpp::ctask;

void test_async_processor(size_t count,
                          size_t threads,
                          int unitWeight,
                          bool bUseWaits,
                          long& mswait,
                          long& msdestruct)
{
    /*
     * test with 10 mln tasks and 10 mln waits takes 35-36 seconds
     * test with 100 mln tasks takes 29-34 seconds
     * test with 100 mln tasks and 10 mln waits takes 39 seconds
     * test with 100 mln tasks and 20 mln waits takes 75 seconds
     * qt test 10 mln signal-slot <=> signal-slot takes 49 seconds
     *
     * /
    /* below is old observation and seems something is wrong with it
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
        beltpp::processor<task_t> proc(threads);

        beltpp::ctask ttob;
        ttob.i = unitWeight;
        task_t tt;
        //tt.bind_cmfptr<ctask, &ctask::mem>(ttob);
        tt = ttob;

        for (size_t i = 0; i < count; ++i)
        {
            //proc.run([unitWeight]{task(unitWeight);});
            proc.run(tt);

            if ((bUseWaits &&
                0 == i % threads))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
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
    size_t count = 1e7;
    long ms_wait, ms_destruct;

    for (size_t threads = 1; threads <= 1; ++threads)
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
