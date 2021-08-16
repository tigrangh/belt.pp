# define the executable
add_executable(idl
    generator.cpp
    generator.hpp
    main.cpp
    resources.cpp
    resources.hpp)

# libraries this module links to
target_link_libraries(idl PUBLIC belt.pp)

# what to do on make install
install(TARGETS idl
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})
