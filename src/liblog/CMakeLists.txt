add_definitions(-DLOG_LIBRARY)

# define the shared library
add_library(log SHARED
    global.hpp
    log_console.cpp
    log.hpp)

# libraries this module links to
target_link_libraries(log PUBLIC belt.pp)

# what to do on make install
install(TARGETS log
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    log.hpp
    DESTINATION include/belt.pp/liblog)
