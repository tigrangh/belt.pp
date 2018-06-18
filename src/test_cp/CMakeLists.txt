# define the executable
add_executable(test_cp
    main.cpp)

# libraries this module links to
target_link_libraries(test_cp PRIVATE processor)

# what to do on make install
install(TARGETS test_cp
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})
