# define the executable
add_executable(idl
    main.cpp)

# libraries this module links to
target_link_libraries(idl PUBLIC belt.pp)

# what to do on make install
install(TARGETS idl
        EXPORT belt.pp.package
        RUNTIME DESTINATION bin/belt.pp)
