/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runLoopEdgeTests(void);

/* Private functions */
static int _checkForExpectedLoopEdges(graphP theGraph, int expectedNumLoops, int expectedNumDirectedLoops);
static int _checkForExpectedDegreesInLoopEdgeGraph(graphP theGraph);
static int runBasicLoopEdgeTests(void);
static int runDFSandEmbeddingLoopEdgeTests(void);

/*************************************************************************
 *************************************************************************/

int runLoopEdgeTests(void)
{
    int retVal = OK;

    gp_Message("Starting Loop Edge Tests");

    if (runBasicLoopEdgeTests() != OK)
        retVal = NOTOK;

    if (runDFSandEmbeddingLoopEdgeTests() != OK)
        retVal = NOTOK;

    if (retVal == OK)
        gp_Message("Finished Loop Edge Tests.\n");
    else
        gp_ErrorMessage("A test of loop edges has failed.\n");

    return retVal;
}

int _checkForExpectedLoopEdges(graphP theGraph, int expectedNumLoops, int expectedNumDirectedLoops)
{
    if (theGraph == NULL)
    {
        gp_ErrorMessage("Invalid parameters.");
        return FALSE;
    }

    for (int v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); ++v)
    {
        int numLoops = 0, numDirectedLoops = 0;

        int e = gp_GetFirstEdge(theGraph, v);
        while (gp_IsEdge(theGraph, e))
        {
            if (gp_GetNeighbor(theGraph, e) == v && e < gp_GetTwin(theGraph, e))
            {
                numLoops++;
                if (gp_GetDirection(theGraph, e))
                    numDirectedLoops++;
            }
            e = gp_GetNextEdge(theGraph, e);
        }

        if (numLoops != expectedNumLoops)
            return FALSE;
        if (expectedNumDirectedLoops >= 0 && numDirectedLoops != expectedNumDirectedLoops)
            return FALSE;
    }

    return TRUE;
}

int _checkForExpectedDegreesInLoopEdgeGraph(graphP theGraph)
{
    if (theGraph == NULL)
    {
        gp_ErrorMessage("Invalid parameters.");
        return FALSE;
    }

    for (int v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); ++v)
    {
        if (gp_GetVertexDegree(theGraph, v) != 9)
            return FALSE;
        if (gp_GetVertexInDegree(theGraph, v) != 6)
            return FALSE;
        if (gp_GetVertexOutDegree(theGraph, v) != 6)
            return FALSE;
    }

    return TRUE;
}

int runBasicLoopEdgeTests(void)
{
    int retVal = OK;
    graphP G = gp_New();
    graphP G1 = gp_New();
    char *graphSerialization = NULL;

    if (G == NULL || G1 == NULL)
    {
        gp_ErrorMessage("Unable to allocate a graph.");
        retVal = NOTOK;
    }

    else if (gp_Read(G, "Petersen-with-loop-edges.txt") != OK ||
             gp_GetM(G) != 45 ||
             !_checkForExpectedLoopEdges(G, 3, 1))
    {
        gp_ErrorMessage("Failed to correctly read graph with loop edges.");
        retVal = NOTOK;
    }
    else if (gp_WriteToString(G, &graphSerialization, WRITE_ADJLIST) != OK ||
             gp_ReadFromString(G1, graphSerialization) != OK ||
             gp_GetM(G1) != 45 ||
             !_checkForExpectedLoopEdges(G1, 3, 1))
    {
        gp_ErrorMessage("Failed to correctly write graph with loop edges.");
        retVal = NOTOK;
    }
    else
    {
        gp_Free(&G1);

        if ((G1 = gp_DupGraph(G)) == NULL ||
            gp_GetM(G1) != 45 ||
            !_checkForExpectedLoopEdges(G1, 3, 1))
        {
            gp_ErrorMessage("Failed to correctly duplicate graph with loop edges.");
            retVal = NOTOK;
        }
        else if (!_checkForExpectedDegreesInLoopEdgeGraph(G1))
        {
            gp_ErrorMessage("Unexpected degree values for vertices in graph with loop edges.");
            retVal = NOTOK;
        }
        else if (!gp_HasLoopEdges(G1))
        {
            gp_ErrorMessage("Unexpected result from gp_HasLoopEdges().");
            retVal = NOTOK;
        }
        else if (!gp_HasParallelEdges(G1, TRUE) || !gp_HasParallelEdges(G1, FALSE))
        {
            gp_ErrorMessage("Unexpected result from gp_HasParallelEdges().");
            retVal = NOTOK;
        }
        else if (gp_DeleteParallelEdges(G1, FALSE) != OK ||
                 gp_GetM(G1) != 35 ||
                 !_checkForExpectedLoopEdges(G1, 2, 1))
        {
            gp_ErrorMessage("Failed to delete parallel undirected loop edges.");
            retVal = NOTOK;
        }
        else if (!gp_HasLoopEdges(G1))
        {
            gp_ErrorMessage("Unexpected result from gp_HasLoopEdges().");
            retVal = NOTOK;
        }
        else if (!gp_HasParallelEdges(G1, TRUE) || gp_HasParallelEdges(G1, FALSE))
        {
            gp_ErrorMessage("Unexpected result from gp_HasParallelEdges().");
            retVal = NOTOK;
        }
        else if (gp_DeleteParallelEdges(G1, TRUE) != OK ||
                 gp_GetM(G1) != 25 ||
                 !_checkForExpectedLoopEdges(G1, 1, -1))
        {
            gp_ErrorMessage("Failed to delete parallel loop edges. %d", gp_GetM(G1));
            retVal = NOTOK;
        }
        else if (!gp_HasLoopEdges(G1))
        {
            gp_ErrorMessage("Unexpected result from gp_HasLoopEdges().");
            retVal = NOTOK;
        }
        else if (gp_HasParallelEdges(G1, TRUE) || gp_HasParallelEdges(G1, FALSE))
        {
            gp_ErrorMessage("Unexpected result from gp_HasParallelEdges().");
            retVal = NOTOK;
        }
        else if (gp_DeleteLoopEdges(G1) != OK ||
                 gp_GetM(G1) != 15)
        {
            gp_ErrorMessage("Failed to delete loop edges.");
            retVal = NOTOK;
        }
        else if (gp_HasLoopEdges(G1))
        {
            gp_ErrorMessage("Unexpected result from gp_HasLoopEdges().");
            retVal = NOTOK;
        }
        else
        {
            gp_Free(&G1);

            if ((G1 = gp_DupGraph(G)) == NULL ||
                gp_GetM(G1) != 45 ||
                !_checkForExpectedLoopEdges(G1, 3, 1))
            {
                gp_ErrorMessage("Failed to correctly duplicate graph with loop edges.");
                retVal = NOTOK;
            }
            else if (gp_DeleteLoopEdges(G1) != OK ||
                     gp_GetM(G1) != 15)
            {
                gp_ErrorMessage("Failed to delete loop edges.");
                retVal = NOTOK;
            }
            else if (gp_HasLoopEdges(G1))
            {
                gp_ErrorMessage("Unexpected result from gp_HasLoopEdges().");
                retVal = NOTOK;
            }
        }
    }

    gp_Free(&G);
    gp_Free(&G1);
    if (graphSerialization != NULL)
        free(graphSerialization);

    return retVal;
}

int runDFSandEmbeddingLoopEdgeTests(void)
{
    return OK;
}
