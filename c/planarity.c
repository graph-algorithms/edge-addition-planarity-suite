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

/****************************************************************************
 Configuration
 ****************************************************************************/

char Mode='r',
     OrigOut='n',
     EmbeddableOut='n',
     ObstructedOut='n',
     AdjListsForEmbeddingsOut='n',
     quietMode='n';

/****************************************************************************
 MESSAGE - prints a string, but when debugging adds \n and flushes stdout
 ****************************************************************************/

char Line[MAXLINE];

void Message(char *message)
{
	if (quietMode == 'n')
	{
	    fprintf(stdout, "%s", message);

#ifdef DEBUG
	    fprintf(stdout, "\n");
	    fflush(stdout);
#endif
	}
}

void ErrorMessage(char *message)
{
	if (quietMode == 'n')
	{
		fprintf(stderr, "%s", message);

#ifdef DEBUG
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
	}
}

void ProjectTitle()
{
    Message("\n=================================================="
            "\nPlanarity version 2.2"
            "\nCopyright (c) 2009 by John M. Boyer"
    		"\nContact info: jboyer at acm.org"
            "\n=================================================="
            "\n");
}

/****************************************************************************
 MAIN
 ****************************************************************************/

int main(int argc, char *argv[])
{
	int retVal=0;

	if (argc <= 1)
		retVal = menu();

	else if (argv[1][0] == '-')
		retVal = commandLine(argc, argv);

	else
		retVal = legacyCommandLine(argc, argv);

	// Close the log file if logging
	gp_Log(NULL);

	return retVal;
}

int helpMessage(char *param)
{
	char *commandStr =
    	"C = command from menu\n"
    	"    -p = Planar embedding and Kuratowski subgraph isolation\n"
        "    -o = Outerplanar embedding and obstruction isolation\n"
        "    -d = Planar graph drawing\n"
        "    -2 = Search for subgraph homeomorphic to K_{2,3}\n"
        "    -3 = Search for subgraph homeomorphic to K_{3,3}\n"
        "    -4 = Search for subgraph homeomorphic to K_4\n"
		"    -c = Color the vertices of the graph\n"
    	"\n";

	ProjectTitle();

	if (param == NULL)
	{
	    Message(
            "'planarity': menu-driven\n"
            "'planarity (-h|-help)': this message\n"
            "'planarity (-h|-help) -gen': more help with nauty generator command line\n"
            "'planarity (-h|-help) -menu': more help with menu-based command line\n"
    	    "'planarity -test [-q] [C]': runs tests (optional quiet mode, single test)\n"
	    	"\n"
	    );

	    Message(
	    	"Common usages\n"
	    	"-------------\n"
            "planarity -s -q -p infile.txt embedding.out [obstruction.out]\n"
	    	"Process infile.txt in quiet mode (-q), putting planar embedding in \n"
	    	"embedding.out or (optionally) a Kuratowski subgraph in Obstruction.out\n"
	    	"Process returns 0=planar, 1=nonplanar, -1=error\n"
	    	"\n"
            "planarity -s -q -d infile.txt embedding.out [drawing.out]\n"
            "If graph in infile.txt is planar, then put embedding in embedding.out \n"
            "and (optionally) an ASCII art drawing in drawing.out\n"
            "Process returns 0=planar, 1=nonplanar, -1=error\n"
	    	"\n"
	    );
	}

	else if (strcmp(param, "-gen") == 0)
	{
	    Message(
	    	"'planarity -gen[s] [-q] C {ncl}': test run command C on graphs generated by\n"
	    	"                                  makeg, part of McKay's nauty program\n"
            "                                  -gens provides statistics per number of edges\n"
	    );

	    Message(commandStr);

	    Message("{ncl}= [-c -t -b] [-d<max>] n [mine [maxe [mod res]]]\n\n");

	    Message(
			"n    = the number of vertices (1..16)\n"
			"mine = the minimum number of edges (no bounds if missing)\n"
			"maxe = the maximum number of edges (same as mine if missing)\n"
			"mod, res = a way to restrict the output to a subset.\n"
			"           All the graphs in G(n,mine..maxe) are divided into\n"
			"           disjoint classes C(mod,0),C(mod,1),...,C(mod,mod-1),\n"
			"           of very approximately equal size.\n"
			"           Only the class C(mod,res) is generated.\n"
			"           The usual relationships between modulo classes are\n"
			"           obeyed; for example C(4,3) = C(8,3) union C(8,7).\n"
			"-c    : only generate connected graphs\n"
			"-t    : only generate triangle-free graphs\n"
			"-b    : only generate bipartite graphs\n"
			"-d<x> : specify an upper bound for the maximum degree.\n"
			"        The value must be adjacent to the 'd', e.g. -d6.\n"
	    );
	}

	else if (strcmp(param, "-menu") == 0)
	{
	    Message(
	    	"'planarity -r [-q] C K N': Random graphs\n"
	    	"'planarity -s [-q] C I O [O2]': Specific graph\n"
	        "'planarity -rm [-q] N O [O2]': Maximal planar random graph\n"
	        "'planarity -rn [-q] N O [O2]': Nonplanar random graph (maximal planar + edge)\n"
	        "'planarity I O [-n O2]': Legacy command-line (default -s -p)\n"
	    	"\n"
	    );

	    Message("-q is for quiet mode (no messages to stdout and stderr)\n\n");

	    Message(commandStr);

	    Message(
	    	"K = # of graphs to randomly generate\n"
	    	"N = # of vertices in each randomly generated graph\n"
	        "I = Input file (for work on a specific graph)\n"
	        "O = Primary output file\n"
	        "    For example, if C=-p then O receives the planar embedding\n"
	    	"    If C=-3, then O receives a subgraph containing a K_{3,3}\n"
	        "O2= Secondary output file\n"
	    	"    For -s, if C=-p or -o, then O2 receives the embedding obstruction\n"
	       	"    For -s, if C=-d, then O2 receives a drawing of the planar graph\n"
	    	"    For -m and -n, O2 contains the original randomly generated graph\n"
	    	"\n"
	    );

	    Message(
	        "planarity process results: 0=OK, -1=NOTOK, 1=NONEMBEDDABLE\n"
	    	"    1 result only produced by specific graph mode (-s)\n"
	        "      with command -2,-3,-4: found K_{2,3}, K_{3,3} or K_4\n"
	    	"      with command -p,-d: found planarity obstruction\n"
	    	"      with command -o: found outerplanarity obstruction\n"
	    );
	}

    return 0;
}

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

// Quick regression test
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
 MENU-DRIVEN PROGRAM
 ****************************************************************************/

int menu()
{
#ifdef PROFILING  // If profiling, then only run RandomGraphs()

     RandomGraphs('p', 0, 0);

#else

char Choice;

     do {
    	ProjectTitle();

        Message("\n"
                "P. Planar embedding and Kuratowski subgraph isolation\n"
                "D. Planar graph drawing\n"
                "O. Outerplanar embedding and obstruction isolation\n"
                "2. Search for subgraph homeomorphic to K_{2,3}\n"
                "3. Search for subgraph homeomorphic to K_{3,3}\n"
                "4. Search for subgraph homeomorphic to K_4\n"
        		"C. Color the vertices of the graph\n"
        		"H. Help message for command line version\n"
                "R. Reconfigure options\n"
                "X. Exit\n"
                "\n"
                "Enter Choice: "
        );

        fflush(stdin);
        scanf(" %c", &Choice);
        Choice = tolower(Choice);

        if (Choice == 'h')
        	helpMessage(NULL);

        else if (Choice == 'r')
        	Reconfigure();

        else if (Choice != 'x')
        {
        	char *secondOutfile = NULL;
        	if (Choice == 'p'  || Choice == 'o' || Choice == 'd')
        		secondOutfile ="";

            switch (tolower(Mode))
            {
                case 's' : SpecificGraph(Choice, NULL, NULL, secondOutfile); break;
                case 'r' : RandomGraphs(Choice, 0, 0); break;
                case 'm' : RandomGraph(Choice, 0, 0, NULL, NULL); break;
                case 'n' : RandomGraph(Choice, 1, 0, NULL, NULL); break;
            }
        }

        if (Choice != 'r' && Choice != 'x')
        {
            Message("\nPress a key then hit ENTER to continue...");
            fflush(stdin);
            scanf(" %*c");
            fflush(stdin);
            Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
        }

     }  while (Choice != 'x');
#endif

     return 0;
}

/****************************************************************************
 ****************************************************************************/

void Reconfigure()
{
     fflush(stdin);

     Message("\nDo you want to \n"
    		 "  Randomly generate graphs (r),\n"
    		 "  Specify a graph (s),\n"
    		 "  Randomly generate a maximal planar graph (m), or\n"
    		 "  Randomly generate a non-planar graph (n)?");
     scanf(" %c", &Mode);

     Mode = tolower(Mode);
     if (!strchr("rsmn", Mode))
    	 Mode = 's';

     if (Mode == 'r')
     {
        Message("\nNOTE: The directories for the graphs you want must exist.\n\n");

        Message("Do you want original graphs in directory 'random' (last 10 max)?");
        scanf(" %c", &OrigOut);

        Message("Do you want adj. matrix of embeddable graphs in directory 'embedded' (last 10 max))?");
        scanf(" %c", &EmbeddableOut);

        Message("Do you want adj. matrix of obstructed graphs in directory 'obstructed' (last 10 max)?");
        scanf(" %c", &ObstructedOut);

        Message("Do you want adjacency list format of embeddings in directory 'adjlist' (last 10 max)?");
        scanf(" %c", &AdjListsForEmbeddingsOut);
     }

     Message("\n");
}

/****************************************************************************
 ****************************************************************************/

void SaveAsciiGraph(graphP theGraph, char *graphName)
{
char Ch;

    Message("Do you want to save the graph in Ascii format (to test.dat)?");
    scanf(" %c", &Ch);
    fflush(stdin);

    if (Ch == 'y')
    {
        int  e, limit;
        FILE *outfile = fopen("test.dat", "wt");
        fprintf(outfile, "%s\n", graphName);

        limit = theGraph->edgeOffset + 2*(theGraph->M + sp_GetCurrentSize(theGraph->edgeHoles));

        for (e = theGraph->edgeOffset; e < limit; e+=2)
        {
            if (theGraph->G[e].v != NIL)
                fprintf(outfile, "%d %d\n", theGraph->G[e].v+1, theGraph->G[e+1].v+1);
        }

        fprintf(outfile, "0 0\n");

        fclose(outfile);
    }
}

/****************************************************************************
 ****************************************************************************/

int GetEmbedFlags(char command)
{
	int embedFlags = 0;

	switch (command)
	{
		case 'o' : embedFlags = EMBEDFLAGS_OUTERPLANAR; break;
		case 'p' : embedFlags = EMBEDFLAGS_PLANAR; break;
		case 'd' : embedFlags = EMBEDFLAGS_DRAWPLANAR; break;
		case '2' : embedFlags = EMBEDFLAGS_SEARCHFORK23; break;
		case '3' : embedFlags = EMBEDFLAGS_SEARCHFORK33; break;
		case '4' : embedFlags = EMBEDFLAGS_SEARCHFORK4; break;
	}

	return embedFlags;
}

