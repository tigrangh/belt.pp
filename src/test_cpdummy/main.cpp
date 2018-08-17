#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include <belt.pp/processor.hpp>

using task_t = std::function<void()>;
struct bobo
{
    size_t index;
    size_t count;
    beltpp::iprocessor<task_t>* processor_producer;
    beltpp::iprocessor<task_t>* processor_consumer;
    beltpp::iprocessor<task_t>* processor_finisher;
};
void producer(bobo object);
void consumer(bobo object);

void producer(bobo object)
{
    if (object.index < object.count)
    {
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        ++object.index;
        //std::cout << index << "\t";

        object.processor_consumer->run([object]
        {
            consumer(object);
        });
    }
    else
        object.processor_finisher->run([]{});
}
void consumer(bobo object)
{
    object.processor_producer->run([object]
    {
        producer(object);
    });

    /*std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << index << "\n";*/
}
/*  testing with 40 attempts    */
/*                          avg     median  stdev   min     max
 * 10 mln tasks bouncing    40.66   40.62   0.455   39.65   41.65
 *
 */
int main(int argc, char** argv)
{
    B_UNUSED(argc);
    B_UNUSED(argv);

    namespace chrono = std::chrono;
    using steady_clock = chrono::steady_clock;
    using time_point = steady_clock::time_point;
    time_point tp_start = steady_clock::now();
    time_point tp_wait;

    try
    {
        using iprocessorptr = std::unique_ptr<beltpp::iprocessor<task_t>>;

        iprocessorptr ptr_producer(new beltpp::processor<task_t>(1));
        iprocessorptr ptr_consumer(new beltpp::processor<task_t>(1));
        iprocessorptr ptr_finisher(new beltpp::processor<task_t>(1));

        /*beltpp::iprocessor<task_t>& processor_producer = *ptr_producer;
        beltpp::iprocessor<task_t>& processor_consumer = *ptr_consumer;
        beltpp::iprocessor<task_t>& processor_finisher = *ptr_finisher;*/

        bobo object;
        object.count = size_t(1e7);
        object.index = 0;
        object.processor_consumer = ptr_consumer.get();
        object.processor_producer = ptr_producer.get();
        object.processor_finisher = ptr_finisher.get();

        object.processor_producer->run([object]
        {
            producer(object);
        });

        object.processor_finisher->wait(1);
        object.processor_producer->wait();
        object.processor_consumer->wait();
        object.processor_finisher->wait();
    }
    catch(std::exception const& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "apparently something has happened" << std::endl;
    }

    tp_wait = steady_clock::now();
    steady_clock::duration elapsed = tp_wait - tp_start;
    chrono::milliseconds ms_elapsed = chrono::duration_cast<chrono::milliseconds>(elapsed);
    long mswait = long(ms_elapsed.count());

    std::cout << mswait << std::endl;

    return 0;
}
