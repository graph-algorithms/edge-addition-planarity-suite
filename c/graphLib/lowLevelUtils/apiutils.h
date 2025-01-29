/*
Copyright (c) 1997-2024, John M. Boyer
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
// you should only need this many characters: an optional '-',
// followed by 10 digits (max signed int value is 2,147,483,647) and
// a null-terminator)
#define MAXCHARSFOR32BITINT 12

    extern int quietMode;

    extern int getQuietModeSetting(void);
    extern void setQuietModeSetting(int);

    extern void Message(char *message);
    extern void ErrorMessage(char *message);

#ifdef __cplusplus
}
#endif

#endif