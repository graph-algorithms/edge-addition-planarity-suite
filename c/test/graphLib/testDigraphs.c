/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Imported functions */

/* Private function declarations (exported within system) */
int runDigraphTests(void);

/* Private functions */
static int testDirectedDFS(void);
static int _CheckUndirectedDFSEdgeTypes(graphP theGraph);

static int testPetersenDigraph(const char *);
static int _testAllEdgesDirected(graphP G);

static int testDigraphTranspose(
    char const *inputFileName,
    char const *expectedOutputFileName,
    int preservedEdgeSource,
    int preservedEdgeTarget);

int runDigraphTests(void)
{
    int retVal = OK;

    gp_Message("Starting Digraph Tests");

    if (testDirectedDFS() != OK)
    {
        gp_ErrorMessage("Directed DFS test failed.");
        retVal = NOTOK;
    }
    else if (testPetersenDigraph(NULL) != OK)
    {
        gp_ErrorMessage("Petersen Digraph test failed.");
        retVal = NOTOK;
    }
    else if (testDigraphTranspose(NULL, NULL, NIL, NIL) != OK)
    {
        gp_ErrorMessage("Digraph transpose test failed.");
        retVal = NOTOK;
    }

    if (retVal == OK)
        gp_Message("Finished Digraph Tests.\n");
    else
        gp_ErrorMessage("Error detected in Digraph Tests.\n");

    return retVal;
}

/****************************************************************************
 testDirectedDFS()
 ****************************************************************************/

int testDirectedDFS(void)
{
    graphP G = gp_New();
    graphP G1 = NULL, G2 = NULL;
    int const expectedDiscoveryTimes[] = {1, 11, 2, 3, 7, 4};
    int const expectedFinishTimes[] = {10, 12, 9, 6, 8, 5};
    char smallDigraph[] = "N=4\n1: 3 4 0\n2: 1 0\n3: 4 0\n4: 3 0\n";
    int lowerVertex, v, e, source, target, Result;
    unsigned expectedType;

    gp_Message("Testing Directed Depth-First Search");

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, "DirectedDFSTest.txt") != OK)
    {
        gp_ErrorMessage("Failed to read directed DFS sample.");
        gp_Free(&G);
        return NOTOK;
    }

    if (gp_DepthFirstSearchEx(G, DFSMODE_DIRECTED) != OK)
    {
        gp_ErrorMessage("Directed DFS returned an unexpected result.");
        gp_Free(&G);
        return NOTOK;
    }

    if (!(gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED_DIRECTED) ||
        (gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED))
    {
        gp_ErrorMessage("Directed DFS graph flags were not set correctly.");
        gp_Free(&G);
        return NOTOK;
    }

    lowerVertex = gp_LowerBoundVertices(G);
    for (v = lowerVertex; v < gp_UpperBoundVertices(G); ++v)
    {
        if (gp_GetIndex(G, v) != expectedDiscoveryTimes[v - lowerVertex] ||
            gp_GetVisitedIndex(G, v) != expectedFinishTimes[v - lowerVertex])
        {
            gp_ErrorMessage("Directed DFS timestamp mismatch at vertex %d: "
                            "expected (%d, %d), found (%d, %d).",
                            v - lowerVertex + 1,
                            expectedDiscoveryTimes[v - lowerVertex],
                            expectedFinishTimes[v - lowerVertex],
                            gp_GetIndex(G, v),
                            gp_GetVisitedIndex(G, v));
            gp_Free(&G);
            return NOTOK;
        }
    }

    for (e = gp_LowerBoundEdges(G); e < gp_UpperBoundEdges(G); ++e)
    {
        if (!gp_EdgeInUse(G, e) ||
            gp_GetDirection(G, e) == EDGEFLAG_DIRECTION_INONLY)
            continue;

        source = gp_GetNeighbor(G, gp_GetTwin(G, e)) - lowerVertex + 1;
        target = gp_GetNeighbor(G, e) - lowerVertex + 1;
        expectedType = EDGE_TYPE_TREE;

        if ((source == 5 && (target == 1 || target == 3)))
            expectedType = EDGE_TYPE_BACK;
        else if (source == 2 && (target == 3 || target == 4))
            expectedType = EDGE_TYPE_CROSS;
        else if (source == 1 && target == 4)
            expectedType = EDGE_TYPE_FORWARD;

        if (gp_GetEdgeType(G, e) != expectedType)
        {
            gp_ErrorMessage("Directed DFS edge type mismatch.");
            gp_Free(&G);
            return NOTOK;
        }
    }

    G1 = gp_DupGraph(G);
    if (G1 == NULL || !(gp_GetGraphFlags(G1) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Graph duplication method failed to preserve directed DFS state.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }

    // The optimized embedding initialization performs its own undirected
    // DFS and must replace the directed DFS state on a duplicated graph.
    if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != OK ||
        (gp_GetGraphFlags(G1) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Embedding did not replace directed DFS state.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }

    if (_CheckUndirectedDFSEdgeTypes(G1) != OK)
    {
        gp_ErrorMessage("Embedding did not replace directed DFS edge types.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }
    gp_Free(&G1);

    // Embedding this digraph after a directed DFS crashed while the stale
    // directed edge types were OR'ed into the new ones rather than replaced.
    G1 = gp_New();
    if (G1 == NULL || gp_ReadFromString(G1, smallDigraph) != OK ||
        (G2 = gp_DupGraph(G1)) == NULL ||
        gp_DepthFirstSearchEx(G1, DFSMODE_DIRECTED) != OK)
    {
        gp_ErrorMessage("Failed to prepare the second directed DFS graph.");
        gp_Free(&G);
        gp_Free(&G1);
        gp_Free(&G2);
        return NOTOK;
    }

    Result = gp_Embed(G1, EMBEDFLAGS_PLANAR);
    if (Result != OK || gp_TestEmbedResultIntegrity(G1, G2, Result) != OK)
    {
        gp_ErrorMessage("Embedding after directed DFS failed or was invalid.");
        gp_Free(&G);
        gp_Free(&G1);
        gp_Free(&G2);
        return NOTOK;
    }
    gp_Free(&G1);
    gp_Free(&G2);

    // The legacy DFS must remain available for directed input and replace
    // directed timestamps and flags with its original undirected DFS state.
    if (gp_DepthFirstSearchEx(G, DFSMODE_UNDIRECTED) != OK ||
        !(gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED) ||
        (gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Undirected DFS did not replace directed DFS state.");
        gp_Free(&G);
        return NOTOK;
    }

    if (_CheckUndirectedDFSEdgeTypes(G) != OK)
    {
        gp_ErrorMessage("Undirected DFS did not replace directed DFS edge types.");
        gp_Free(&G);
        return NOTOK;
    }

    for (v = lowerVertex; v < gp_UpperBoundVertices(G); ++v)
    {
        if (gp_GetVisitedIndex(G, v) != 0)
        {
            gp_ErrorMessage("Undirected DFS did not clear directed finish times.");
            gp_Free(&G);
            return NOTOK;
        }
    }

    gp_Free(&G);
    return OK;
}

/********************************************************************
 _CheckUndirectedDFSEdgeTypes()

 Returns OK if every edge record carries exactly one of the edge types
 assigned by an undirected DFS, which fails if any bits of a prior DFS
 (e.g. EDGE_TYPE_CROSS from a directed DFS) survived in the edge types.
 ********************************************************************/

int _CheckUndirectedDFSEdgeTypes(graphP theGraph)
{
    int e;
    unsigned edgeType;

    for (e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); ++e)
    {
        if (!gp_EdgeInUse(theGraph, e))
            continue;

        edgeType = gp_GetEdgeType(theGraph, e);
        if (edgeType != EDGE_TYPE_CHILD && edgeType != EDGE_TYPE_PARENT &&
            edgeType != EDGE_TYPE_BACK && edgeType != EDGE_TYPE_FORWARD)
            return NOTOK;
    }

    return OK;
}

/****************************************************************************
 testPetersenDigraph()

 Test various digraph features using the Petersen digraph.
 ****************************************************************************/

int testPetersenDigraph(const char *inputFileName)
{
    int retVal = OK;

    if (inputFileName == NULL)
    {
        gp_Message("Testing Digraph Read, Embed, Undirected DFS, Convert to Undirected, and Non-Digraph Function Detection");

        if (testPetersenDigraph("Petersen.digraph.txt") != OK)
        {
            gp_ErrorMessage("Test of Digraphs failed on Petersen.digraph.txt");
            retVal = NOTOK;
        }
        if (testPetersenDigraph("Petersen.digraph.0-based.txt") != OK)
        {
            gp_ErrorMessage("Test of Digraphs failed on Petersen.digraph.0-based.txt");
            retVal = NOTOK;
        }
    }
    else
    {
        graphP G = gp_New();
        graphP G1 = NULL;
        int quietModeCache;
        char *dummyStr = NULL; // Safe throwaway pointer for early-outs

        if (G == NULL)
            return NOTOK;

        if (gp_Read(G, inputFileName) != OK)
        {
            gp_ErrorMessage("Failed to read Petersen digraph sample.");
            gp_Free(&G);
            return NOTOK;
        }

        //  Verify edges and flags
        if (gp_GetM(G) != 15)
        {
            gp_ErrorMessage("Petersen Digraph should have exactly 15 edges.");
            gp_Free(&G);
            return NOTOK;
        }

        // Verify the directed edge flag detected and that all edges are directed
        if (!(gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED))
        {
            gp_ErrorMessage("Directed edge flag was not set upon reading digraph.");
            gp_Free(&G);
            return NOTOK;
        }

        if (!_testAllEdgesDirected(G))
        {
            gp_ErrorMessage("Unexpected undirected edge or edge direction mismatch found.");
            gp_Free(&G);
            return NOTOK;
        }

        // Test digraph duplication
        if ((G1 = gp_DupGraph(G)) == NULL)
        {
            gp_Free(&G);
            return NOTOK;
        }

        if (!gp_HasDirectedEdges(G1))
        {
            gp_ErrorMessage("gp_HasDirectedEdges() returned unexpected FALSE value result.");
            gp_Free(&G);
            gp_Free(&G1);
            return NOTOK;
        }

        if (!_testAllEdgesDirected(G1))
        {
            gp_ErrorMessage("Unexpected undirected edge or edge direction mismatch found.");
            gp_Free(&G);
            gp_Free(&G1);
            return NOTOK;
        }

        // Test that the digraph is embeddable
        if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE)
        {
            gp_ErrorMessage("Call to embed digraph did not return expected result.");
            gp_Free(&G);
            gp_Free(&G1);
            return NOTOK;
        }

        if (gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
        {
            gp_ErrorMessage("Embed integrity check failed.");
            gp_Free(&G);
            gp_Free(&G1);
            return NOTOK;
        }

        gp_Free(&G1);

        // Run early-outs quietly (with a dummy string for write operations)
        quietModeCache = gp_GetQuietMode();
        gp_SetQuietMode(QUIETMODE_ALL);

        if (gp_DepthFirstSearch(G) != OK ||
            gp_ComputeLowpoints(G) == OK ||
            gp_ComputeLeastAncestors(G) == OK ||
            gp_WriteToString(G, &dummyStr, WRITE_GRAPH6) == OK ||
            gp_WriteToString(G, &dummyStr, WRITE_ADJMATRIX) == OK)
        {
            gp_SetQuietMode(quietModeCache);
            gp_ErrorMessage("Digraph early-out test failed.");
            gp_Free(&G);
            if (dummyStr != NULL)
                free(dummyStr);
            return NOTOK;
        }

        gp_SetQuietMode(quietModeCache);

        //  Clear edge direction flags, and verify
        if (gp_ClearEdgeDirectionFlags(G) != OK)
        {
            gp_Free(&G);
            return NOTOK;
        }

        if (gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED)
        {
            gp_ErrorMessage("Directed edge detected flag should be clear but is not clear.");
            gp_Free(&G);
            return NOTOK;
        }

        if (gp_HasDirectedEdges(G))
        {
            gp_ErrorMessage("gp_HasDirectedEdges() returned unexpected TRUE result.");
            gp_Free(&G);
            return NOTOK;
        }

        gp_Free(&G);

        return OK;
    }

    return retVal;
}

/****************************************************************************
 _testAllEdgesDirected()

 Support routine for testPetersenDigraph()
 ****************************************************************************/

int _testAllEdgesDirected(graphP G)
{
    int v = NIL, e = NIL, eTwin = NIL, eDir = 0, eTwinDir = 0;

    if (G == NULL)
    {
        gp_ErrorMessage("Invalid parameters.");
        return FALSE;
    }

    for (v = gp_LowerBoundVertices(G); v < gp_UpperBoundVertices(G); ++v)
    {
        e = gp_GetFirstEdge(G, v);
        while (gp_IsEdge(G, e))
        {
            eTwin = gp_GetTwin(G, e);
            eDir = gp_GetDirection(G, e);
            eTwinDir = gp_GetDirection(G, eTwin);

            if (eDir == 0 || eTwinDir == 0 || eDir == eTwinDir)
                return FALSE;

            e = gp_GetNextEdge(G, e);
        }
    }

    return TRUE;
}

/****************************************************************************
 testDigraphTranspose()
 ****************************************************************************/

int testDigraphTranspose(
    char const *inputFileName,
    char const *expectedOutputFileName,
    int preservedEdgeSource,
    int preservedEdgeTarget)
{
    int retVal = OK;

    if (inputFileName == NULL)
    {
        gp_Message("Testing Digraph Transpose");

        if (testDigraphTranspose("Petersen.digraph.txt", "Digraph.transposeTest.txt", 5, 1) != OK)
        {
            gp_ErrorMessage("Digraph Transpose Test failed on Petersen.digraph.txt");
            retVal = NOTOK;
        }

        if (testDigraphTranspose(
                "Petersen.digraph.0-based.txt",
                "Digraph.transposeTest.0-based.txt", 5, 1) != OK)
        {
            gp_ErrorMessage("Digraph Transpose Test failed on Petersen.digraph.txt");
            retVal = NOTOK;
        }
    }
    else
    {
        graphP G = gp_New();
        char *actualOutput = NULL;
        int e = NIL;

        gp_Message("Testing Digraph Transpose");

        if (G == NULL)
            return NOTOK;

        if (gp_Read(G, inputFileName) != OK)
        {
            gp_ErrorMessage("Failed to read Petersen digraph sample for transpose test.");
            gp_Free(&G);
            return NOTOK;
        }

        // Preserve one undirected edge while transposing all directed edges.
        e = gp_FindDirectedEdge(G, preservedEdgeSource, preservedEdgeTarget, EDGEFLAG_DIRECTION_OUTONLY);
        if (e == NIL)
        {
            gp_ErrorMessage("Failed to find directed edge to preserve during transpose.");
            gp_Free(&G);
            return NOTOK;
        }
        gp_SetDirection(G, e, 0);

        if (gp_TransposeDirectedGraph(G) != OK)
        {
            gp_ErrorMessage("Directed graph transpose returned an unexpected result.");
            gp_Free(&G);
            return NOTOK;
        }

        if (gp_WriteToString(G, &actualOutput, WRITE_ADJLIST) != OK || actualOutput == NULL ||
            TextFileMatchesString(expectedOutputFileName, actualOutput) != TRUE)
        {
            gp_ErrorMessage("Directed graph transpose output did not match the expected sample.");
            gp_Free(&G);
            if (actualOutput != NULL)
                free(actualOutput);
            return NOTOK;
        }

        gp_Free(&G);
        free(actualOutput);
        return OK;
    }

    return retVal;
}
