add_definitions(-DMESSAGES_LIBRARY)

# define the shared library
add_library(messages SHARED
    global.hpp
    messages.cpp
    messages.hpp)

# libraries this module links to
target_link_libraries(messages PUBLIC belt.pp)

# what to do on make install
install(TARGETS messages
        EXPORT belt.pp.package
        RUNTIME DESTINATION bin/belt.pp
        LIBRARY DESTINATION lib/belt.pp
        ARCHIVE DESTINATION lib/belt.pp/archive)

install(FILES
    global.hpp
    messages.hpp
    DESTINATION include/belt.pp/libmessages)
