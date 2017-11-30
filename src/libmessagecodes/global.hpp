#pragma once

#include <belt.pp/global.hpp>

#if defined(MESSAGECODES_LIBRARY)
#define MESSAGECODESSHARED_EXPORT BELT_EXPORT
#else
#define MESSAGECODESSHARED_EXPORT BELT_IMPORT
#endif

