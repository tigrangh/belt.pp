# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/belt.pp")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


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


