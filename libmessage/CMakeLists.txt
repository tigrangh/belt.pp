add_definitions(-DMESSAGE_LIBRARY)

add_library(message SHARED
    global.hpp
    message.cpp
    message.hpp)

target_link_libraries(message belt.pp)
