
add_definitions(-DMESSAGECODES_LIBRARY)

add_library(messagecodes SHARED
    global.hpp
    messagecodes.cpp
    messagecodes.hpp)

target_link_libraries(messagecodes include)
