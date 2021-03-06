# interface library for headers only module
add_library(belt.pp INTERFACE)

# modules linking to this library will include following
# directories. two options are provided for installed package
# and for in-tree source build
target_include_directories(belt.pp INTERFACE
    $<INSTALL_INTERFACE:include/belt.pp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

# what to do on make install
install(TARGETS belt.pp
        EXPORT belt.pp.package
        DESTINATION lib/belt.pp)

set(SRC_FILES
    delegate.hpp
    global.hpp
    event.hpp
    ilog.hpp
    iprocessor.hpp
    isocket.hpp
    iterator_wrapper.hpp
    json.hpp
    log.hpp
    message_global.hpp
    messages.hpp
    meta.hpp
    packet.hpp
    parser.hpp
    processor.hpp
    queue.hpp
    scope_helper.hpp
    socket.hpp
    utility.hpp)

install(FILES
    ${SRC_FILES}
    DESTINATION include/belt.pp/belt.pp)

#
# add custom target simply for IDE to show the files
#
add_custom_target(belt.pp.include SOURCES
    ${SRC_FILES})
