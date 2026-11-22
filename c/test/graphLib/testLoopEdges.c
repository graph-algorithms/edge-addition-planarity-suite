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

static int _VerifyLoopsAreBackEdges(graphP theGraph, int observeDirection);
static int runDFSandEmbeddingLoopEdgeTests(void);

/*************************************************************************
 *************************************************************************/

int runLoopEdgeTests(void)
{
    int retVal = OK;

    gp_Message("Starting Loop Edge Tests");

    if (runBasicLoopEdgeTests() != OK)
        retVal = NOTOK;
    else if (runDFSandEmbeddingLoopEdgeTests() != OK)
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

    gp_Message("Testing basic I/O on graphs containing undirected and directed loop edges.");

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

        gp_Message("Testing parallel and non-parallel loop edge duplication, detection, and deletion.");

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

            gp_Message("Testing simultaneous deletion of parallel and non-parallel loop edges.");

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

static int _VerifyLoopsAreBackEdges(graphP theGraph, int observeDirection)
{
    for (int e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); e += 2)
    {
        if (gp_EdgeInUse(theGraph, e))
        {
            int eTwin = gp_GetTwin(theGraph, e);

            // A loop edge has the same neighbor in e and eTwin
            if (gp_GetNeighbor(theGraph, e) == gp_GetNeighbor(theGraph, eTwin))
            {
                // If not observing directionality of the loop, then either edge record
                // can be BACK edge record and the other has to be the FORWARD edge record.
                if (!observeDirection)
                {
                    if ((gp_GetEdgeType(theGraph, e) == EDGE_TYPE_BACK && gp_GetEdgeType(theGraph, eTwin) == EDGE_TYPE_FORWARD) ||
                        (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_FORWARD && gp_GetEdgeType(theGraph, eTwin) == EDGE_TYPE_BACK))
                        ;
                    // Otherwise the loop edge has the incorrect type for an undirected DFS
                    else
                        return FALSE;
                }

                // For a directed DFS, if the edge is directed, then it is the out-only edge record that
                // goes BACK to the visited vertex (the same self, in this case).
                else
                {
                    // Unless e is in-only, we can exit the vertex from it.
                    int e_out = gp_GetDirection(theGraph, e) == EDGEFLAG_DIRECTION_INONLY ? eTwin : e;

                    eTwin = gp_GetTwin(theGraph, e_out);

                    if (gp_GetDirection(theGraph, e_out) == EDGEFLAG_DIRECTION_OUTONLY)
                    {
                        if (gp_GetEdgeType(theGraph, e_out) == EDGE_TYPE_BACK && gp_GetEdgeType(theGraph, eTwin) == EDGE_TYPE_FORWARD)
                            ;
                        // Otherwise the loop edge has the incorrect type for a directed loop edge in a directed DFS
                        else
                            return FALSE;
                    }
                    // For an undirected loop edge, even in a directed DFS, either BACK/FORWARD type setting is appropriate
                    else
                    {
                        if ((gp_GetEdgeType(theGraph, e_out) == EDGE_TYPE_BACK && gp_GetEdgeType(theGraph, eTwin) == EDGE_TYPE_FORWARD) ||
                            (gp_GetEdgeType(theGraph, e_out) == EDGE_TYPE_FORWARD && gp_GetEdgeType(theGraph, eTwin) == EDGE_TYPE_BACK))
                            ;
                        else
                            return FALSE;
                    }
                }
            }
        }
    }

    return TRUE;
}

int runDFSandEmbeddingLoopEdgeTests(void)
{
    int retVal = OK;
    graphP G = gp_New();
    graphP G1 = NULL;

    gp_Message("Testing undirected depth-first search behavior on loop edges.");

    if (G == NULL)
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
    else if ((G1 = gp_DupGraph(G)) == NULL ||
             gp_GetM(G1) != 45 ||
             !_checkForExpectedLoopEdges(G1, 3, 1))
    {
        gp_ErrorMessage("Failed to duplicate graph with loop edges.");
        retVal = NOTOK;
    }
    else if (gp_DepthFirstSearchEx(G1, DFSMODE_UNDIRECTED) != OK)
    {
        gp_ErrorMessage("Undirected DFS failed on graph with loop edges.");
        retVal = NOTOK;
    }
    else if (!_VerifyLoopsAreBackEdges(G1, FALSE))
    {
        gp_ErrorMessage("Loop edges were not all marked as back edges after undirected DFS.");
        retVal = NOTOK;
    }
    else
    {
        gp_Message("Testing planar embedding with loop edges, non-planarity detection, optimized initialization variant.");

        gp_Free(&G1);
        if ((G1 = gp_DupGraph(G)) == NULL ||
            gp_GetM(G1) != 45 ||
            !_checkForExpectedLoopEdges(G1, 3, 1))
        {
            gp_ErrorMessage("Failed to duplicate graph with loop edges.");
            retVal = NOTOK;
        }
        else if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE ||
                 gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
        {
            gp_ErrorMessage("Embedding with loop edges failed to detect non-planar result, optimized initialization variant.");
            retVal = NOTOK;
        }
        else
        {
            gp_Message("Testing planar embedding with loop edges, non-planarity detection, incremental initialization variant.");

            gp_Free(&G1);
            if ((G1 = gp_DupGraph(G)) == NULL ||
                gp_GetM(G1) != 45 ||
                !_checkForExpectedLoopEdges(G1, 3, 1))
            {
                gp_ErrorMessage("Failed to duplicate graph with loop edges.");
                retVal = NOTOK;
            }
            else if (gp_DepthFirstSearchEx(G1, DFSMODE_UNDIRECTED) != OK)
            {
                gp_ErrorMessage("Undirected DFS failed on graph with loop edges.");
                retVal = NOTOK;
            }
            else if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE ||
                     gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
            {
                gp_ErrorMessage("Embedding with loop edges failed to detect non-planar result, incremental initialization variant.");
                retVal = NOTOK;
            }
        }
    }

    gp_Free(&G);
    gp_Free(&G1);

    return retVal;
}
