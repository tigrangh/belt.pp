#ifndef BELT_PROCESSOR_GLOBAL_H
#define BELT_PROCESSOR_GLOBAL_H

#include <belt.pp/global.hpp>

#if defined(PROCESSOR_LIBRARY)
#define PROCESSORSHARED_EXPORT BELT_EXPORT
#else
#define PROCESSORSHARED_EXPORT BELT_IMPORT
#endif

#endif// BELT_PROCESSOR_GLOBAL_H
