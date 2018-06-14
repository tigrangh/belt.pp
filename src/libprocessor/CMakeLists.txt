if(NOT WIN32)
set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")
endif()

add_definitions(-DPROCESSOR_LIBRARY)

# define the shared library
add_library(processor SHARED
    global.hpp
    processor.cpp
    processor.hpp)

# libraries this module links to
target_link_libraries(processor PUBLIC belt.pp)

# what to do on make install
install(TARGETS processor
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    processor.hpp
    DESTINATION include/belt.pp/libprocessor)
