/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runParallelEdgeTests(void);

/* Private functions */
static int runManyParallelEdgesTest(void);
static int runSingleParallelEdgeTest(void);
static int runParallelUndirectedTest(void);
static int runDirectedParallelEdgeModeTest(void);
static int runRandomParallelEdgeGraphTests(void);
static int runParallelEdgeTestAllGraphsTests(void);
static int addParallelEdges(graphP theGraph);

/*************************************************************************
 *************************************************************************/

int runParallelEdgeTests(void)
{
    int retVal = OK;

    gp_Message("Starting Parallel Edge Tests\n");

    if (runSingleParallelEdgeTest() != OK)
        retVal = NOTOK;

    if (runManyParallelEdgesTest() != OK)
        retVal = NOTOK;

    if (runParallelUndirectedTest() != OK)
        retVal = NOTOK;

    if (runDirectedParallelEdgeModeTest() != OK)
        retVal = NOTOK;

    if (runRandomParallelEdgeGraphTests() != OK)
        retVal = NOTOK;

    if (runParallelEdgeTestAllGraphsTests() != OK)
        retVal = NOTOK;

    if (retVal == OK)
        gp_Message("Finished Parallel Edge Tests.\n");
    else
        gp_ErrorMessage("A test of parallel edges has failed.\n");

    return retVal;
}

/*************************************************************************
 *************************************************************************/

int runManyParallelEdgesTest(void)
{
    int retVal = OK;
    graphP G = gp_New();
    graphP G1 = NULL, G2 = NULL;

    gp_Message("Testing APIs on graph with many parallel edges.");

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, "Petersen-with-parallel-edges.txt") != OK &&
        gp_Read(G, "c/samples/Petersen-with-parallel-edges.txt") != OK)
    {
        retVal = NOTOK;
    }
    else if (gp_GetM(G) != 60)
        retVal = NOTOK;

    else if (!(gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED))
        retVal = NOTOK;

    else if ((G1 = gp_DupGraph(G)) == NULL)
        retVal = NOTOK;

    if (retVal == OK)
    {
        // The DFS and the embedders accept the parallel edges
        if (gp_DepthFirstSearch(G1) != OK)
            retVal = NOTOK;
        else if ((G2 = gp_DupGraph(G)) == NULL)
            retVal = NOTOK;
        else if (gp_Embed(G2, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE)
            retVal = NOTOK;
        else if (gp_TestEmbedResultIntegrity(G2, G, NONEMBEDDABLE) != NONEMBEDDABLE)
            retVal = NOTOK;
        else
        {
            gp_Free(&G2);
            if ((G2 = gp_DupGraph(G)) == NULL)
                retVal = NOTOK;
            else if (gp_ExtendWith_K33Search(G2) != OK)
                retVal = NOTOK;
            else if (gp_Embed(G2, EMBEDFLAGS_SEARCHFORK33) != NONEMBEDDABLE)
                retVal = NOTOK;
            else if (gp_TestEmbedResultIntegrity(G2, G, NONEMBEDDABLE) != NONEMBEDDABLE)
                retVal = NOTOK;
        }
    }

    if (retVal == OK)
    {
        // The adjacency list format preserves the parallel edges
        char *adjListStr = NULL;

        gp_Free(&G1);
        if (gp_WriteToString(G, &adjListStr, WRITE_ADJLIST) != OK)
            retVal = NOTOK;
        else if ((G1 = gp_New()) == NULL)
            retVal = NOTOK;
        else if (gp_ReadFromString(G1, adjListStr) != OK)
            retVal = NOTOK;
        else if (gp_GetM(G1) != 60)
            retVal = NOTOK;
        else if (gp_DeleteParallelEdges(G1, TRUE) != OK || gp_GetM(G1) != 15)
            retVal = NOTOK;

        if (adjListStr != NULL)
            free(adjListStr);
    }

    if (retVal == OK)
    {
        // Delete parallel edges from a fresh copy and verify exact 15-edge state
        gp_Free(&G1);
        if ((G1 = gp_DupGraph(G)) == NULL)
            retVal = NOTOK;

        else if (gp_DeleteParallelEdges(G1, TRUE) != OK)
            retVal = NOTOK;

        else if (gp_GetM(G1) != 15)
            retVal = NOTOK;

        // The parallel edge deletions leave holes in the edge storage, so
        // we ensure they exist.
        else if (G1->numEdgeHoles == 0)
            retVal = NOTOK;

        // Verify embedding and structural integrity of the simple graph
        // version of G in G1, including tolerating holes in its edge storage
        else if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE)
            retVal = NOTOK;

        else if (gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
            retVal = NOTOK;
    }

    if (retVal != OK)
        gp_ErrorMessage("Failed test of APIs on graph with many parallel edges.");

    gp_Free(&G);
    gp_Free(&G1);
    gp_Free(&G2);

    return retVal;
}

/*************************************************************************
// Grab the Petersen graph, which is a 15-edge graph.
// Ensure vertices 1 and 2 report being degree 3.
// Add one edge known to be parallel, (1, 2).
// Ensure that one edge has shown up and that vertices
//     1 and 2 report being degree 4.
// Ensure a copy of the graph still supports the DFS
//     utilities and is still correctly found nonplanar
// Call the method that deletes the parallel edges
// Ensure that the number of edges goes down by 1.
// Ensure that vertices 1 and 2 report being degree 3.
 *************************************************************************/

int runSingleParallelEdgeTest(void)
{
    int retVal = OK;
    graphP G = gp_New();
    graphP G1 = NULL;

    gp_Message("Testing APIs when adding a single parallel edge.");

    if (G == NULL)
        retVal = NOTOK;

    else if (gp_Read(G, "Petersen.txt") != OK || gp_GetM(G) != 15)
        retVal = NOTOK;

    else if (gp_GetVertexDegree(G, 1) != 3 || gp_GetVertexDegree(G, 1) != 3)
        retVal = NOTOK;

    else if (gp_AddEdge(G, 1, 0, 2, 0) != OK || gp_GetM(G) != 16)
        retVal = NOTOK;

    else if (gp_GetVertexDegree(G, 1) != 4 || gp_GetVertexDegree(G, 1) != 4)
        retVal = NOTOK;

    else if ((G1 = gp_DupGraph(G)) == NULL)
        retVal = NOTOK;

    else if (gp_DepthFirstSearch(G1) != OK || gp_ComputeLowpoints(G1) != OK || gp_ComputeLeastAncestors(G1) != OK)
        retVal = NOTOK;

    else if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE)
        retVal = NOTOK;

    else if (gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
        retVal = NOTOK;

    else if (gp_DeleteParallelEdges(G, TRUE) != OK)
        retVal = NOTOK;

    else if (gp_GetM(G) != 15)
        retVal = NOTOK;

    else if (gp_GetVertexDegree(G, 1) != 3 || gp_GetVertexDegree(G, 1) != 3)
        retVal = NOTOK;

    if (retVal != OK)
        gp_ErrorMessage("Failed test of APIs when adding a single parallel edge.");

    gp_Free(&G);
    gp_Free(&G1);

    return retVal;
}

/*************************************************************************
 *************************************************************************/

int runDirectedParallelEdgeModeTest(void)
{
    int retVal = OK;
    graphP G = gp_New();
    graphP G1 = NULL;
    graphP G2 = NULL;

    gp_Message("Testing modes of deleting parallel directed edges.");

    if (G == NULL)
        retVal = NOTOK;

    else if (gp_EnsureEdgeCapacity(G, 6) != OK || gp_EnsureVertexCapacity(G, 2) != OK)
        retVal = NOTOK;

    else if (gp_AddEdge(G, 1, 0, 2, 0) != OK)
        retVal = NOTOK;
    else
        gp_SetDirection(G, gp_GetFirstEdge(G, 1), EDGEFLAG_DIRECTION_OUTONLY);

    if (retVal == OK)
    {
        if (gp_AddEdge(G, 1, 0, 2, 0) != OK)
            retVal = NOTOK;
        else
            gp_SetDirection(G, gp_GetFirstEdge(G, 1), EDGEFLAG_DIRECTION_OUTONLY);
    }

    if (retVal == OK)
    {
        if (gp_AddEdge(G, 2, 0, 1, 0) != OK)
            retVal = NOTOK;
        else
            gp_SetDirection(G, gp_GetFirstEdge(G, 2), EDGEFLAG_DIRECTION_OUTONLY);
    }

    if (retVal == OK && gp_GetM(G) != 3)
        retVal = NOTOK;

    if (retVal == OK && (G1 = gp_DupGraph(G)) == NULL)
        retVal = NOTOK;

    if (retVal == OK && (G2 = gp_DupGraph(G)) == NULL)
        retVal = NOTOK;

    if (retVal == OK && (gp_DeleteParallelEdges(G1, TRUE) != OK || gp_GetM(G1) != 1))
        retVal = NOTOK;

    if (retVal == OK && (gp_DeleteParallelEdges(G2, FALSE) != OK || gp_GetM(G2) != 2))
        retVal = NOTOK;

    if (retVal != OK)
        gp_ErrorMessage("Failed test of modes of deleting parallel directed edges.");

    gp_Free(&G);
    gp_Free(&G1);
    gp_Free(&G2);

    return retVal;
}

/*************************************************************************
 Ensure that an adjacency list formatted graph with parallel edges doesn't
 set the digraph flag if it is undirected.
 *************************************************************************/
int runParallelUndirectedTest(void)
{
    int retVal = OK;
    graphP G = gp_New();

    gp_Message("Testing digraph flag clear when reading undirected graph with parallel edges.");

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, "Petersen-parallel-undirected.txt") != OK &&
        gp_Read(G, "c/samples/Petersen-parallel-undirected.txt") != OK)
    {
        retVal = NOTOK;
    }
    else if (gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED)
        retVal = NOTOK;

    if (retVal != OK)
        gp_ErrorMessage("Failed digraph flag test when reading undirected graph with parallel edges.");

    gp_Free(&G);

    return retVal;
}

/*************************************************************************
 *************************************************************************/

int runRandomParallelEdgeGraphTests(void)
{
    int retVal = OK;

    gp_Message("Testing random graphs with parallel edges added.");

    if (RandomGraphs("-p", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-p", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-d", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-d", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-o", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-o", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-2", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-2", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-3", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-3", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-4", 500, 16, NULL, TRUE, FALSE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-4", 500, 16, NULL, TRUE, TRUE, addParallelEdges) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    if (retVal != OK)
        gp_ErrorMessage("Failed testing of random graphs with parallel edges added.");

    return retVal;
}

/*************************************************************************
 *************************************************************************/
// FIXME: This will be removed when runParallelEdgeTestAllGraphsTests() is moved from planarityCommandLine.c into the parallel edge test sourcefile, and runTestAllGraphsTest() is moved into a common testUtils.c
extern int runTestAllGraphsTest(char const *commandString, char const *infileName, char const *expectedValidationStr, int (*mutatorFunc)(graphP));

int runParallelEdgeTestAllGraphsTests(void)
{
    int retVal = OK;

    gp_Message("Test planarity-related algorithms with parallel edges.\n");

    if (runTestAllGraphsTest("-p", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("Planarity test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-d", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("Planar graph drawing test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-o", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("Outerplanarity test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-2", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("K2,3 homeomorph search test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-3", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("K3,3 homeomorph search test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-4", "n8.mALL.g6", NULL, addParallelEdges) != OK)
    {
        gp_ErrorMessage("K4 homeomorph search test on all graphs with parallel edges failed.");
        retVal = NOTOK;
    }

    if (retVal != OK)
        gp_ErrorMessage("Failed parallel edges test of a planarity-related algorithm.");

    return retVal;
}

/****************************************************************************
 addParallelEdges()
 Adds three parallel edges to every edge of theGraph. For each vertex v, the
 higher numbered neighbors of v are stacked, and then each is given a second
 edge attached on the link[0] side of v's adjacency list. Then the higher
 numbered neighbors, now including the new edges, are stacked again and each
 is given another edge attached on the link[1] side. The copies of an edge
 are thus interleaved with the copies of the other edges at both endpoints.
 ****************************************************************************/
int addParallelEdges(graphP theGraph)
{
    int origM = gp_GetM(theGraph);
    int *neighborList = NULL;
    int v, e, w, link, neighborCount = 0;

    if ((neighborList = (int *)calloc(4 * origM + 1, sizeof(int))) == NULL)
        return NOTOK;

    for (link = 0; link <= 1; link++)
    {
        for (v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); ++v)
        {
            neighborCount = 0;

            e = gp_GetFirstEdge(theGraph, v);
            while (gp_IsEdge(theGraph, e))
            {
                w = gp_GetNeighbor(theGraph, e);
                if (w > v)
                    neighborList[neighborCount++] = w;
                e = gp_GetNextEdge(theGraph, e);
            }

            for (int i = 0; i < neighborCount; i++)
            {
                w = neighborList[i];
                if (gp_DynamicAddEdge(theGraph, v, link, w, link) != OK)
                {
                    free(neighborList);
                    return NOTOK;
                }
            }
        }
    }

    free(neighborList);
    return gp_GetM(theGraph) == 4 * origM ? OK : NOTOK;
}
