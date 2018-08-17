# define the executable
add_executable(test_files_diff
    main.cpp)

# libraries this module links to
target_link_libraries(test_files_diff PRIVATE
    mesh.pp
    belt.pp
    )

# what to do on make install
install(TARGETS test_files_diff
        EXPORT belt.pp.package
        RUNTIME DESTINATION ${BELTPP_INSTALL_DESTINATION_RUNTIME}
        LIBRARY DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY}
        ARCHIVE DESTINATION ${BELTPP_INSTALL_DESTINATION_ARCHIVE})
