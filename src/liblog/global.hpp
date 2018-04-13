#pragma once

#include <belt.pp/global.hpp>

#if defined(LOG_LIBRARY)
#define LOGSHARED_EXPORT BELT_EXPORT
#else
#define LOGSHARED_EXPORT BELT_IMPORT
#endif

