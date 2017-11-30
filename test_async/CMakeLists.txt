set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

add_executable(test_async
    main.cpp)

target_link_libraries(test_async processor include)
