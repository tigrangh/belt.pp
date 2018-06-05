#pragma once

#include <belt.pp/global.hpp>

#if defined(EVENT_LIBRARY)
#define EVENTSHARED_EXPORT BELT_EXPORT
#else
#define EVENTSHARED_EXPORT BELT_IMPORT
#endif

