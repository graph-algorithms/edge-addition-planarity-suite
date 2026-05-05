/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/
#ifndef APIUTILS_H
#define APIUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdio.h"

// N.B. Every time this is used to create a string for a message or
// error message, the developer must check that there will not be a
// memory overwrite error.
#define MAXLINE 1024

// N.B. Every time you're trying to read a 32-bit int from a string,
// you should only need to read this many characters: an optional '-',
// followed by 10 digits (max signed 32-bit int value is 2,147,483,647).
// One must always allocate an additional byte for the null-terminator!
#define MAXCHARSFOR32BITINT 11

#if defined(_MSC_VER) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define APPLY_FORMAT_ATTRIBUTE 0
#elif defined(__has_attribute)
#define APPLY_FORMAT_ATTRIBUTE __has_attribute(format)
#elif defined(__GNUC__) || defined(__clang__)
#define APPLY_FORMAT_ATTRIBUTE 1
#else
#define APPLY_FORMAT_ATTRIBUTE 0
#endif

#if APPLY_FORMAT_ATTRIBUTE
#if defined(__GNUC__)
#define FORMAT_PRINTF(formatIndex, firstArg) __attribute__((format(gnu_printf, formatIndex, firstArg)))
#else
#define FORMAT_PRINTF(formatIndex, firstArg) __attribute__((format(printf, formatIndex, firstArg)))
#endif
#else
#define FORMAT_PRINTF(formatIndex, firstArg)
#endif

    extern int quietMode;

    extern int getQuietModeSetting(void);
    extern void setQuietModeSetting(int);

    extern void Message(char const *message, ...) FORMAT_PRINTF(1, 2);
    extern void ErrorMessage(char const *message, ...) FORMAT_PRINTF(1, 2);

    int GetNumCharsToReprInt(int theNum, int *numCharsRequired);
#ifdef __cplusplus
}
#endif

#endif
