add_definitions(-DSOCKET_LIBRARY)
#add_definitions(-DUSE_BOOST_SOCKET)

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

# some code duplication goes here related to boost
#if (NOT BUILD_SHARED_LIBS)
#set (Boost_USE_STATIC_LIBS ON)
#set (CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
#endif()
#set (Boost_MULTITHREADED ON)
#set (Boost_USE_STATIC_RUNTIME OFF)
#
#find_package(Boost 1.58 REQUIRED
#    COMPONENTS system
#               filesystem
#               program_options
#               locale)
#target_link_libraries(socket PRIVATE Boost::system)
#
#find_package(OpenSSL REQUIRED SSL Crypto)
#target_link_libraries(socket PRIVATE OpenSSL::SSL OpenSSL::Crypto)

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
