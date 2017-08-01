set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )
#set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

add_executable(test_cpdummy
    main.cpp)

target_link_libraries(test_cpdummy processor include)
