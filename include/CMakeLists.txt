add_library(include INTERFACE)
add_custom_target(include_ide SOURCES
    delegate.hpp
    global.hpp
    imessage.hpp
    iprocessor.hpp
    isocket.hpp
    message.hpp
    processor.hpp
    queue.hpp
    scope_helper.hpp
    socket.hpp)
