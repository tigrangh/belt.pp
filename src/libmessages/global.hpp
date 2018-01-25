#pragma once

#include <belt.pp/global.hpp>

#if defined(MESSAGES_LIBRARY)
#define MESSAGESSHARED_EXPORT BELT_EXPORT
#else
#define MESSAGESSHARED_EXPORT BELT_IMPORT
#endif

