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
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    messages.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/libmessages)
