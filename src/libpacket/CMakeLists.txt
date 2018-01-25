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
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    packet.hpp
    DESTINATION include/belt.pp/libpacket)
