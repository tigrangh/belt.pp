add_definitions(-DDIRECT_STREAM_LIBRARY)

# define the library
add_library(direct_stream
    global.hpp
    direct_stream.cpp
    direct_stream.hpp)

# libraries this module links to
target_link_libraries(direct_stream PUBLIC belt.pp packet)

# what to do on make install
install(TARGETS direct_stream
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})

install(FILES
    global.hpp
    direct_stream.hpp
    DESTINATION ${BELTPP_INSTALL_DESTINATION_INCLUDE}/libdirect_stream)
