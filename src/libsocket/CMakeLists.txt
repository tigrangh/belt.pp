add_definitions(-DSOCKET_LIBRARY)

# define the library
add_library(socket
    global.hpp
    event.cpp
    event.hpp
    event_impl.hpp
    native.cpp
    native.hpp
    poll_master.hpp
    socket.cpp
    socket.hpp)

if (WIN32)
    set(wslib Ws2_32.lib Mswsock.lib)
endif()
# libraries this module links to
target_link_libraries(socket PUBLIC belt.pp packet PRIVATE ${wslib})

# what to do on make install
install(TARGETS socket
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    event.hpp
    socket.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/libsocket)
