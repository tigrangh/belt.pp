add_definitions(-DLOG_LIBRARY)

# define the library
add_library(log
    global.hpp
    log_console.cpp
    log_file.cpp
    log.hpp)

# libraries this module links to
target_link_libraries(log PUBLIC belt.pp PRIVATE utility)

# what to do on make install
install(TARGETS log
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    log.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/liblog)
