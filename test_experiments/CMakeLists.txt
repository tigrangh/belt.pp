set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )
#set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

add_executable(test_experiments
    main.cpp)

target_link_libraries(test_experiments socket processor message include
    #p2psocket
    messagecodes)
