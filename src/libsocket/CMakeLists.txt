add_definitions(-DSOCKET_LIBRARY)

# define the shared library
add_library(socket SHARED
    global.hpp
    socket.cpp
    socket.hpp)

if (WIN32)
    set(wslib "Ws2_32.lib")
endif()
# libraries this module links to
target_link_libraries(socket PUBLIC belt.pp packet event PRIVATE ${wslib})

# what to do on make install
install(TARGETS socket
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    socket.hpp
    DESTINATION include/belt.pp/libsocket)
