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

#define MAXLINE 1024

// The string representation for an integer must account for: an optional '-',
// then 10 digits (max signed 32-bit int), and a null-terminator
#define MAXCHARSFOR32BITINT 11

    // Used within the graphLib and also usable by graph applications to
    // emit error messages and informational messages.
    void ErrorMessage(char const *message);
    void Message(char const *message);

    // These methods control whether ErrorMessage() and Message() calls
    // emit output or skip producing output (the default)
    int gp_GetQuietModeFlag(void);
    void gp_SetQuietModeFlag(int newQuietModeFlag);

#ifdef __cplusplus
}
#endif

#endif
