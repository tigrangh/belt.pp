add_definitions(-DUTILITY_LIBRARY)

# define the shared library
add_library(utility SHARED
    global.hpp
    utility.cpp
    utility.hpp)

# libraries this module links to
target_link_libraries(utility PUBLIC belt.pp)

# what to do on make install
install(TARGETS utility
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    utility.hpp
    DESTINATION include/belt.pp/libutility)
