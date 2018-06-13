add_definitions(-DEVENT_LIBRARY)

# define the shared library
add_library(event SHARED
    global.hpp
    event.cpp
    event.hpp
    poll_master.hpp)

if (WIN32)
set(wslib "Ws2_32.lib")
endif()

# libraries this module links to
target_link_libraries(event PUBLIC belt.pp PRIVATE ${wslib})

# what to do on make install
install(TARGETS event
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    event.hpp
    DESTINATION include/belt.pp/libevent)
