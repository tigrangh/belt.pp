add_executable(test_experiments
    main.cpp)

target_link_libraries(test_experiments socket processor message belt.pp
    #p2psocket
    messagecodes)
