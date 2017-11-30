add_library(include INTERFACE)
add_custom_target(include_ide SOURCES
    delegate.hpp
    global.hpp
    iprocessor.hpp
    isocket.hpp
    iterator_wrapper.hpp
    message.hpp
    message_global.hpp
    messagecodes.hpp
    p2psocket.hpp
    processor.hpp
    queue.hpp
    scope_helper.hpp
    socket.hpp)

target_include_directories(include INTERFACE .)
