#pragma once

#include <belt.pp/global.hpp>

#if defined(UTILITY_LIBRARY)
#define UTILITYSHARED_EXPORT BELT_EXPORT
#else
#define UTILITYSHARED_EXPORT BELT_IMPORT
#endif

