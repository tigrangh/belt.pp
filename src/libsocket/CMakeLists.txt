add_definitions(-DSOCKET_LIBRARY)

# define the shared library
add_library(socket SHARED
    global.hpp
    socket.cpp
    socket.hpp)

# libraries this module links to
target_link_libraries(socket PUBLIC belt.pp packet event)

# what to do on make install
install(TARGETS socket
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    socket.hpp
    DESTINATION include/belt.pp/libsocket)
