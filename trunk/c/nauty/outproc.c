/***********************************************************************
 Author: John Boyer
 Date: 16 May 2001, 2 Jan 2004, 7 Feb 2009

 This file contains functions that connect McKay's makeg program output
 with planarity-related graph algorithm implementations.
 ***********************************************************************/

#define EXTDEFS
#define MAXN 16
#include "naututil.h"

#include "outproc.h"

#include "../graph.h"
#include "../graphK23Search.h"
#include "../graphK33Search.h"
#include "../graphDrawPlanar.h"

#include <stdlib.h>

graphP theGraph=NULL, origGraph=NULL;

unsigned long numGraphs = 0;
unsigned long numErrors = 0;

unsigned long numNonplanar = 0;
unsigned long numNotOuterplanar = 0;
unsigned long numWithK23 = 0;
unsigned long numWithK33 = 0;

void outprocTest(FILE *f, graph *g, int n, char command, unsigned long *pStat);

void outprocTestPlanarity(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'p', &numNonplanar);
}

void outprocTestDrawPlanar(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'd', &numNonplanar);
}

void outprocTestOuterplanarity(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, 'o', &numNotOuterplanar);
}

void outprocTestK23Search(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, '2', &numWithK23);
}

void outprocTestK33Search(FILE *f, graph *g, int n)
{
    outprocTest(f, g, n, '3', &numWithK33);
}

/***********************************************************************
 CreateGraphs() - creates theGraph given the number of vertices n
 ***********************************************************************/

int CreateGraphs(int n, char command)
{
    if ((theGraph = gp_New()) == NULL ||
    		gp_EnsureEdgeCapacity(theGraph, n*(n-1)/2) != OK ||
    		gp_InitGraph(theGraph, n) != OK)
        return NOTOK;

    if ((origGraph = gp_New()) == NULL ||
    		gp_EnsureEdgeCapacity(origGraph, n*(n-1)/2) != OK ||
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
                     /* If we only stopped because the graph contains too
                        many edges, then the planarity implementation will
                        find the graph to be non-planar without needing
                        any more edges, so we change to a status of OK.
                        But if a real NOTOK error happened, it is reported. */
                     if (ErrorCode == NONEMBEDDABLE)
                         ErrorCode = OK;
                     break;
                 }
             }

			 PO2 >>= 1;
          }
     }

	 return ErrorCode;
}

void outprocTest(FILE *f, graph *g, int n, char command, unsigned long *pStat)
{
	if (theGraph == NULL)
	{
		if (CreateGraphs(n, command) != OK)
		{
			if (!numErrors)
				fprintf(f, "\rUnable to create the graph structure.\n");
			numErrors++;
		}
	}

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
		}

		// Save to origGraph a copy of theGraph in its initial state
		if ((Result = gp_CopyGraph(theGraph, origGraph)) != OK)
			fprintf(f, "\rFailed to copy graph #%lu\n", numGraphs);
		else
		{
			Result = gp_Embed(theGraph, embedFlags);

			if (Result == OK || Result == NONEMBEDDABLE)
			{
				gp_SortVertices(theGraph);

				if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != OK)
				{
					Result = NOTOK;
					if (!numErrors)
						fprintf(f, "\rIntegrity check failed on graph #%lu.\n", numGraphs);
				}
			}
		}

		if (Result == OK)
			;
		else if (Result == NONEMBEDDABLE)
			(*pStat)++;
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
	if (numGraphs % 379 == 0)
	{
		fprintf(f, "\r%ld ", numGraphs);
		fflush(f);
	}
}

/***********************************************************************
 Test_PrintStats() - called by makeg to print the final stats.
 ***********************************************************************/

void Test_PrintStats(FILE *msgfile, char command)
{
char *msg = NULL;
unsigned long result = 0;

    // Print the final number of graphs generated.
    fprintf(msgfile, "\r%ld ", numGraphs);

    // Report the number of errors
    fprintf(msgfile, "# Errors=%ld\n", numErrors);

    // Report the stats
    switch (command)
    {
        case 'p' :
        case 'd' : msg = "not Planar"; result = numNonplanar; break;
        case 'o' : msg = "not Outerplanar"; result = numNotOuterplanar; break;
        case '2' : msg = "with K2,3"; result = numWithK23; break;
        case '3' : msg = "with K3,3"; result = numWithK33; break;
    }

    fprintf(msgfile, "# %s=%ld\n", msg, result);

    gp_Free(&theGraph);
    gp_Free(&origGraph);
}
