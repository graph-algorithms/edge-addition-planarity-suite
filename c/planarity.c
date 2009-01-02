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

/****************************************************************************
 Configuration
 ****************************************************************************/

char Mode='r',
     OrigOut='n',
     EmbeddableOut='n',
     ObstructedOut='n',
     AdjListsForEmbeddingsOut='n';

/****************************************************************************
 MAIN
 ****************************************************************************/

int main()
{
int  embedFlags = EMBEDFLAGS_PLANAR;

#ifdef PROFILING  // If profiling, then only run RandomGraphs()
     RandomGraphs(embedFlags);
#else
char Choice;

     do {
        printf("\n=================================================="
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
               "\nR. Reconfigure options"
               "\nX. Exit"
               "\n"
               "\nEnter Choice: \n");
        fflush(stdout);

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
            case 'r' : Reconfigure(); break;
        }

        if (embedFlags)
        {
            switch (tolower(Mode))
            {
                case 's' : SpecificGraph(embedFlags); break;
                case 'r' : RandomGraphs(embedFlags); break;
            }

            printf("\nPress ENTER to continue...\n");
            fflush(stdout);
            fflush(stdin);
            getc(stdin);
            fflush(stdin);
            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            fflush(stdout);
        }
     }  while (Choice != 'x');
#endif

     return OK;
}

/****************************************************************************
 ****************************************************************************/

void Reconfigure()
{
     fflush(stdin);

     printf("\nDo you want to randomly generate graphs or specify a graph (r/s)?\n");
     fflush(stdout);
     scanf(" %c", &Mode);

     if (Mode != 's')
     {
        printf("\nNOTE: The directories for the graphs you want must exist.\n\n");
        fflush(stdout);

        printf("Do you want original graphs in directory 'random'?");
        fflush(stdout);
        scanf(" %c", &OrigOut);

        printf("Do you want adj. matrix of embeddable graphs in directory 'embedded'?");
        fflush(stdout);
        scanf(" %c", &EmbeddableOut);

        printf("Do you want adj. matrix of obstructed graphs in directory 'obstructed'?");
        fflush(stdout);
        scanf(" %c", &ObstructedOut);

        printf("Do you want adjacency list format of embeddings in directory 'adjlist'?");
        fflush(stdout);
        scanf(" %c", &AdjListsForEmbeddingsOut);
     }

     printf("\n");
     fflush(stdout);
}

/****************************************************************************
 ****************************************************************************/

void SaveAsciiGraph(graphP theGraph, char *graphName)
{
char Ch;

    printf("Do you want to save the graph in Ascii format (to test.dat)?");
    fflush(stdout);
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

     printf("Enter number of vertices:");
     fflush(stdout);
     scanf(" %d", &numVertices);
     if (numVertices <= 0 || numVertices > 1000000)
     {
         printf("Must be between 1 and 1000000; changed to 10000\n");
         fflush(stdout);
         numVertices = 10000;
     }

     srand(platform_GetTime());

/* Make a graph structure for a graph and the embedding of that graph */

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, numVertices) != OK)
     {
          gp_Free(&theGraph);
          printf("Memory allocation/initialization error.\n");
          fflush(stdout);
          return;
     }

     start = platform_GetTime();
     if (gp_CreateRandomGraphEx(theGraph, 3*numVertices-6+extraEdges) != OK)
     {
         printf("gp_CreateRandomGraphEx() failed\n");
         fflush(stdout);
         return;
     }
     end = platform_GetTime();
     printf("Created random graph with %d edges in %.3lf seconds. Now processing\n", theGraph->M, platform_GetDuration(start,end));
     fflush(stdout);

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
          printf("Planar graph successfully embedded");
     else if (Result == NONEMBEDDABLE)
          printf("Nonplanar graph successfully justified");
     else printf("Failure occurred");

     printf(" in %.3lf seconds.\n", platform_GetDuration(start,end));
     fflush(stdout);

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
int  Result, I;
int  NumGraphs=0, SizeOfGraphs=0, NumEmbeddableGraphs=0;
int  ObstructionMinorFreqs[NUM_MINORS];
graphP theGraph=NULL;
platform_time start, end;
#ifdef DEBUG
graphP origGraph=NULL;
#endif

     printf("Enter number of graphs to generate:");
     fflush(stdout);
     scanf(" %d", &NumGraphs);

     if (NumGraphs <= 0 || NumGraphs > 10000000)
     {
         printf("Must be between 1 and 10000000; changed to 100\n");
         fflush(stdout);
         NumGraphs = 100;
     }

     printf("Enter size of graphs:");
     fflush(stdout);
     scanf(" %d", &SizeOfGraphs);
     if (SizeOfGraphs <= 0 || SizeOfGraphs > 10000)
     {
         printf("Must be between 1 and 10000; changed to 15\n");
         fflush(stdout);
         SizeOfGraphs = 15;
     }

     srand(platform_GetTime());

     for (I=0; I<NUM_MINORS; I++)
          ObstructionMinorFreqs[I] = 0;

/* Reuse Graphs */
// Make a graph structure for a graph

     if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, SizeOfGraphs) != OK)
     {
          printf("Error creating space for a graph of the given size.\n");
          fflush(stdout);
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
          printf("Error creating space for the second graph structure of the given size.\n");
          fflush(stdout);
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
              printf("Error creating space for a graph of the given size.\n");
              fflush(stdout);
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
              printf("Error creating space for the second graph structure of the given size.\n");
              fflush(stdout);
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
              printf("gp_CreateRandomGraph() failed\n");
              fflush(stdout);
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
                   printf("Error found twice!\n");
                   fflush(stdout);
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
          printf("%d\r", I+1);
          fflush(stdout);
#endif

          if (Result == NOTOK)
          {
              printf("\nError found\n");
              fflush(stdout);
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
     printf("\nDone (%.3lf seconds).\n", platform_GetDuration(start,end));
     fflush(stdout);

// Report statistics for planar or outerplanar embedding

     if (embedFlags == EMBEDFLAGS_PLANAR || embedFlags == EMBEDFLAGS_OUTERPLANAR)
     {
         printf("Num Embedded=%d.\n", NumEmbeddableGraphs);

         for (I=0; I<5; I++)
              printf("Minor %c = %d\n", I+'A', ObstructionMinorFreqs[I]);

         if (!(embedFlags & ~EMBEDFLAGS_PLANAR))
         {
             printf("\nNote: E1 are added to C, E2 are added to A, and E=E3+E4+K5 homeomorphs.\n");

             for (I=5; I<NUM_MINORS; I++)
                  printf("Minor E%d = %d\n", I-4, ObstructionMinorFreqs[I]);
         }
     }

// Report statistics for graph drawing

     else if (embedFlags == EMBEDFLAGS_DRAWPLANAR)
         printf("Num Graphs Embedded and Drawn=%d.\n", NumEmbeddableGraphs);

// Report statistics for subgraph homeomorphism algorithms

     else if (embedFlags == EMBEDFLAGS_SEARCHFORK23)
         printf("Of the generated graphs, %d did not contain a K_{2,3} homeomorph as a subgraph.\n", NumEmbeddableGraphs);

     else if (embedFlags == EMBEDFLAGS_SEARCHFORK33)
         printf("Of the generated graphs, %d did not contain a K_{3,3} homeomorph as a subgraph.\n", NumEmbeddableGraphs);

     fflush(stdout);
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

     printf("Enter graph file name:");
     fflush(stdout);
     scanf(" %s", theFileName);

     if (!strchr(theFileName, '.'))
         strcat(theFileName, ".txt");

/* Read the graph into memory */

     Result = gp_Read(theGraph, theFileName);

     if (Result == NONEMBEDDABLE)
     {
         printf("Too many edges... graph is non-planar.  Proceeding...\n");
         fflush(stdout);
         Result = OK;
     }

     if (Result != OK)
     {
         printf("Failed to read graph\n");
         fflush(stdout);
     }
     else
     {
         platform_time start, end;
         graphP origGraph = gp_DupGraph(theGraph);

         start = platform_GetTime();
         Result = gp_Embed(theGraph, embedFlags);
         end = platform_GetTime();
         printf("gp_Embed() completed in %.3lf seconds.\n", platform_GetDuration(start,end));
         fflush(stdout);

         if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != OK)
         {
             Result = NOTOK;
             printf("FAILED integrity check.\n");
         }
         else
             printf("Successful integrity check.\n");
         fflush(stdout);

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

         printf(theMsg, resultStr);
         fflush(stdout);

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

