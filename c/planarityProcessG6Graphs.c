/*
Copyright (c) 1997-2022, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdio.h>

#include "planarity.h"

/****************************************************************************
 ProcessG6Graphs()
 TODO: write docs
 ****************************************************************************/

#define NUM_MINORS  9

int ProcessG6Graphs(char command, char *infileName, char *outfileName)
{
	graphP theGraph;
	int Result = OK;

	if ((infileName = ConstructInputFilename(infileName)) == NULL)
		return NOTOK;

	FILE* file = fopen(infileName, "r");
	char g6Repr[256];

	while (fgets(g6Repr, sizeof(g6Repr), file) != NULL) {
		Result = ProcessG6Graph(command, &theGraph, g6Repr);
	}

	fclose(file);

	// Free the graph
	gp_Free(&theGraph);

	// Flush any remaining message content to the user, and return the result
	FlushConsole(stdout);

	return Result;
}

int ProcessG6Graph(char command, graphP *pGraph, char g6Repr[])
{
	int Result = OK;

	sprintf(Line, ".g6 string is %s", g6Repr);
	Message(Line);
	FlushConsole(stdout);

	return Result==OK || Result==NONEMBEDDABLE ? OK : NOTOK;
}
