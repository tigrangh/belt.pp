add_library(belt.pp INTERFACE)
add_custom_target(include SOURCES
    delegate.hpp
    global.hpp
    iprocessor.hpp
    isocket.hpp
    iterator_wrapper.hpp
    message.hpp
    message_global.hpp
    messagecodes.hpp
    #p2psocket.hpp
    processor.hpp
    queue.hpp
    scope_helper.hpp
    socket.hpp)

target_include_directories(belt.pp INTERFACE ..)
