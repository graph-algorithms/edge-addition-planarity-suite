#ifndef APPCONST_H
#define APPCONST_H

/* Copyright (c) 1997-2003 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced in whole or in part without
        the written permission of the author. */

/* When defined, prints out run-time stats on a number of subordinate
    routines in the embedder */

//#define PROFILE
#ifdef PROFILE
#include "platformTime.h"
#endif

/* Define DEBUG to get additional debugging. The default is to define it when MSC does */

#ifdef _DEBUG
#define DEBUG
#endif

/* Some low-level functions are replaced by faster macros, except when debugging */

#define SPEED_MACROS
#ifdef DEBUG
#undef SPEED_MACROS
#endif

/* Return status values */

#define OK              1
#define NOTOK           0
#define NONEMBEDDABLE   -3

/* Array indices are used as pointers, and this means bad pointer */

#define NIL		-1
#define NIL_CHAR	0xFF

/* Defines fopen strings for reading and writing text files on PC and UNIX */

#ifdef WINDOWS
#define READTEXT        "rt"
#define WRITETEXT       "wt"
#else
#define READTEXT        "r"
#define WRITETEXT       "w"
#endif

#endif
