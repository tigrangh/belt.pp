add_subdirectory(belt.pp)
add_subdirectory(libmessage)
add_subdirectory(libmessagecodes)
add_subdirectory(libprocessor)
add_subdirectory(libsocket)
#add_subdirectory(libp2psocket)
add_subdirectory(test_async)
add_subdirectory(test_cp)
add_subdirectory(test_cpdummy)
add_subdirectory(test_experiments)

install(FILES belt.pp-config.cmake DESTINATION lib/belt.pp)
install(EXPORT belt.pp.package DESTINATION lib/belt.pp)


