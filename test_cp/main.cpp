#include <iostream>
#include <memory>
#include <fstream>
#include <vector>
#include <processor.hpp>

using task_t = std::function<void()>;
void reader(size_t iChunkSize,
            std::ifstream& fl_read,
            std::ofstream& fl_write,
            beltpp::iprocessor<task_t>& processor_copier);
void writer(size_t iChunkSize,
            std::vector<char> const& buffer,
            std::ifstream& fl_read,
            std::ofstream& fl_write,
            beltpp::iprocessor<task_t>& processor_copier);

void reader(size_t iChunkSize,
            std::ifstream& fl_read,
            std::ofstream& fl_write,
            beltpp::iprocessor<task_t>& processor_copier)
{
    std::shared_ptr<std::vector<char>> ptr_buffer(new std::vector<char>(iChunkSize));
    fl_read.read(&ptr_buffer->operator[](0), iChunkSize);
    if (fl_read)
    {
        size_t iReadSize = fl_read.gcount();

        if (iReadSize < iChunkSize)
        {
            ptr_buffer->resize(iReadSize);
        }

        processor_copier.run([iChunkSize,
                             ptr_buffer,
                             &fl_read,
                             &fl_write,
                             &processor_copier]
        {
            writer(iChunkSize, *ptr_buffer, fl_read, fl_write, processor_copier);
        });
    }
}
void writer(size_t iChunkSize,
            std::vector<char> const& buffer,
            std::ifstream& fl_read,
            std::ofstream& fl_write,
            beltpp::iprocessor<task_t>& processor_copier)
{
    if (buffer.size() >= iChunkSize)
        processor_copier.run([iChunkSize, &fl_read, &fl_write, &processor_copier]
        {
            reader(iChunkSize, fl_read, fl_write, processor_copier);
        });

    fl_write.write(&buffer[0], buffer.size());
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
        {
            std::cout << "usage - test_cp input destination" << std::endl;
            return 1;
        }

        beltpp::iprocessor<task_t>* ptr_copier(new beltpp::processor<task_t>(1));

        beltpp::iprocessor<task_t>& processor_copier = *ptr_copier;

        std::ifstream fl_read(argv[1], std::ios_base::binary);
        std::ofstream fl_write(argv[2], std::ios_base::binary);

        if (false == fl_read.is_open())
        {
            std::cout << "check the input file " << argv[1] << std::endl;
            return 2;
        }
        if (false == fl_write.is_open())
        {
            std::cout << "check the output file " << argv[2] << std::endl;
            return 3;
        }

        size_t iChunkSize = 10000000;

        processor_copier.run([iChunkSize,
                             &fl_read,
                             &fl_write,
                             &processor_copier]
        {
            reader(iChunkSize, fl_read, fl_write, processor_copier);
        });

        processor_copier.wait();

        fl_read.close();
        fl_write.close();
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
