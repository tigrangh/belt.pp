add_definitions(-DMESSAGE_LIBRARY)

# define the shared library
add_library(message SHARED
    global.hpp
    message.cpp
    message.hpp)

# libraries this module links to
target_link_libraries(message PUBLIC belt.pp)

# what to do on make install
install(TARGETS message
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    message.hpp
    DESTINATION include/belt.pp/libmessage)
