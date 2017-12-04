add_definitions(-DMESSAGECODES_LIBRARY)

# define the shared library
add_library(messagecodes SHARED
    global.hpp
    messagecodes.cpp
    messagecodes.hpp)

# libraries this module links to
target_link_libraries(messagecodes PUBLIC belt.pp)

# what to do on make install
install(TARGETS messagecodes
        EXPORT belt.pp.package
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    messagecodes.hpp
    DESTINATION include/belt.pp/libmessagecodes)
