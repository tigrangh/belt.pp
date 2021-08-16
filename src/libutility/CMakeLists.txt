add_definitions(-DUTILITY_LIBRARY)

# define the library
add_library(utility
    global.hpp
    utility.cpp
    utility.hpp)

# libraries this module links to
target_link_libraries(utility PUBLIC belt.pp)

# what to do on make install
install(TARGETS utility
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    utility.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/libutility)
