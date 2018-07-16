# define the executable
add_executable(idl_TypeScript
    generator.cpp
    generator.hpp
    main.cpp)

# libraries this module links to
target_link_libraries(idl_TypeScript PUBLIC belt.pp)

# what to do on make install
install(TARGETS idl_TypeScript
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})
