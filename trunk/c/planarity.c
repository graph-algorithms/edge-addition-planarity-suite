/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:
John M. Boyer and Wendy J. Myrvold, "On the Cutting Edge: Simplified O(n)
Planarity by Edge Addition,"  Journal of Graph Algorithms and Applications,
Vol. 8, No. 3, pp. 241-273, 2004.

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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "graph.h"
#include "platformTime.h"

#include "graphK23Search.h"
#include "graphK33Search.h"
#include "graphDrawPlanar.h"

void SpecificGraph(int);
void RandomGraph(int extraEdges);
void RandomGraphs(int);
void Reconfigure();

int newCommandLine(int argc, char *argv[]);
int legacyCommandLine(int argc, char *argv[]);
int menu();

/****************************************************************************
 Configuration
 ****************************************************************************/

char Mode='r',
     OrigOut='n',
     EmbeddableOut='n',
     ObstructedOut='n',
     AdjListsForEmbeddingsOut='n',
     menuMode='n';

/****************************************************************************
 MESSAGE - prints a string, but when debugging adds \n and flushes stdout
 ****************************************************************************/

#define MAXLINE 1024
char Line[MAXLINE];

void Message(char *message)
{
	if (menuMode == 'y')
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
    fprintf(stderr, "%s", message);

#ifdef DEBUG
    fprintf(stderr, "\n");
    fflush(stderr);
#endif
}

/****************************************************************************
 MAIN
 ****************************************************************************/

int main(int argc, char *argv[])
{
	if (argc == 1)
		return menu();

	if (argv[1][0] == '-')
		return newCommandLine(argc, argv);

	return legacyCommandLine(argc, argv);
}

int helpMessage()
{
	char menuModeTemp = menuMode;
	menuMode = 'y';

    Message(
    	"'planarity': menu-driven\n"
        "'planarity (-h|-help)': this message\n"
    	"'planarity -unit': runs unit tests\n"
    	"'planarity -nauty ...': runs nauty testing\n"
    	"'planarity -r C K N': Random graphs\n"
    	"'planarity -s C I O [O2]': Specific graph\n"
        "'planarity -m N O [O2]': Maximal planar random graph\n"
        "'planarity -n N O [O2]': Non-planar random graph (maximal planar plus edge)\n"
        "'planarity I O [-n O2]': Legacy command-line (default -s -p)\n"
    	"\n"
    );

    Message(
    	"C = command from menu\n"
    	"    -p = Planar embedding and Kuratowski subgraph isolation\n"
        "    -o = Outerplanar embedding and obstruction isolation\n"
        "    -d = Planar graph drawing\n"
        "    -2 = Search for subgraph homeomorphic to K2,3\n"
        "    -3 = Search for subgraph homeomorphic to K3,3\n"
    	"\n"
    );

    Message(
    	"K = # of graphs to randomly generate\n"
    	"N = # of vertices in each randomly generated graph\n"
        "I = Input file (for work on a specific graph)\n"
        "O = Primary output file\n"
        "    For example, if C=-p then O receives the planar embedding\n"
    	"    If C=-3, then O receives a subgraph containing a K3,3\n"
        "O2= Secondary output file\n"
    	"    For -s, if C=-p or -o, then O2 receives the embedding obstruction\n"
       	"    For -s, if C=-d, then O2 receives a drawing of the planar graph\n"
    	"    For -m and -n, O2 contains the original randomly generated graph\n"
    	"\n"
    );

    menuMode = menuModeTemp;
    return 0;
}

int callNauty(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int callUnitTests(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int callRandomGraphs(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int callSpecificGraph(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int callRandomMaxPlanarGraph(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int callRandomNonplanarGraph(int argc, char *argv[])
{
	ErrorMessage("It's on the to-do list!");
	return 0;
}

int newCommandLine(int argc, char *argv[])
{
	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)
		return helpMessage();

	if (strcmp(argv[1], "-nauty") == 0)
		return callNauty(argc, argv);

	if (strcmp(argv[1], "-unit") == 0)
		return callUnitTests(argc, argv);

	if (strcmp(argv[1], "-r") == 0)
		return callRandomGraphs(argc, argv);

	if (strcmp(argv[1], "-s") == 0)
		return callSpecificGraph(argc, argv);

	if (strcmp(argv[1], "-m") == 0)
		return callRandomMaxPlanarGraph(argc, argv);

	if (strcmp(argv[1], "-n") == 0)
		return callRandomNonplanarGraph(argc, argv);

	ErrorMessage("Unsupported command line.  Here is the help for this program.\n");
	helpMessage();
	return -1;
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

	// In the legacy program, NOTOK was -2 and OK was 0
	return Result==NOTOK ? -2 : 0;
}

/****************************************************************************
 MENU-DRIVEN PROGRAM
 ****************************************************************************/

int menu()
{
int  embedFlags = EMBEDFLAGS_PLANAR;

#ifdef PROFILING  // If profiling, then only run RandomGraphs()

     RandomGraphs(embedFlags);

#else

char Choice;

	 menuMode = 'y';

     do {
        Message("\n=================================================="
                "\nPlanarity Algorithms"
                "\nby John M. Boyer"
                "\n=================================================="
                "\n"
                "\nM. Maximal planar random graph"
                "\nN. Non-planar random graph (maximal planar plus edge)"
                "\nO. Outerplanar embedding and obstruction isolation"
                "\nP. Planar embedding and Kuratowski subgraph isolation"
                "\nD. Planar graph drawing"
                "\n2. Search for subgraph homeomorphic to K2,3"
                "\n3. Search for subgraph homeomorphic to K3,3"
        		"\nH. Help message for command line version"
                "\nR. Reconfigure options"
                "\nX. Exit"
                "\n"
                "\nEnter Choice: ");

        fflush(stdin);
        scanf(" %c", &Choice);
        Choice = tolower(Choice);

        embedFlags = 0;
        switch (Choice)
        {
            case 'm' : RandomGraph(0); break;
            case 'n' : RandomGraph(1); break;
            case 'o' : embedFlags = EMBEDFLAGS_OUTERPLANAR; break;
            case 'p' : embedFlags = EMBEDFLAGS_PLANAR; break;
            case 'd' : embedFlags = EMBEDFLAGS_DRAWPLANAR; break;
            case '2' : embedFlags = EMBEDFLAGS_SEARCHFORK23; break;
            case '3' : embedFlags = EMBEDFLAGS_SEARCHFORK33; break;
            case 'h' : helpMessage(); break;
            case 'r' : Reconfigure(); break;
        }

        if (embedFlags)
        {
            switch (tolower(Mode))
            {
                case 's' : SpecificGraph(embedFlags); break;
                case 'r' : RandomGraphs(embedFlags); break;
            }

            Message("\nPress ENTER to continue...");
            fflush(stdin);
            getc(stdin);
            fflush(stdin);
            Message("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
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

     Message("\nDo you want to randomly generate graphs or specify a graph (r/s)?");
     scanf(" %c", &Mode);

     if (Mode != 's')
     {
        Message("\nNOTE: The directories for the graphs you want must exist.\n\n");

        Message("Do you want original graphs in directory 'random'?");
        scanf(" %c", &OrigOut);

        Message("Do you want adj. matrix of embeddable graphs in directory 'embedded'?");
        scanf(" %c", &EmbeddableOut);

        Message("Do you want adj. matrix of obstructed graphs in directory 'obstructed'?");
        scanf(" %c", &ObstructedOut);

        Message("Do you want adjacency list format of embeddings in directory 'adjlist'?");
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
 Creates a random maximal planar graph, then addes extraEdges edges to it.
 ****************************************************************************/

void RandomGraph(int extraEdges)
{
int  numVertices, Result;
platform_time start, end;
graphP theGraph=NULL, origGraph;

     Message("Enter number of vertices:");
     scanf(" %d", &numVertices);
     if (numVertices <= 0 || numVertices > 1000000)
     {
         ErrorMessage("Must be between 1 and 1000000; changed to 10000\n");
         numVertices = 10000;
     }

     srand(platform_GetTime());

/* Make a graph structure for a graph and the embedding of that graph */

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, numVertices) != OK)
     {
          gp_Free(&theGraph);
          ErrorMessage("Memory allocation/initialization error.\n");
          return;
     }

     start = platform_GetTime();
     if (gp_CreateRandomGraphEx(theGraph, 3*numVertices-6+extraEdges) != OK)
     {
         ErrorMessage("gp_CreateRandomGraphEx() failed\n");
         return;
     }
     end = platform_GetTime();

     sprintf(Line, "Created random graph with %d edges in %.3lf seconds. ", theGraph->M, platform_GetDuration(start,end));
     Message(Line);
     Message("Now processing\n");

#ifdef _DEBUG
     gp_Write(theGraph, "randomGraph.txt", WRITE_ADJLIST);
#endif

     origGraph = gp_DupGraph(theGraph);

     start = platform_GetTime();
     Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);
     end = platform_GetTime();

     if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != OK)
         Result = NOTOK;

     if (Result == OK)
          Message("Planar graph successfully embedded");
     else if (Result == NONEMBEDDABLE)
    	  Message("Nonplanar graph successfully justified");
     else ErrorMessage("Failure occurred");

     sprintf(Line, " in %.3lf seconds.\n", platform_GetDuration(start,end));
     Message(Line);

     SaveAsciiGraph(theGraph, "maxplanar");

     gp_Free(&theGraph);
     gp_Free(&origGraph);
}

/****************************************************************************
 ****************************************************************************/

#define NUM_MINORS  9

void RandomGraphs(int embedFlags)
{
char theFileName[256];
int  Result=OK, I;
int  NumGraphs=0, SizeOfGraphs=0, NumEmbeddableGraphs=0;
int  ObstructionMinorFreqs[NUM_MINORS];
graphP theGraph=NULL;
platform_time start, end;
#ifdef DEBUG
graphP origGraph=NULL;
#endif

	 Message("Enter number of graphs to generate:");
     scanf(" %d", &NumGraphs);

     if (NumGraphs <= 0 || NumGraphs > 10000000)
     {
    	 ErrorMessage("Must be between 1 and 10000000; changed to 100\n");
         NumGraphs = 100;
     }

     Message("Enter size of graphs:");
     scanf(" %d", &SizeOfGraphs);
     if (SizeOfGraphs <= 0 || SizeOfGraphs > 10000)
     {
    	 ErrorMessage("Must be between 1 and 10000; changed to 15\n");
         SizeOfGraphs = 15;
     }

     srand(platform_GetTime());

     for (I=0; I<NUM_MINORS; I++)
          ObstructionMinorFreqs[I] = 0;

/* Reuse Graphs */
// Make a graph structure for a graph

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, SizeOfGraphs) != OK)
     {
    	  ErrorMessage("Error creating space for a graph of the given size.\n");
          return;
     }

// Enable the appropriate feature

//     if (embedFlags != EMBEDFLAGS_SEARCHFORK33)
//         gp_AttachK33Search(theGraph);

     switch (embedFlags)
     {
        case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(theGraph); break;
        case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(theGraph); break;
        case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(theGraph); break;
     }

//     if (embedFlags != EMBEDFLAGS_SEARCHFORK23)
//         gp_AttachK23Search(theGraph);

//     if (embedFlags != EMBEDFLAGS_SEARCHFORK33)
//         gp_RemoveExtension(theGraph, "K33Search");

//     if (embedFlags != EMBEDFLAGS_SEARCHFORK23)
//         gp_RemoveExtension(theGraph, "K23Search");

#ifdef DEBUG
// Make another graph structure to store the original graph that is randomly generated

     if ((origGraph = gp_New()) == NULL || gp_InitGraph(origGraph, SizeOfGraphs) != OK)
     {
    	  ErrorMessage("Error creating space for the second graph structure of the given size.\n");
          return;
     }

// Enable the appropriate feature

     switch (embedFlags)
     {
        case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(origGraph); break;
        case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(origGraph); break;
        case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(origGraph); break;
     }
#endif
/* End Reuse graphs */

     // Generate the graphs and try to embed each

     start = platform_GetTime();

     for (I=0; I < NumGraphs; I++)
     {
/* Use New Graphs */
/*
         // Make a graph structure for a graph

         if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, SizeOfGraphs) != OK)
         {
              ErrorMessage("Error creating space for a graph of the given size.\n");
              return;
         }

         // Enable the appropriate feature

         switch (embedFlags)
         {
            case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(theGraph); break;
            case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(theGraph); break;
            case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(theGraph); break;
         }

#ifdef DEBUG
         // Make another graph structure to store the original graph that is randomly generated

         if ((origGraph = gp_New()) == NULL || gp_InitGraph(origGraph, SizeOfGraphs) != OK)
         {
              ErrorMessage("Error creating space for the second graph structure of the given size.\n");
              return;
         }

         // Enable the appropriate feature

         switch (embedFlags)
         {
            case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(origGraph); break;
            case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(origGraph); break;
            case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(origGraph); break;
         }
#endif
*/
/* End Use New Graphs */

          if (gp_CreateRandomGraph(theGraph) != OK)
          {
        	  ErrorMessage("gp_CreateRandomGraph() failed\n");
              break;
          }

          if (tolower(OrigOut)=='y')
          {
              sprintf(theFileName, "random\\%04d.txt", I+1);
              gp_Write(theGraph, theFileName, WRITE_ADJLIST);
          }

#ifdef DEBUG
          gp_CopyGraph(origGraph, theGraph);
#endif

#ifdef _DEBUG
/* This is useful for capturing the originals of graphs
   that crash or hang the implementation, as opposed to
   producing an error.

          gp_Write(origGraph, "origGraph.txt", WRITE_ADJLIST);
          gp_Write(theGraph, "theGraph.txt", WRITE_ADJLIST);

          gp_ReinitializeGraph(origGraph);
          gp_Read(origGraph, "origGraph.txt");
          gp_Write(origGraph, "origGraphCopy.txt", WRITE_ADJLIST);

          gp_ReinitializeGraph(origGraph);
          gp_CopyGraph(origGraph, theGraph);
*/
#endif

          Result = gp_Embed(theGraph, embedFlags);

#ifdef DEBUG
          if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != OK)
              Result = NOTOK;
#endif

          if (Result == OK)
          {
               NumEmbeddableGraphs++;

               if (tolower(EmbeddableOut) == 'y')
               {
                   sprintf(theFileName, "embedded\\%04d.txt", I+1);
                   gp_Write(theGraph, theFileName, WRITE_ADJMATRIX);
               }

               if (tolower(AdjListsForEmbeddingsOut) == 'y')
               {
                   sprintf(theFileName, "adjlist\\%04d.txt", I+1);
                   gp_Write(theGraph, theFileName, WRITE_ADJLIST);
               }
          }
          else if (Result == NONEMBEDDABLE)
          {
               if (embedFlags == EMBEDFLAGS_PLANAR || embedFlags == EMBEDFLAGS_OUTERPLANAR)
               {
                   if (theGraph->IC.minorType & MINORTYPE_A)
                        ObstructionMinorFreqs[0] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_B)
                        ObstructionMinorFreqs[1] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_C)
                        ObstructionMinorFreqs[2] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_D)
                        ObstructionMinorFreqs[3] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_E)
                        ObstructionMinorFreqs[4] ++;

                   if (theGraph->IC.minorType & MINORTYPE_E1)
                        ObstructionMinorFreqs[5] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_E2)
                        ObstructionMinorFreqs[6] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_E3)
                        ObstructionMinorFreqs[7] ++;
                   else if (theGraph->IC.minorType & MINORTYPE_E4)
                        ObstructionMinorFreqs[8] ++;

                   if (tolower(ObstructedOut) == 'y')
                   {
                       sprintf(theFileName, "obstructed\\%04d.txt", I+1);
                       gp_Write(theGraph, theFileName, WRITE_ADJMATRIX);
                   }
               }
          }

#ifdef DEBUG
          else
          {
               sprintf(theFileName, "error\\%04d.txt", I+1);
               gp_Write(origGraph, theFileName, WRITE_ADJLIST);

               gp_ReinitializeGraph(theGraph);
               gp_CopyGraph(theGraph, origGraph);
               Result = gp_Embed(theGraph, embedFlags);
               if (Result == NOTOK)
               {
            	   ErrorMessage("Error found twice!\n");
               }
               else Result = NOTOK;
          }
#endif

/* Reuse Graphs */
          gp_ReinitializeGraph(theGraph);
#ifdef DEBUG
          gp_ReinitializeGraph(origGraph);
#endif
/* End Reuse Graphs*/

/* Use New Graphs */
/*
          gp_Free(&theGraph);
#ifdef DEBUG
          gp_Free(&origGraph);
#endif
*/
/* End Use New Graphs */

#ifdef DEBUG
          if (menuMode == 'y')
          {
              printf(Line, "%d\r", I+1);
              fflush(stdout);
          }
#endif

          if (Result == NOTOK)
          {
        	  ErrorMessage("\nError found\n");
              break;
          }
     }

// Free the graph structures created before the loop
/* Reuse Graphs */
     gp_Free(&theGraph);
#ifdef DEBUG
     gp_Free(&origGraph);
#endif
/* End Reuse Graphs */

// Print some demographic results

     end = platform_GetTime();
     if (Result != NOTOK)
         Message("\nNo Errors Found.");
     sprintf(Line, "\nDone (%.3lf seconds).\n", platform_GetDuration(start,end));
     Message(Line);

// Report statistics for planar or outerplanar embedding

     if (embedFlags == EMBEDFLAGS_PLANAR || embedFlags == EMBEDFLAGS_OUTERPLANAR)
     {
         sprintf(Line, "Num Embedded=%d.\n", NumEmbeddableGraphs);
         Message(Line);

         for (I=0; I<5; I++)
         {
        	  // Outerplanarity does not produces minors C and D
        	  if (embedFlags == EMBEDFLAGS_OUTERPLANAR && (I==2 || I==3))
        		  continue;

              sprintf(Line, "Minor %c = %d\n", I+'A', ObstructionMinorFreqs[I]);
              Message(Line);
         }

         if (!(embedFlags & ~EMBEDFLAGS_PLANAR))
         {
             sprintf(Line, "\nNote: E1 are added to C, E2 are added to A, and E=E3+E4+K5 homeomorphs.\n");
             Message(Line);

             for (I=5; I<NUM_MINORS; I++)
             {
                  sprintf(Line, "Minor E%d = %d\n", I-4, ObstructionMinorFreqs[I]);
                  Message(Line);
             }
         }
     }

// Report statistics for graph drawing

     else if (embedFlags == EMBEDFLAGS_DRAWPLANAR)
     {
         sprintf(Line, "Num Graphs Embedded and Drawn=%d.\n", NumEmbeddableGraphs);
         Message(Line);
     }

// Report statistics for subgraph homeomorphism algorithms

     else if (embedFlags == EMBEDFLAGS_SEARCHFORK23)
     {
         sprintf(Line, "Of the generated graphs, %d did not contain a K_{2,3} homeomorph as a subgraph.\n", NumEmbeddableGraphs);
         Message(Line);
     }

     else if (embedFlags == EMBEDFLAGS_SEARCHFORK33)
     {
         sprintf(Line, "Of the generated graphs, %d did not contain a K_{3,3} homeomorph as a subgraph.\n", NumEmbeddableGraphs);
         Message(Line);
     }
}

/****************************************************************************
 ****************************************************************************/

void SpecificGraph(int embedFlags)
{
graphP theGraph = gp_New();
char theFileName[100];
int  Result;
char *theMsg = "The embedFlags were incorrectly set.\n";
char *resultStr = "";

/* Set up the result message string and enable features based on the embedFlags */

     // If the planarity bit is set, we may be doing
     // core planarity or an extension algorithms
     if (embedFlags & EMBEDFLAGS_PLANAR)
     {
         // If only the planar flag is set, then we're doing core planarity
         if (!(embedFlags & ~EMBEDFLAGS_PLANAR))
         {
             theMsg = "The graph is%s planar.\n";
         }

         // Otherwise we test for and enable known extension algorithm(s)
         else if (embedFlags == EMBEDFLAGS_DRAWPLANAR)
         {
             theMsg = "The graph is%s planar.\n";
             gp_AttachDrawPlanar(theGraph);
         }
         else if (embedFlags == EMBEDFLAGS_SEARCHFORK33)
         {
             gp_AttachK33Search(theGraph);
             theMsg = "A subgraph homeomorphic to K_{3,3} was%s found.\n";
         }
     }

     // If the outerplanarity bit is set, we may be doing
     // core outerplanarity or an extension algorithms
     else if (embedFlags & EMBEDFLAGS_OUTERPLANAR)
     {
         // If only the outerplanar flag is set, then we're doing core outerplanarity
         if (!(embedFlags & ~EMBEDFLAGS_OUTERPLANAR))
             theMsg = "The graph is%s outerplanar.\n";

         // Otherwise we test for and enable known extension algorithm(s)
         else if (embedFlags == EMBEDFLAGS_SEARCHFORK23)
         {
             gp_AttachK23Search(theGraph);
             theMsg = "A subgraph homeomorphic to K_{2,3} was%s found.\n";
         }
     }

/* Get the filename of the graph to test */

     Message("Enter graph file name:\n");
     scanf(" %s", theFileName);

     if (!strchr(theFileName, '.'))
         strcat(theFileName, ".txt");

/* Read the graph into memory */

     Result = gp_Read(theGraph, theFileName);

     if (Result == NONEMBEDDABLE)
     {
         ErrorMessage("Too many edges... graph is non-planar.  Proceeding...\n");
         Result = OK;
     }

     if (Result != OK)
     {
         ErrorMessage("Failed to read graph\n");
     }
     else
     {
         platform_time start, end;
         graphP origGraph = gp_DupGraph(theGraph);

         start = platform_GetTime();
         Result = gp_Embed(theGraph, embedFlags);
         end = platform_GetTime();
         sprintf(Line, "gp_Embed() completed in %.3lf seconds.\n", platform_GetDuration(start,end));
         Message(Line);

         if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != OK)
         {
             Result = NOTOK;
             ErrorMessage("FAILED integrity check.\n");
         }
         else
             Message("Successful integrity check.\n");

         gp_Free(&origGraph);

         switch (Result)
         {
            case OK :
                 if (embedFlags == EMBEDFLAGS_SEARCHFORK33)
                     resultStr = " not";

                 if (embedFlags == EMBEDFLAGS_SEARCHFORK23)
                     resultStr = " not";

                 break;

            case NONEMBEDDABLE :

                 if (embedFlags == EMBEDFLAGS_PLANAR)
                     resultStr = " not";

                 if (embedFlags == EMBEDFLAGS_DRAWPLANAR)
                     resultStr = " not";

                 if (embedFlags == EMBEDFLAGS_OUTERPLANAR)
                     resultStr = " not";

                 break;

            default :
                 theMsg = "The embedder failed.";
                 break;
         }

         sprintf(Line, theMsg, resultStr);
         Message(Line);

         strcat(theFileName, ".out");

         if (embedFlags == EMBEDFLAGS_DRAWPLANAR && Result == OK)
             gp_DrawPlanar_RenderToFile(theGraph, "render.beforeSort.txt");

         // Restore the vertex ordering of the original graph and the write the result
         gp_SortVertices(theGraph);
         gp_Write(theGraph, theFileName, WRITE_ADJLIST);

         if (embedFlags == EMBEDFLAGS_DRAWPLANAR && Result == OK)
         {
             graphP testGraph = gp_New();

             gp_AttachDrawPlanar(testGraph);

             gp_DrawPlanar_RenderToFile(theGraph, "render.afterSort.txt");

             Result = gp_Read(testGraph, theFileName);

             gp_DrawPlanar_RenderToFile(testGraph, "render.afterRead.txt");
         }
     }

     gp_Free(&theGraph);
}
