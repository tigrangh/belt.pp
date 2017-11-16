#ifndef BELT_GLOBAL_H
#define BELT_GLOBAL_H

#ifdef _MSC_VER

#define B_OS_WIN

#define BELT_EXPORT     __declspec(dllexport)
#define BELT_IMPORT     __declspec(dllimport)
#else

#define B_OS_UNIX

#define BELT_EXPORT
#define BELT_IMPORT
#endif

#endif// BELT_GLOBAL
