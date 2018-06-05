add_definitions(-DEVENT_LIBRARY)

# define the shared library
add_library(event SHARED
    global.hpp
    event.cpp
    event.hpp
    poll_master.hpp)

# libraries this module links to
target_link_libraries(event PUBLIC belt.pp)

# what to do on make install
install(TARGETS event
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    event.hpp
    DESTINATION include/belt.pp/libevent)
