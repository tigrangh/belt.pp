add_definitions(-DPACKET_LIBRARY)

# define the shared library
add_library(packet SHARED
    global.hpp
    packet.cpp
    packet.hpp)

# libraries this module links to
target_link_libraries(packet PUBLIC belt.pp)

# what to do on make install
install(TARGETS packet
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    packet.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/libpacket)
