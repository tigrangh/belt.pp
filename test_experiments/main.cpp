#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <delegate.hpp>
#include <processor.hpp>
#include <type_traits>

class animal
{
public:
    int height;
    double age;
    bool gender;
};


int main(int argc, char** argv)
{
    animal pig;
    beltpp::delegate<const int&> height;
    height.bind_mptr<animal, &animal::height>(pig);

    pig.height = 35;
    //++height();


    std::cout << height() << std::endl;
    return 0;
}
