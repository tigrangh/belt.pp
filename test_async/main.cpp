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
                //std::this_thread::sleep_for(std::chrono::milliseconds(0));
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
/*  testing with 40 attempts    */
/*                                  avg     median  stdev   min     max
 * 10 mln tasks and 10 mln waits    36.47   36.56   0.74    35.09   37.81
 * 100 mln tasks                    24.66   24.66   2.42    19.62   29.67
 * 100 mln tasks and 10 mln waits   39.42   39.26   0.6     38.41   40.98
 * 100 mln tasks and 20 mln waits   75.74   75.58   1.23    73.61   78.34
 *
 * qt test 10 mln signal-slot <=> signal-slot takes 49 seconds
 *
 */
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
