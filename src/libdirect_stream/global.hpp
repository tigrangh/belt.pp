#pragma once

#include <belt.pp/global.hpp>

#if defined(DIRECT_STREAM_LIBRARY)
#define DIRECT_STREAM_SHARED_EXPORT BELT_EXPORT
#else
#define DIRECT_STREAM_SHARED_EXPORT BELT_IMPORT
#endif

