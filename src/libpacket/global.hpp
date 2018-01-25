#pragma once

#include <belt.pp/global.hpp>

#if defined(PACKET_LIBRARY)
#define PACKETSHARED_EXPORT BELT_EXPORT
#else
#define PACKETSHARED_EXPORT BELT_IMPORT
#endif
