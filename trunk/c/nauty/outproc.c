/***********************************************************************
 Author: John Boyer
 Date: 16 May 2001, 2 Jan 2004, 7 Feb 2009

 This file contains functions that connect McKay's makeg program output
 with planarity-related graph algorithm implementations.
 ***********************************************************************/

#define EXTDEFS
#define MAXN 16
#include "naututil.h"
extern int g_maxe, g_mod, g_res;
extern char quietMode;

#include "outproc.h"

#include "../graph.h"
#include "../graphK23Search.h"
#include "../graphK33Search.h"
#include "../graphK4Search.h"
#include "../graphDrawPlanar.h"

#include <stdlib.h>

graphP theGraph=NULL, origGraph=NULL;

unsigned long numGraphs = 0;
unsigned long numErrors = 0;
unsigned long numOKs = 0;

void Test_InitStats()
{
	numGraphs = 0;
	numErrors = 0;
	numOKs = 0;
}

void outprocTest(FILE *f, graph *g, int n, char command);

void outprocTestPlanarity(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'p');
}

void outprocTestDrawPlanar(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'd');
}

void outprocTestOuterplanarity(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'o');
}

void outprocTestK23Search(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, '2');
}

void outprocTestK33Search(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, '3');
}

void outprocTestK4Search(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, '4');
}

/***********************************************************************
 CreateGraphs() - creates theGraph given the number of vertices n
 ***********************************************************************/

int CreateGraphs(int n, char command)
{
	int numArcs = 2*(g_maxe > 0 ? g_maxe : 1);

    if ((theGraph = gp_New()) == NULL ||
    		gp_EnsureArcCapacity(theGraph, numArcs) != OK ||
    		gp_InitGraph(theGraph, n) != OK)
        return NOTOK;

    if ((origGraph = gp_New()) == NULL ||
    		gp_EnsureArcCapacity(origGraph, numArcs) != OK ||
    		gp_InitGraph(origGraph, n) != OK)
    {
        gp_Free(&theGraph);
        return NOTOK;
    }

    switch (command)
    {
		case 'p' :
			break;
		case 'd' :
			gp_AttachDrawPlanar(theGraph);
			gp_AttachDrawPlanar(origGraph);
			break;
		case 'o' :
			break;
		case '2' :
			gp_AttachK23Search(theGraph);
			gp_AttachK23Search(origGraph);
			break;
		case '3' :
			gp_AttachK33Search(theGraph);
			gp_AttachK33Search(origGraph);
			break;
		case '4' :
			gp_AttachK4Search(theGraph);
			gp_AttachK4Search(origGraph);
			break;
		default  :
			return NOTOK;
    }

    return OK;
}

/***********************************************************************
 WriteErrorGraph() - writes a graph to error.txt, giving n in
						decimal then the adj. matrix in hexadecimal.
 ***********************************************************************/

void WriteErrorGraph(graph *g, int n)
{
	FILE *Outfile = fopen("error.txt", "wt");
	register int i;

	fprintf(Outfile, "%d\n", n);

	for (i = 0; i < n; ++i)
		fprintf(Outfile, "%04X\n", g[i]);

	fclose(Outfile);
}

/***********************************************************************
 TransferGraph() - edges from makeg graph g are added to theGraph.
 ***********************************************************************/

int  TransferGraph(graphP theGraph, graph *g, int n)
{
int  i, j, ErrorCode;
unsigned long PO2;

	 gp_ReinitializeGraph(theGraph);

     for (i = 0, ErrorCode = OK; i < n-1 && ErrorCode==OK; i++)
     {
          theGraph->G[i].v = i;

		  PO2 = 1 << (MAXN - 1);
		  PO2 >>= i+1;

          for (j = i+1; j < n; j++)
          {
             if (g[i] & PO2)
             {
                 ErrorCode = gp_AddEdge(theGraph, i, 0, j, 0);
                 if (ErrorCode != OK)
                 {
                     // If we only stopped because the graph contains too
                     // many edges, then we determine whether or not this is
                     // a real error as described below
                     if (ErrorCode == NONEMBEDDABLE)
                     {
                    	 // In the default case of this implementation, the graph's
                    	 // arc capacity is set to accommodate a complete graph,
                    	 // so if there was a failure to add an edge, then some
                    	 // corruption has occurred and we report an error.
                    	 if (gp_GetArcCapacity(theGraph)/2 == (n*(n-1)/2))
                    		 ErrorCode = NOTOK;

                    	 // Many of the algorithms require only a small sampling
                    	 // of edges to be successful.  For example, planar embedding
                    	 // and Kuratowski subgraph isolation required only 3n-5
                    	 // edges.  K3,3 search requires only 3n edges from the input
                    	 // graph.  If a user modifies the test code to exploit this
                    	 // lower limit, then we permit the failure to add an edge since
                    	 // the failure to add an edge is expected.
                    	 else
                    		 ErrorCode = OK;
                     }
                     break;
                 }
             }

			 PO2 >>= 1;
          }
     }

	 return ErrorCode;
}

void outprocTest(FILE *f, graph *g, int n, char command)
{
	if (theGraph == NULL)
	{
		Test_InitStats();

		if (CreateGraphs(n, command) != OK)
		{
			if (!numErrors)
				fprintf(f, "\rUnable to create the graph structure.\n");
			numErrors++;
		}
	}

	// Copy from the nauty graph to the origGraph
	if (TransferGraph(origGraph, g, n) != OK)
	{
		if (!numErrors)
		{
			numErrors++;
			fprintf(f, "\rFailed to initialize with generated graph in error.txt\n");
			WriteErrorGraph(g, n);
		}
		else numErrors++;
	}

	else
	{
		int Result = OK, embedFlags = EMBEDFLAGS_PLANAR;

		switch (command)
		{
			case 'o' : embedFlags = EMBEDFLAGS_OUTERPLANAR; break;
			case 'p' : embedFlags = EMBEDFLAGS_PLANAR; break;
			case 'd' : embedFlags = EMBEDFLAGS_DRAWPLANAR; break;
			case '2' : embedFlags = EMBEDFLAGS_SEARCHFORK23; break;
			case '3' : embedFlags = EMBEDFLAGS_SEARCHFORK33; break;
			case '4' : embedFlags = EMBEDFLAGS_SEARCHFORK4; break;
		}

		// Now copy from the origGraph into theGraph on which the work will be done
		if ((Result = gp_CopyGraph(theGraph, origGraph)) != OK)
			fprintf(f, "\rFailed to copy graph #%lu\n", numGraphs);
		else
		{
			Result = gp_Embed(theGraph, embedFlags);

			if (Result == OK || Result == NONEMBEDDABLE)
			{
				gp_SortVertices(theGraph);

				if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != Result)
				{
					Result = NOTOK;
					if (!numErrors)
						fprintf(f, "\rIntegrity check failed on graph #%lu.\n", numGraphs);
				}
			}
		}

		if (Result == OK)
			numOKs++;
		else if (Result == NONEMBEDDABLE)
			;
		else
		{
			if (!numErrors)
			{
				numErrors++;
				fprintf(f, "\rFailed on graph #%lu.  Written to error.txt and error_adj.txt\n", numGraphs);
				WriteErrorGraph(g, n);
				gp_Write(origGraph, "error_adj.txt", WRITE_ADJLIST);
			}
			else numErrors++;
		}
	}

	numGraphs++;

	if (quietMode == 'n')
	{
#ifndef DEBUG
		if (numGraphs % 379 == 0)
#endif
		{
			fprintf(f, "\r%ld ", numGraphs);
			fflush(f);
		}
	}
}

/***********************************************************************
 Test_PrintStats() - called by makeg to print the final stats.
 ***********************************************************************/

void Test_PrintStats(FILE *msgfile, char command)
{
char *msg = NULL;

	if (quietMode == 'n')
	{
	    // Print the final counter value
	    fprintf(msgfile, "\r%ld \n", numGraphs);

	    // Report the number of graphs, and the modulus and residue class of the generator
	    if (g_mod > 1)
	    	fprintf(msgfile, "# Graphs=%ld, mod=%d, res=%d\n", numGraphs, g_mod, g_res);
	    else
	    	fprintf(msgfile, "# Graphs=%ld\n", numGraphs);

	    // Report the number of errors
	    fprintf(msgfile, "# Errors=%ld\n", numErrors);

	    // Report the stats
	    switch (command)
	    {
	        case 'p' :
	        case 'd' : msg = "not Planar"; break;
	        case 'o' : msg = "not Outerplanar"; break;
	        case '2' : msg = "with K2,3"; break;
	        case '3' : msg = "with K3,3"; break;
	        case '4' : msg = "with K4"; break;
	    }

	    fprintf(msgfile, "# %s=%ld\n", msg, numGraphs - numOKs);
	}

    gp_Free(&theGraph);
    gp_Free(&origGraph);
}
