add_definitions(-DMESSAGE_LIBRARY)

add_library(message SHARED
    global.hpp
    message.cpp
    message.hpp)

target_link_libraries(message PUBLIC belt.pp)

install(TARGETS message
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    message.hpp
    DESTINATION include/belt.pp/libmessage)


