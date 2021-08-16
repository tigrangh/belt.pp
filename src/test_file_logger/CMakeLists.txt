# define the executable
add_executable(test_file_logger
    main.cpp)

# libraries this module links to
target_link_libraries(test_file_logger PRIVATE
    processor
    utility
    log
    )

# what to do on make install
install(TARGETS test_file_logger
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})
