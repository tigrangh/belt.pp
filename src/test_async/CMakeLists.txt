# define the executable
add_executable(test_async
    main.cpp)

# libraries this module links to
target_link_libraries(test_async PRIVATE processor)

# what to do on make install
install(TARGETS test_async
        EXPORT belt.pp.package
        RUNTIME DESTINATION bin/belt.pp)
