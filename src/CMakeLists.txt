if( WIN32 )
    SET(BELTPP_INSTALL_DESTINATION_RUNTIME bin)
    SET(BELTPP_INSTALL_DESTINATION_LIBRARY lib)
    SET(BELTPP_INSTALL_DESTINATION_ARCHIVE lib)
    SET(BELTPP_INSTALL_DESTINATION_INCLUDE include)
else( WIN32 )
    SET(BELTPP_INSTALL_DESTINATION_RUNTIME bin/belt.pp)
    SET(BELTPP_INSTALL_DESTINATION_LIBRARY lib/belt.pp)
    SET(BELTPP_INSTALL_DESTINATION_ARCHIVE lib/belt.pp)
    SET(BELTPP_INSTALL_DESTINATION_INCLUDE include/belt.pp)
endif( WIN32 )

if( NOT WIN32 )
    # don't skip the full RPATH for the build tree
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/belt.pp")

    # not sure if there is any use from below command
    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    #SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

add_subdirectory(belt.pp)
add_subdirectory(idl)
add_subdirectory(liblog)
add_subdirectory(libmessages)
add_subdirectory(libpacket)
add_subdirectory(libprocessor)
add_subdirectory(libsocket)
add_subdirectory(libutility)
add_subdirectory(test_async)
add_subdirectory(test_cp)
add_subdirectory(test_cpdummy)
add_subdirectory(test_experiments)

# following is used for find_package functionality
install(FILES belt.pp-config.cmake DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY})
install(EXPORT belt.pp.package DESTINATION ${BELTPP_INSTALL_DESTINATION_LIBRARY})


