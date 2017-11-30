#ifndef BELT_SOCKET_GLOBAL_H
#define BELT_SOCKET_GLOBAL_H

#include <belt.pp/global.hpp>

#if defined(SOCKET_LIBRARY)
#define SOCKETSHARED_EXPORT BELT_EXPORT
#else
#define SOCKETSHARED_EXPORT BELT_IMPORT
#endif

#endif// BELT_SOCKET_GLOBAL_H
