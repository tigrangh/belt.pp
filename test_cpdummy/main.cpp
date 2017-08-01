#include <iostream>
#include <thread>
#include <chrono>
#include <processor.hpp>

using task_t = std::function<void()>;
void reader(size_t index,
            size_t count,
            beltpp::iprocessor<task_t>& processor_copier);
void writer(size_t index,
            size_t count,
            beltpp::iprocessor<task_t>& processor_copier);

void reader(size_t index,
            size_t count,
            beltpp::iprocessor<task_t>& processor_copier)
{
    if (index < count)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ++index;

        processor_copier.run([index,
                             count,
                             &processor_copier]
        {
            writer(index, count, processor_copier);
        });
    }
}
void writer(size_t index,
            size_t count,
            beltpp::iprocessor<task_t>& processor_copier)
{
    processor_copier.run([index, count, &processor_copier]
    {
        reader(index, count, processor_copier);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(int argc, char** argv)
{
    try
    {
        beltpp::iprocessor<task_t>* ptr_copier(new beltpp::processor<task_t>(2));

        beltpp::iprocessor<task_t>& processor_copier = *ptr_copier;

        size_t count = 10, index = 0;

        processor_copier.run([index,
                             count,
                             &processor_copier]
        {
            reader(index, count, processor_copier);
        });

        processor_copier.wait();
    }
    catch(std::exception const& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    catch(...)
    {
        std::cout << "apparently something has happened" << std::endl;
    }

    return 0;
}
