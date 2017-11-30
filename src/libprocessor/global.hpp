#pragma once

#include <belt.pp/global.hpp>

#if defined(PROCESSOR_LIBRARY)
#define PROCESSORSHARED_EXPORT BELT_EXPORT
#else
#define PROCESSORSHARED_EXPORT BELT_IMPORT
#endif

