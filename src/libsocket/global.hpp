#pragma once

#include <belt.pp/global.hpp>

#if defined(SOCKET_LIBRARY)
#define SOCKETSHARED_EXPORT BELT_EXPORT
#else
#define SOCKETSHARED_EXPORT BELT_IMPORT
#endif

