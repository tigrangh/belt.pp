#pragma once

#include <belt.pp/global.hpp>

#if defined(MESSAGE_LIBRARY)
#define MESSAGESHARED_EXPORT BELT_EXPORT
#else
#define MESSAGESHARED_EXPORT BELT_IMPORT
#endif
