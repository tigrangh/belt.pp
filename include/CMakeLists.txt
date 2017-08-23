add_library(include INTERFACE)
add_custom_target(include_ide SOURCES
    delegate.hpp
    global.hpp
    iprocessor.hpp
    processor.hpp)
