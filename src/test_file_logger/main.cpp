#include <iostream>
#include <chrono>

#include <belt.pp/processor.hpp>
#include <belt.pp/log.hpp>

int main()
{

     beltpp::ilog_ptr pFileLogger = beltpp::file_logger("exe_publiqd_p2p", "Test.txt");

     pFileLogger->warning("Test1");
     pFileLogger->warning("Test2");

     pFileLogger->error("Test3");
     pFileLogger->error("Test5");

     //pFileLogger->warning("Warning1");
     //pFileLogger->error("Error1");

    return 0;
}
