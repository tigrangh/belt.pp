# define the executable
add_executable(test_experiments
    main.cpp)

# libraries this module links to
target_link_libraries(test_experiments PRIVATE
    socket
    processor
    packet
    messages)

# what to do on make install
install(TARGETS test_experiments
        EXPORT belt.pp.package
        RUNTIME DESTINATION bin/belt.pp)
