/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#include "appconst.h"

#include "apiutils.h"
#include "apiutils.private.h"

// The graphLib ErrorMessage() and Message() calls are suppressed by
// default, but an application can turn them on if desired.
int quietModeFlag = TRUE;

int getQuietModeFlag(void)
{
    return quietModeFlag;
}

void setQuietModeFlag(int newQuietModeFlag)
{
    quietModeFlag = newQuietModeFlag;
}

void Message(char const *message)
{
    if (!getQuietModeFlag())
    {
        fprintf(stdout, "%s", message);
        fflush(stdout);
    }
}

void ErrorMessage(char const *message)
{
    if (!getQuietModeFlag())
    {
        fprintf(stderr, "%s", message);
        fflush(stderr);
    }
}

// LOGGING is not defined in the standard compile configuration.
// A graphLib developer can uncomment LOGGING in apiutils.private.h
#ifdef LOGGING

/********************************************************************
 _Log()

 When the project is compiled with LOGGING enabled, this method writes
 a string to the file PLANARITY.LOG in the current working directory.
 On first write, the file is created or cleared.
 Call this method with NULL to close the log file.
 ********************************************************************/

void closeLogFileAtExit(void);

void _Log(char const *Str)
{
    static FILE *logfile = NULL;
    static int triedlogfile = FALSE;

    if (logfile == NULL && !triedlogfile)
    {
        triedlogfile = TRUE;
        if (atexit(closeLogFileAtExit) != 0)
            ErrorMessage("Unable to set up atexit() to close Edge_Addition_Planarity_Suite log file on exit");
        else
        {
            if ((logfile = fopen("Edge_Addition_Planarity_Suite.LOG", WRITETEXT)) == NULL)
                ErrorMessage("Unable to open the Edge_Addition_Planarity_Suite log file");
        }
    }

    if (logfile != NULL)
    {
        if (Str != NULL)
        {
            fprintf(logfile, "%s", Str);
            fflush(logfile);
        }
        else
        {
            fclose(logfile);
            logfile = NULL;
        }
    }
}

void _LogLine(char const *Str)
{
    _Log(Str);
    _Log("\n");
}

void closeLogFileAtExit(void)
{
    _gp_Log(NULL);
}

static char LogStr[MAXLINE + 1];

char *_MakeLogStr1(const char *format, int one)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(LogStr, format, one);
#pragma GCC diagnostic pop
    return LogStr;
}

char *_MakeLogStr2(const char *format, int one, int two)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(LogStr, format, one, two);
#pragma GCC diagnostic pop
    return LogStr;
}

char *_MakeLogStr3(const char *format, int one, int two, int three)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(LogStr, format, one, two, three);
#pragma GCC diagnostic pop
    return LogStr;
}

char *_MakeLogStr4(const char *format, int one, int two, int three, int four)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(LogStr, format, one, two, three, four);
#pragma GCC diagnostic pop
    return LogStr;
}

char *_MakeLogStr5(const char *format, int one, int two, int three, int four, int five)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(LogStr, format, one, two, three, four, five);
#pragma GCC diagnostic pop
    return LogStr;
}

#endif // LOGGING
