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
 RandomGraphs()
 ****************************************************************************/

#define NUM_MINORS  9

int  RandomGraphs(char command, int NumGraphs, int SizeOfGraphs)
{
char theFileName[256];
int  Result=OK, I;
int  NumEmbeddableGraphs=0;
int  ObstructionMinorFreqs[NUM_MINORS];
graphP theGraph=NULL;
platform_time start, end;
graphP origGraph=NULL;
int embedFlags = GetEmbedFlags(command);

     if (NumGraphs == 0)
     {
	     Prompt("Enter number of graphs to generate:");
         scanf(" %d", &NumGraphs);
     }

     if (NumGraphs <= 0 || NumGraphs > 1000000000)
     {
    	 ErrorMessage("Must be between 1 and 1000000000; changed to 100\n");
         NumGraphs = 100;
     }

     if (SizeOfGraphs == 0)
     {
         Prompt("Enter size of graphs:");
         scanf(" %d", &SizeOfGraphs);
     }

     if (SizeOfGraphs <= 0 || SizeOfGraphs > 10000)
     {
    	 ErrorMessage("Must be between 1 and 10000; changed to 15\n");
         SizeOfGraphs = 15;
     }

     srand(time(NULL));

     for (I=0; I<NUM_MINORS; I++)
          ObstructionMinorFreqs[I] = 0;

/* Reuse Graphs */
// Make a graph structure for a graph

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, SizeOfGraphs) != OK)
     {
    	  ErrorMessage("Error creating space for a graph of the given size.\n");
          return  NOTOK;
     }

// Enable the appropriate feature

//     if (embedFlags != EMBEDFLAGS_SEARCHFORK33)
//         gp_AttachK33Search(theGraph);

     switch (embedFlags)
     {
        case EMBEDFLAGS_SEARCHFORK4  : gp_AttachK4Search(theGraph); break;
        case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(theGraph); break;
        case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(theGraph); break;
        case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(theGraph); break;
     }

// Make another graph structure to store the original graph that is randomly generated

     if ((origGraph = gp_New()) == NULL || gp_InitGraph(origGraph, SizeOfGraphs) != OK)
     {
    	  ErrorMessage("Error creating space for the second graph structure of the given size.\n");
          return NOTOK;
     }

// Enable the appropriate feature

     switch (embedFlags)
     {
        case EMBEDFLAGS_SEARCHFORK4  : gp_AttachK4Search(origGraph); break;
        case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(origGraph); break;
        case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(origGraph); break;
        case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(origGraph); break;
     }

/* End Reuse graphs */

     fprintf(stdout, "0\r");
     fflush(stdout);

     // Generate the graphs and try to embed each

     platform_GetTime(start);

     for (I=0; I < NumGraphs; I++)
     {
/* Use New Graphs */
/*
         // Make a graph structure for a graph

         if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, SizeOfGraphs) != OK)
         {
              ErrorMessage("Error creating space for a graph of the given size.\n");
              return NOTOK;
         }

         // Enable the appropriate feature

         switch (embedFlags)
         {
            case EMBEDFLAGS_SEARCHFORK4  : gp_AttachK4Search(theGraph); break;
            case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(theGraph); break;
            case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(theGraph); break;
            case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(theGraph); break;
         }

         // Make another graph structure to store the original graph that is randomly generated

         if ((origGraph = gp_New()) == NULL || gp_InitGraph(origGraph, SizeOfGraphs) != OK)
         {
              ErrorMessage("Error creating space for the second graph structure of the given size.\n");
              return NOTOK;
         }

         // Enable the appropriate feature

         switch (embedFlags)
         {
            case EMBEDFLAGS_SEARCHFORK4  : gp_AttachK4Search(origGraph); break;
            case EMBEDFLAGS_SEARCHFORK33 : gp_AttachK33Search(origGraph); break;
            case EMBEDFLAGS_SEARCHFORK23 : gp_AttachK23Search(origGraph); break;
            case EMBEDFLAGS_DRAWPLANAR   : gp_AttachDrawPlanar(origGraph); break;
         }
*/
/* End Use New Graphs */

          if (gp_CreateRandomGraph(theGraph) != OK)
          {
        	  ErrorMessage("gp_CreateRandomGraph() failed\n");
        	  Result = NOTOK;
              break;
          }

          if (tolower(OrigOut)=='y')
          {
              sprintf(theFileName, "random\\%d.txt", I%10);
              gp_Write(theGraph, theFileName, WRITE_ADJLIST);
          }

          gp_CopyGraph(origGraph, theGraph);

          Result = gp_Embed(theGraph, embedFlags);

          if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != Result)
              Result = NOTOK;

          if (Result == OK)
          {
               NumEmbeddableGraphs++;

               if (tolower(EmbeddableOut) == 'y')
               {
                   sprintf(theFileName, "embedded\\%d.txt", I%10);
                   gp_Write(theGraph, theFileName, WRITE_ADJMATRIX);
               }

               if (tolower(AdjListsForEmbeddingsOut) == 'y')
               {
                   sprintf(theFileName, "adjlist\\%d.txt", I%10);
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
                       sprintf(theFileName, "obstructed\\%d.txt", I%10);
                       gp_Write(theGraph, theFileName, WRITE_ADJMATRIX);
                   }
               }
          }

          else
          {
               sprintf(theFileName, "error\\%d.txt", I%10);
               gp_Write(origGraph, theFileName, WRITE_ADJLIST);

               gp_ReinitializeGraph(theGraph);
               gp_CopyGraph(theGraph, origGraph);
               Result = gp_Embed(theGraph, embedFlags);
               if (Result != OK && Result != NONEMBEDDABLE)
               {
            	   ErrorMessage("Error found twice!\n");
               }
               Result = NOTOK;
          }

/* Reuse Graphs */
          gp_ReinitializeGraph(theGraph);
          gp_ReinitializeGraph(origGraph);

/* End Reuse Graphs*/

/* Use New Graphs */
/*
          gp_Free(&theGraph);
          gp_Free(&origGraph);
*/
/* End Use New Graphs */

//#ifdef DEBUG
          if (quietMode == 'n' && (I+1) % 379 == 0)
          {
              fprintf(stdout, "%d\r", I+1);
              fflush(stdout);
          }
//#endif

          if (Result != OK && Result != NONEMBEDDABLE)
          {
        	  ErrorMessage("\nError found\n");
              Result = NOTOK;
              break;
          }
     }

     platform_GetTime(end);

     // Finish the count
     fprintf(stdout, "%d\n", NumGraphs);
     fflush(stdout);

// Free the graph structures created before the loop
/* Reuse Graphs */
     gp_Free(&theGraph);
     gp_Free(&origGraph);
/* End Reuse Graphs */

// Print some demographic results

     if (Result == OK || Result == NONEMBEDDABLE)
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

     else if (embedFlags == EMBEDFLAGS_SEARCHFORK4)
     {
         sprintf(Line, "Of the generated graphs, %d did not contain a K_4 homeomorph as a subgraph.\n", NumEmbeddableGraphs);
         Message(Line);
     }

     FlushConsole(stdout);

     return Result==OK || Result==NONEMBEDDABLE ? OK : NOTOK;
}

/****************************************************************************
 Creates a random maximal planar graph, then adds 'extraEdges' edges to it.
 ****************************************************************************/

int RandomGraph(char command, int extraEdges, int numVertices, char *outfileName, char *outfile2Name)
{
int  Result;
platform_time start, end;
graphP theGraph=NULL, origGraph;
int embedFlags = GetEmbedFlags(command);
char saveEdgeListFormat;

     if (embedFlags != EMBEDFLAGS_PLANAR)
     {
    	 ErrorMessage("Random max planar graph and non-planar modes only support planarity command\n");
    	 return NOTOK;
     }

     if (numVertices <= 0)
     {
         Prompt("Enter number of vertices:");
         scanf(" %d", &numVertices);
         if (numVertices <= 0 || numVertices > 1000000)
         {
             ErrorMessage("Must be between 1 and 1000000; changed to 10000\n");
             numVertices = 10000;
         }
     }

     srand(time(NULL));

/* Make a graph structure for a graph and the embedding of that graph */

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, numVertices) != OK)
     {
          gp_Free(&theGraph);
          ErrorMessage("Memory allocation/initialization error.\n");
          return NOTOK;
     }

     platform_GetTime(start);
     if (gp_CreateRandomGraphEx(theGraph, 3*numVertices-6+extraEdges) != OK)
     {
         ErrorMessage("gp_CreateRandomGraphEx() failed\n");
         return NOTOK;
     }
     platform_GetTime(end);

     sprintf(Line, "Created random graph with %d edges in %.3lf seconds. ", theGraph->M, platform_GetDuration(start,end));
     Message(Line);
     Message("Now processing\n");
     FlushConsole(stdout);

     if (outfile2Name != NULL)
     {
         gp_Write(theGraph, outfile2Name, WRITE_ADJLIST);
     }

     origGraph = gp_DupGraph(theGraph);

     platform_GetTime(start);
     Result = gp_Embed(theGraph, embedFlags);
     platform_GetTime(end);

     sprintf(Line, "Finished processing in %.3lf seconds. Testing integrity of result...\n", platform_GetDuration(start,end));
     Message(Line);

	 gp_SortVertices(theGraph);

     if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != Result)
         Result = NOTOK;

     if (Result == OK)
          Message("Planar graph successfully embedded\n");
     else if (Result == NONEMBEDDABLE)
    	  Message("Nonplanar graph successfully justified\n");
     else ErrorMessage("Failure occurred");

     if (Result == OK || Result == NONEMBEDDABLE)
     {
    	 if (outfileName != NULL)
    		 gp_Write(theGraph, outfileName, WRITE_ADJLIST);
     }

     Prompt("Do you want to save the generated graph in edge list format (y/n)? ");
     fflush(stdin);
     scanf(" %c", &saveEdgeListFormat);
     if (tolower(saveEdgeListFormat) == 'y')
     {
    	 char *fileName = "maxPlanarEdgeList.txt";
         if (extraEdges > 0)
        	 fileName = "nonPlanarEdgeList.txt";

         SaveAsciiGraph(theGraph, fileName);
         sprintf(Line, "Edge list format saved to '%s'\n", fileName);
    	 Message(Line);
     }

     gp_Free(&theGraph);
     gp_Free(&origGraph);

     FlushConsole(stdout);
     return Result;
}
