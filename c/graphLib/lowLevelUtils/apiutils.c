/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdarg.h>

#include "apiutils.h"
#include "appconst.h"

int quietMode = FALSE;

int getQuietModeSetting(void)
{
	return quietMode;
}

void setQuietModeSetting(int newQuietModeSetting)
{
	quietMode = newQuietModeSetting;
}

void Message(char *message)
{
	if (!getQuietModeSetting())
	{
		fprintf(stdout, "%s", message);
		fflush(stdout);
	}
}

void ErrorMessage(char *message)
{
	if (!getQuietModeSetting())
	{
		fprintf(stderr, "%s", message);
		fflush(stderr);
	}
}