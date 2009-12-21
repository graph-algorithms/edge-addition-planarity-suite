/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:

* John M. Boyer. "Simplified O(n) Algorithms for Planar Graph Embedding,
  Kuratowski Subgraph Isolation, and Related Problems". Ph.D. Dissertation,
  University of Victoria, 2001.

* John M. Boyer and Wendy J. Myrvold. "On the Cutting Edge: Simplified O(n)
  Planarity by Edge Addition". Journal of Graph Algorithms and Applications,
  Vol. 8, No. 3, pp. 241-273, 2004.

* John M. Boyer. "A New Method for Efficiently Generating Planar Graph
  Visibility Representations". In P. Eades and P. Healy, editors,
  Proceedings of the 13th International Conference on Graph Drawing 2005,
  Lecture Notes Comput. Sci., Volume 3843, pp. 508-511, Springer-Verlag, 2006.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

* Neither the name of the Planarity-Related Graph Algorithms Project nor the names
  of its contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "planarity.h"

int callNauty(int argc, char *argv[]);
int runTests(int argc, char *argv[]);
int callRandomGraphs(int argc, char *argv[]);
int callSpecificGraph(int argc, char *argv[]);
int callRandomMaxPlanarGraph(int argc, char *argv[]);
int callRandomNonplanarGraph(int argc, char *argv[]);

/****************************************************************************
 Command Line Processor
 ****************************************************************************/

int commandLine(int argc, char *argv[])
{
	int Result = OK;

	if (argc >= 3 && strcmp(argv[2], "-q") == 0)
		quietMode = 'y';

	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)
	{
		Result = helpMessage(argc >= 3 ? argv[2] : NULL);
	}

	else if (strcmp(argv[1], "-gen") == 0 || strcmp(argv[1], "-gens") == 0)
		Result = callNauty(argc, argv);

	else if (strcmp(argv[1], "-test") == 0)
		Result = runTests(argc, argv);

	else if (strcmp(argv[1], "-r") == 0)
		Result = callRandomGraphs(argc, argv);

	else if (strcmp(argv[1], "-s") == 0)
		Result = callSpecificGraph(argc, argv);

	else if (strcmp(argv[1], "-rm") == 0)
		Result = callRandomMaxPlanarGraph(argc, argv);

	else if (strcmp(argv[1], "-rn") == 0)
		Result = callRandomNonplanarGraph(argc, argv);

	else
	{
		ErrorMessage("Unsupported command line.  Here is the help for this program.\n");
		helpMessage(NULL);
		Result = NOTOK;
	}

	return Result == OK ? 0 : (Result == NONEMBEDDABLE ? 1 : -1);
}

/****************************************************************************
 Legacy Command Line Processor from version 1.x
 ****************************************************************************/

int legacyCommandLine(int argc, char *argv[])
{
graphP theGraph = gp_New();
int Result;

	Result = gp_Read(theGraph, argv[1]);
	if (Result != OK)
	{
		if (Result != NONEMBEDDABLE)
		{
			if (strlen(argv[1]) > MAXLINE - 100)
				sprintf(Line, "Failed to read graph\n");
			else
				sprintf(Line, "Failed to read graph %s\n", argv[1]);
			ErrorMessage(Line);
			return -2;
		}
	}

	Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);

	if (Result == OK)
	{
		gp_SortVertices(theGraph);
		gp_Write(theGraph, argv[2], WRITE_ADJLIST);
	}

	else if (Result == NONEMBEDDABLE)
	{
		if (argc >= 5 && strcmp(argv[3], "-n")==0)
		{
			gp_SortVertices(theGraph);
			gp_Write(theGraph, argv[4], WRITE_ADJLIST);
		}
	}
	else
		Result = NOTOK;

	gp_Free(&theGraph);

	// In the legacy 1.x versions, OK/NONEMBEDDABLE was 0 and NOTOK was -2
	return Result==OK || Result==NONEMBEDDABLE ? 0 : -2;
}

/****************************************************************************
 Call Nauty's MAKEG graph generator.
 ****************************************************************************/

// 'planarity -gen[s] C {ncl}': exhaustive tests with graphs generated by makeg,
// from McKay's nauty package
// makeg [-c -t -b] [-d<max>] n [mine [maxe [mod res]]]

extern unsigned long numGraphs;
extern unsigned long numErrors;
extern unsigned long numOKs;

int callNauty(int argc, char *argv[])
{
	char command;
	int numArgs, argsOffset, i, j, n;
	char *args[12];

	if (argc < 4 || argc > 12)
		return -1;

	// Determine the offset of the arguments after command C
	argsOffset = (argv[2][0] == '-' && argv[2][1] == 'q') ? 3 : 2;

	// Obtain the command C
	command = argv[argsOffset][1];

	// Same number of args, except exclude -gen[s], the optional -q, and the command C
	numArgs = argc-argsOffset;

	// Change 0th arg from planarity to makeg
	args[0] = "makeg";

	// Copy the rest of the args from argv, except -gen[s] and command C
	for (i=1; i < numArgs; i++)
		args[i] = argv[i+argsOffset];

	// Generate order N graphs of all sizes (number of edges)
	if (strcmp(argv[1], "-gen") == 0)
		return makeg_main(command, numArgs, args) == 0 ? 0 : -1;

	// Otherwise, generate statistics for each number of edges
	// and provide totals
	else
	{
		unsigned long totalGraphs=0, totalErrors=0, totalOKs=0;
		int nIndex;
		char edgeStr[10];
		unsigned long stats[16*15/2+1][3];
		platform_time start, end;
		int minEdges=-1, maxEdges=-1;

		// Find where the order of the graph is set
		nIndex = -1;
		for (i=1; i < numArgs; i++)
		{
			if (args[i][0] != '-')
			{
				nIndex = i;
				break;
			}
		}
		if (nIndex < 0)
			return -1;

		// Get the order of the graph and make sure it isn't bigger than
		// can be accommodated by the stats array
		n = atoi(args[nIndex]);
		if (n > 16)
			return -1;

		// If the caller did set the min and max edges, then we want to get those
		// values and respect them
		if (numArgs > nIndex + 1)
		{
			minEdges = atoi(args[nIndex+1]);
			if (numArgs > nIndex + 2)
				maxEdges = atoi(args[nIndex+2]);
			if (minEdges < 0 || minEdges > n*(n-1)/2)
				minEdges = 0;
			if (maxEdges < minEdges)
				maxEdges = minEdges;
			if (maxEdges > n*(n-1)/2)
				maxEdges = n*(n-1)/2;
		}
		else
		{
			minEdges = 0;
			maxEdges = n*(n-1)/2;
		}

		// Set the string used for the mine and maxe settings into the args
		// and ensure numArgs is set so that the string will be used by makeg
		args[nIndex+1] = args[nIndex+2] = edgeStr;
		if (numArgs < nIndex + 3)
			numArgs = nIndex + 3;

		// Do an edge-by-edge generation
	    platform_GetTime(start);

	    for (j = minEdges; j <= maxEdges; j++)
		{
			sprintf(edgeStr, "%d", j);

			if (makeg_main(command, numArgs, args) != 0)
			{
				printf("An error occurred.\n");
				return -1;
			}

			stats[j][0] = numGraphs;
			stats[j][1] = numErrors;
			stats[j][2] = numOKs;

			totalGraphs += numGraphs;
			totalErrors += numErrors;
			totalOKs += numOKs;
		}

		platform_GetTime(end);

		// Indicate if there were errors
		if (totalErrors > 0)
		{
			printf("Errors occurred\n");
			return -1;
		}

		// Otherwise, provide the statistics
		printf("\nNO ERRORS\n\n");

		printf("# Edges  # graphs    # OKs       # NoEmbeds\n");
		printf("-------  ----------  ----------  ----------\n");
		for (j = minEdges; j <= maxEdges; j++)
		{
			printf("%7d  %10lu  %10lu  %10lu\n", j, stats[j][0], stats[j][2], stats[j][0]-stats[j][2]);
		}
		printf("Totals   %10lu  %10lu  %10lu\n", totalGraphs, totalOKs, totalGraphs-totalOKs);

		printf("\nTotal time = %.3lf seconds\n", platform_GetDuration(start,end));

		return 0;
	}
}

/****************************************************************************
 Quick regression test
 ****************************************************************************/

int runTests(int argc, char *argv[])
{
#define NUMCOMMANDSTOTEST	6

	platform_time start, end;
	char *commandLine[] = {
			"planarity", "-gen", "C", "9"
	};
	char *commands[NUMCOMMANDSTOTEST] = {
			"-p", "-d", "-o", "-2", "-3", "-4"
	};
	char *commandNames[NUMCOMMANDSTOTEST] = {
			"planarity", "planar drawing", "outerplanarity",
			"K_{2,3} search", "K_{3,3} search", "K_4 search"
	};
	int success = TRUE;
	int results[] = { 194815, 194815, 269377, 268948, 191091, 265312 };
	int i, startCommand, stopCommand;

	startCommand = 0;
	stopCommand = NUMCOMMANDSTOTEST;

	// If a single test command is given...
	if (argc == 4 || (argc == 3 && quietMode != 'y'))
	{
		char *commandToTest = argv[2 + (quietMode == 'y' ? 1 : 0)];

		// Determine which test to run...
		for (i = 0; i < NUMCOMMANDSTOTEST; i++)
		{
			if (strcmp(commandToTest, commands[i]) == 0)
			{
				startCommand = i;
				stopCommand = i+1;
				break;
			}
		}
	}

	// Either run all tests or the selected test
    platform_GetTime(start);

    for (i=startCommand; i < stopCommand; i++)
	{
		printf("Testing %s\n", commandNames[i]);

		commandLine[2] = commands[i];
		if (callNauty(4, commandLine) != 0)
		{
			printf("An error occurred.\n");
			success = FALSE;
		}

		if (results[i] != numGraphs-numOKs)
		{
			printf("Incorrect result on command %s.\n", commands[i]);
			success = FALSE;
		}
	}

	platform_GetTime(end);
    printf("Finished processing in %.3lf seconds.\n", platform_GetDuration(start,end));

	if (success)
	    printf("Tests of all commands succeeded.\n");

	return success ? 0 : -1;
}

/****************************************************************************
 callRandomGraphs()
 ****************************************************************************/

// 'planarity -r [-q] C K N': Random graphs
int callRandomGraphs(int argc, char *argv[])
{
	char Choice = 0;
	int offset = 0, NumGraphs, SizeOfGraphs;

	if (argc < 5)
		return -1;

	if (argv[2][0] == '-' && (Choice = argv[2][1]) == 'q')
	{
		Choice = argv[3][1];
		if (argc < 6)
			return -1;
		offset = 1;
	}

	NumGraphs = atoi(argv[3+offset]);
	SizeOfGraphs = atoi(argv[4+offset]);

    return RandomGraphs(Choice, NumGraphs, SizeOfGraphs);
}

/****************************************************************************
 callSpecificGraph()
 ****************************************************************************/

// 'planarity -s [-q] C I O [O2]': Specific graph
int callSpecificGraph(int argc, char *argv[])
{
	char Choice=0, *infileName=NULL, *outfileName=NULL, *outfile2Name=NULL;
	int offset = 0;

	if (argc < 5)
		return -1;

	if (argv[2][0] == '-' && (Choice = argv[2][1]) == 'q')
	{
		Choice = argv[3][1];
		if (argc < 6)
			return -1;
		offset = 1;
	}

	infileName = argv[3+offset];
	outfileName = argv[4+offset];
	if (argc == 6+offset)
	    outfile2Name = argv[5+offset];

	return SpecificGraph(Choice, infileName, outfileName, outfile2Name);
}

/****************************************************************************
 callRandomMaxPlanarGraph()
 ****************************************************************************/

// 'planarity -rm [-q] N O [O2]': Maximal planar random graph
int callRandomMaxPlanarGraph(int argc, char *argv[])
{
	int offset = 0, numVertices;
	char *outfileName = NULL, *outfile2Name = NULL;

	if (argc < 4)
		return -1;

	if (argv[2][0] == '-' && argv[2][1] == 'q')
	{
		if (argc < 5)
			return -1;
		offset = 1;
	}

	numVertices = atoi(argv[2+offset]);
	outfileName = argv[3+offset];
	if (argc == 5+offset)
	    outfile2Name = argv[4+offset];

	return RandomGraph('p', 0, numVertices, outfileName, outfile2Name);
}

/****************************************************************************
 callRandomNonplanarGraph()
 ****************************************************************************/

// 'planarity -rn [-q] N O [O2]': Non-planar random graph (maximal planar plus edge)
int callRandomNonplanarGraph(int argc, char *argv[])
{
	int offset = 0, numVertices;
	char *outfileName = NULL, *outfile2Name = NULL;

	if (argc < 4)
		return -1;

	if (argv[2][0] == '-' && argv[2][1] == 'q')
	{
		if (argc < 5)
			return -1;
		offset = 1;
	}

	numVertices = atoi(argv[2+offset]);
	outfileName = argv[3+offset];
	if (argc == 5+offset)
	    outfile2Name = argv[4+offset];

	return RandomGraph('p', 1, numVertices, outfileName, outfile2Name);
}
