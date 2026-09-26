/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "../planarity.h"
#include "../../graphLib/io/strOrFile.h"

/* Library-private function the edge storage tests call directly */
extern int _CompactEdgeStorage(graphP theGraph);

/* Called from planarityCommandLine.c */
int runSparse6ReadTests(void);
int runSparse6WriteTests(void);
int runSparse6TestAllGraphsTests(void);
char *copySparse6TestString(char const *s6Str);

/* Defined in planarityCommandLine.c */
int runTestAllGraphsTest(char const *commandString, char const *infileName, char const *expectedValidationStr);
int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag);

static int compareSparse6Output(char const *ours, char const *expected, char const *what);
static int runSparse6WriteLineTest(char const *g6Str, char const *expectedLine);
static int runSparse6WriteRoundTripTest(char const *g6FileName, char const *s6FileName, int incremental);
static int runSparse6WriteContractTests(void);
static int runCompactEdgeStorageTests(void);
static int runSparse6LockstepTest(char const *g6FileName, char const *s6FileName, int inputInMemFlag, int expectedNumGraphs);
static int runSparse6AcceptTest(char const *s6Str, char const *expectedG6Line);
static int runSparse6AcceptEdgesTest(char const *s6Str, int order, int const edges[][2], int numEdges);
static int runSparse6RejectTest(char const *s6Str);
static int runSparse6LargeOrderTests(void);

/****************************************************************************
 compareSparse6Output()

 Compares the output of the sparse6 writer, which begins with the header,
 with the expected lines, which may or may not, and reports the first line
 that differs. Returns OK when every line matches, NOTOK otherwise.
 ****************************************************************************/

static int compareSparse6Output(char const *ours, char const *expected, char const *what)
{
    char const *s6Header = ">>sparse6<<";
    char const *p = ours;
    char const *q = expected;
    int lineNum = 1;

    if (ours == NULL || expected == NULL)
    {
        gp_ErrorMessage("No sparse6 output to compare for %s.", what);
        return NOTOK;
    }

    if (strncmp(p, s6Header, strlen(s6Header)) != 0)
    {
        gp_ErrorMessage("The sparse6 output for %s does not begin with the header.", what);
        return NOTOK;
    }
    p += strlen(s6Header);

    if (strncmp(q, s6Header, strlen(s6Header)) == 0)
        q += strlen(s6Header);

    while (*p != '\0' || *q != '\0')
    {
        size_t ourLen = strcspn(p, "\r\n");
        size_t expLen = strcspn(q, "\r\n");

        if (ourLen != expLen || strncmp(p, q, ourLen) != 0)
        {
            gp_ErrorMessage("Line %d of the sparse6 output for %s is \"%.*s\" "
                            "rather than \"%.*s\".",
                            lineNum, what, (int)ourLen, p, (int)expLen, q);
            return NOTOK;
        }

        p += ourLen;
        q += expLen;
        while (*p == '\r' || *p == '\n')
            p++;
        while (*q == '\r' || *q == '\n')
            q++;
        lineNum++;
    }

    return OK;
}

/****************************************************************************
 runSparse6WriteLineTest()

 Reads one graph from the graph6 string and checks that the sparse6 writer
 produces exactly the expected line for it, which is the line nauty writes.
 ****************************************************************************/

static int runSparse6WriteLineTest(char const *g6Str, char const *expectedLine)
{
    int Result = OK;
    graphP theGraph = NULL;
    char *inputStr = copySparse6TestString(g6Str);
    char *outputStr = NULL;
    char expected[MAXLINE + 1];

    if (inputStr == NULL)
        return NOTOK;

    if ((theGraph = gp_New()) == NULL ||
        gp_ReadFromString(theGraph, inputStr) != OK)
    {
        gp_ErrorMessage("Unable to read the graph6 line \"%s\".", g6Str);
        Result = NOTOK;
    }
    else if (gp_WriteToString(theGraph, &outputStr, WRITE_SPARSE6) != OK)
    {
        gp_ErrorMessage("Unable to write the graph of \"%s\" as sparse6.", g6Str);
        Result = NOTOK;
    }
    else
    {
        snprintf(expected, sizeof(expected), "%s\n", expectedLine);
        Result = compareSparse6Output(outputStr, expected, g6Str);
    }

    if (outputStr != NULL)
        free(outputStr);
    gp_Free(&theGraph);
    free(inputStr);

    return Result;
}

/****************************************************************************
 runSparse6WriteRoundTripTest()

 Reads every graph of the graph6 file and writes it with the sparse6 writer,
 as a whole graph per line, or, when incremental is set, as a first whole
 graph followed by the stored symmetric difference from each graph to the
 next, and compares the output with the sparse6 file, which nauty wrote from
 the same graphs, byte for byte. In the incremental case it also checks that
 applying each written batch leaves the writer's graph equal to the graph
 read, with no holes in its edge storage.
 ****************************************************************************/

static int runSparse6WriteRoundTripTest(char const *g6FileName, char const *s6FileName, int incremental)
{
    int Result = OK;
    graphP theGraph = NULL;
    graphP writerGraph = NULL;
    G6ReadIteratorP theReader = NULL;
    S6WriteIteratorP theWriter = NULL;
    char *expected = ReadTextFileIntoString(s6FileName);
    char *outputStr = NULL;
    int numGraphs = 0;

    if (expected == NULL)
    {
        gp_ErrorMessage("Unable to read \"%s\".", s6FileName);
        return NOTOK;
    }

    if ((theGraph = gp_New()) == NULL ||
        g6_NewReader(&theReader, theGraph) != OK ||
        g6_InitReaderWithFileName(theReader, g6FileName) != OK)
    {
        gp_ErrorMessage("Unable to open \"%s\" for the sparse6 write test.", g6FileName);
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theReader) != OK)
        {
            gp_ErrorMessage("Unable to read graph %d of \"%s\".", numGraphs + 1, g6FileName);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theReader))
            break;

        numGraphs++;

        if (!incremental)
        {
            // Each graph is written by its own writer, so the output has one
            // header and one line per graph, compared as we go
            char *lineStr = NULL;

            if (gp_WriteToString(theGraph, &lineStr, WRITE_SPARSE6) != OK)
            {
                gp_ErrorMessage("Unable to write graph %d of \"%s\".", numGraphs, g6FileName);
                Result = NOTOK;
            }
            else
            {
                // Advance the expectation one line for each graph
                size_t expLen = strcspn(expected, "\r\n");
                char expLine[MAXLINE + 1];

                snprintf(expLine, sizeof(expLine), "%.*s\n", (int)expLen, expected);
                Result = compareSparse6Output(lineStr, expLine, g6FileName);
                memmove(expected, expected + expLen, strlen(expected + expLen) + 1);
                while (*expected == '\r' || *expected == '\n')
                    memmove(expected, expected + 1, strlen(expected));
            }

            if (lineStr != NULL)
                free(lineStr);

            continue;
        }

        if (writerGraph == NULL)
        {
            // The first graph is written whole, from a copy the writer owns
            if ((writerGraph = gp_New()) == NULL ||
                gp_EnsureVertexCapacity(writerGraph, gp_GetN(theGraph)) != OK ||
                gp_CopyGraph(writerGraph, theGraph) != OK ||
                s6_NewWriter(&theWriter, writerGraph) != OK ||
                s6_InitWriterWithString(theWriter, &outputStr) != OK ||
                s6_WriteGraph(theWriter) != OK)
            {
                gp_ErrorMessage("Unable to write the first graph of \"%s\".", g6FileName);
                Result = NOTOK;
            }

            continue;
        }

        // Store the symmetric difference: the edges of the writer's graph
        // that the graph read lacks are deletions, the others are additions
        for (int e = gp_LowerBoundEdges(writerGraph); Result == OK && e < gp_UpperBoundEdges(writerGraph); e += 2)
        {
            if (gp_EdgeInUse(writerGraph, e))
            {
                int u = gp_GetNeighbor(writerGraph, gp_GetTwin(writerGraph, e));
                int v = gp_GetNeighbor(writerGraph, e);

                if (!gp_IsEdge(theGraph, gp_FindEdge(theGraph, u, v)) &&
                    s6_StoreGraphChange(theWriter, e, NIL, NIL) != OK)
                {
                    gp_ErrorMessage("Unable to store a deletion for graph %d of \"%s\".", numGraphs, g6FileName);
                    Result = NOTOK;
                }
            }
        }

        for (int e = gp_LowerBoundEdges(theGraph); Result == OK && e < gp_UpperBoundEdges(theGraph); e += 2)
        {
            if (gp_EdgeInUse(theGraph, e))
            {
                int u = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e));
                int v = gp_GetNeighbor(theGraph, e);

                if (!gp_IsEdge(writerGraph, gp_FindEdge(writerGraph, u, v)) &&
                    s6_StoreGraphChange(theWriter, NIL, u, v) != OK)
                {
                    gp_ErrorMessage("Unable to store an addition for graph %d of \"%s\".", numGraphs, g6FileName);
                    Result = NOTOK;
                }
            }
        }

        if (Result == OK && s6_WriteGraph(theWriter) != OK)
        {
            gp_ErrorMessage("Unable to write the changes for graph %d of \"%s\".", numGraphs, g6FileName);
            Result = NOTOK;
        }

        // Applying the batch must leave the writer's graph equal to the graph
        // read, and dense
        if (Result == OK)
        {
            char *ourG6 = NULL, *readG6 = NULL;

            if (gp_WriteToString(writerGraph, &ourG6, WRITE_G6) != OK ||
                gp_WriteToString(theGraph, &readG6, WRITE_G6) != OK ||
                strcmp(ourG6, readG6) != 0)
            {
                gp_ErrorMessage("After graph %d of \"%s\" the writer's graph is \"%s\" "
                                "rather than \"%s\".",
                                numGraphs, g6FileName, ourG6 == NULL ? "" : ourG6,
                                readG6 == NULL ? "" : readG6);
                Result = NOTOK;
            }
            else if (writerGraph->numEdgeHoles != 0)
            {
                gp_ErrorMessage("After graph %d of \"%s\" the writer's graph has "
                                "%d edge holes.",
                                numGraphs, g6FileName, writerGraph->numEdgeHoles);
                Result = NOTOK;
            }

            if (ourG6 != NULL)
                free(ourG6);
            if (readG6 != NULL)
                free(readG6);
        }
    }

    if (incremental)
    {
        // The string is handed over when the writer is freed
        s6_FreeWriter(&theWriter);

        if (Result == OK)
            Result = compareSparse6Output(outputStr, expected, g6FileName);
    }
    else if (Result == OK && strlen(expected) != 0)
    {
        gp_ErrorMessage("\"%s\" has more lines than \"%s\" has graphs.", s6FileName, g6FileName);
        Result = NOTOK;
    }

    if (Result == OK)
        gp_Message("Sparse6 %s write of \"%s\" matched \"%s\" on all %d graphs.",
                   incremental ? "incremental" : "whole-graph", g6FileName, s6FileName, numGraphs);

    if (outputStr != NULL)
        free(outputStr);
    g6_FreeReader(&theReader);
    gp_Free(&theGraph);
    gp_Free(&writerGraph);
    free(expected);

    return Result;
}

/****************************************************************************
 runSparse6WriteContractTests()

 Exercises the refusals of the writer: nothing can be stored before the
 first whole graph is written; a deletion must name an edge in use and an
 addition a pair of real, distinct vertices with no edge between them; a
 pair stored twice in one batch is refused; a batch is refused, and
 discarded, once the graph has been modified directly, after which a whole
 write and then a batch succeed; a graph with hidden edges and a digraph
 are refused; a writer is initialized once. Each refusal must leave the
 writer able to write.
 ****************************************************************************/

static int runSparse6WriteContractTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    graphP theGraph = NULL;
    S6WriteIteratorP theWriter = NULL;
    char *outputStr = NULL;
    int lower = 0;
    int eFirst = NIL;
    // The path 0-1-2-3-4 is written whole, then {0, 1} is deleted and
    // {0, 4} added in one batch, then {3, 4} is deleted directly, which
    // forces a whole write, and then {1, 2} is deleted in a batch; the
    // lines are what copyg -s and copyg -i write for those graphs
    char const *expected =
        ":DaYn\n"
        ";b?\n"
        ":DgYb\n"
        ";g^\n";

    if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 5) != OK)
    {
        gp_Free(&theGraph);
        return NOTOK;
    }

    lower = gp_LowerBoundVertexStorage(theGraph);

    for (int v = 0; v < 4; v++)
    {
        if (gp_AddEdge(theGraph, lower + v, 0, lower + v + 1, 0) != OK)
        {
            gp_Free(&theGraph);
            return NOTOK;
        }
    }

    if (s6_NewWriter(&theWriter, theGraph) != OK ||
        s6_InitWriterWithString(theWriter, &outputStr) != OK)
    {
        s6_FreeWriter(&theWriter);
        if (outputStr != NULL)
            free(outputStr);
        gp_Free(&theGraph);
        return NOTOK;
    }

    gp_SetQuietMode(QUIETMODE_ALL);

    eFirst = gp_FindEdge(theGraph, lower, lower + 1);

    // Before the first whole graph, nothing can be stored
    if (s6_StoreGraphChange(theWriter, eFirst, NIL, NIL) == OK ||
        s6_StoreGraphChange(theWriter, NIL, lower, lower + 4) == OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("A change was stored before the first graph was written.");
        Result = NOTOK;
    }

    // A second initialization is refused
    if (Result == OK)
    {
        char *secondStr = NULL;

        if (s6_InitWriterWithString(theWriter, &secondStr) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("The sparse6 writer was initialized twice.");
            Result = NOTOK;
        }
    }

    if (Result == OK && s6_WriteGraph(theWriter) != OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Unable to write the path graph whole.");
        Result = NOTOK;
    }

    // Invalid changes: an edge not in use, an edge already present, a loop, a
    // vertex out of range, and neither an edge nor a pair
    if (Result == OK &&
        (s6_StoreGraphChange(theWriter, gp_UpperBoundEdges(theGraph), NIL, NIL) == OK ||
         s6_StoreGraphChange(theWriter, NIL, lower, lower + 1) == OK ||
         s6_StoreGraphChange(theWriter, NIL, lower + 2, lower + 2) == OK ||
         s6_StoreGraphChange(theWriter, NIL, lower, lower + 5) == OK ||
         s6_StoreGraphChange(theWriter, eFirst, lower, lower + 4) == OK ||
         s6_StoreGraphChange(theWriter, NIL, NIL, NIL) == OK))
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("An invalid change was stored.");
        Result = NOTOK;
    }

    // A valid batch: delete {0, 1}, add {0, 4}; the same pair twice is refused
    if (Result == OK &&
        (s6_StoreGraphChange(theWriter, eFirst, NIL, NIL) != OK ||
         s6_StoreGraphChange(theWriter, NIL, lower, lower + 4) != OK))
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Unable to store a valid batch.");
        Result = NOTOK;
    }

    if (Result == OK &&
        (s6_StoreGraphChange(theWriter, eFirst, NIL, NIL) == OK ||
         s6_StoreGraphChange(theWriter, NIL, lower + 4, lower) == OK))
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("A pair was stored twice in one batch.");
        Result = NOTOK;
    }

    if (Result == OK && s6_WriteGraph(theWriter) != OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Unable to write the batch.");
        Result = NOTOK;
    }

    // The batch was applied: {0, 1} is gone and {0, 4} is there, densely
    if (Result == OK &&
        (gp_IsEdge(theGraph, gp_FindEdge(theGraph, lower, lower + 1)) ||
         !gp_IsEdge(theGraph, gp_FindEdge(theGraph, lower, lower + 4)) ||
         gp_GetM(theGraph) != 4 || theGraph->numEdgeHoles != 0))
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("The batch was not applied to the graph as written.");
        Result = NOTOK;
    }

    // A direct edit after storing a change: the batch is refused and
    // discarded, a whole write is accepted, and a batch is accepted again
    if (Result == OK)
    {
        int eToDelete = gp_FindEdge(theGraph, lower + 3, lower + 4);

        if (s6_StoreGraphChange(theWriter, eToDelete, NIL, NIL) != OK ||
            gp_DeleteEdge(theGraph, eToDelete) != OK ||
            _CompactEdgeStorage(theGraph) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to set up the direct edit case.");
            Result = NOTOK;
        }
        else if (s6_WriteGraph(theWriter) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("A batch was written after a direct edit of the graph.");
            Result = NOTOK;
        }
        else if (s6_WriteGraph(theWriter) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to write the graph whole after a direct edit.");
            Result = NOTOK;
        }
        else if (s6_StoreGraphChange(theWriter, gp_FindEdge(theGraph, lower + 1, lower + 2), NIL, NIL) != OK ||
                 s6_WriteGraph(theWriter) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to write a batch after the whole write.");
            Result = NOTOK;
        }
    }

    // Hidden edges are refused, whole and batch alike, until restored
    if (Result == OK)
    {
        int eHidden = gp_FindEdge(theGraph, lower + 2, lower + 3);

        gp_HideEdge(theGraph, eHidden);

        if (s6_WriteGraph(theWriter) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("A graph with a hidden edge was written.");
            Result = NOTOK;
        }

        gp_RestoreEdge(theGraph, eHidden);
    }

    // A digraph is refused
    if (Result == OK)
    {
        theGraph->graphFlags |= GRAPHFLAGS_DIRECTEDEDGEDETECTED;

        if (s6_WriteGraph(theWriter) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("A digraph was written as sparse6.");
            Result = NOTOK;
        }

        theGraph->graphFlags &= ~GRAPHFLAGS_DIRECTEDEDGEDETECTED;
    }

    gp_SetQuietMode(origQuietMode);

    // The refusals left the output intact: the string is handed over on free
    s6_FreeWriter(&theWriter);

    if (Result == OK)
        Result = compareSparse6Output(outputStr, expected, "the contract cases");

    if (outputStr != NULL)
        free(outputStr);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runCompactEdgeStorageTests()

 Deletes edges of K5 so that holes lie in the middle and at the end of the
 edge storage, recorded by either record of their pairs, and checks that
 _CompactEdgeStorage() removes every hole while keeping the edge set, the
 edge count and the flags of the moved edge.
 ****************************************************************************/

static int runCompactEdgeStorageTests(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    char *before = NULL, *after = NULL;
    int lower = 0;
    int eLast = NIL, uLast = NIL, vLast = NIL;
    unsigned origQuietMode = gp_GetQuietMode();

    if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 5) != OK)
    {
        gp_Free(&theGraph);
        return NOTOK;
    }

    lower = gp_LowerBoundVertexStorage(theGraph);

    for (int u = 0; Result == OK && u < 5; u++)
        for (int v = u + 1; Result == OK && v < 5; v++)
            if (gp_AddEdge(theGraph, lower + u, 0, lower + v, 0) != OK)
                Result = NOTOK;

    // Delete the last pair, then a middle pair by its odd record, then the
    // pair before the last, so that the trailing region is holes
    if (Result == OK &&
        (gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 3, lower + 4)) != OK ||
         gp_DeleteEdge(theGraph, gp_GetTwin(theGraph, gp_FindEdge(theGraph, lower + 1, lower + 2))) != OK ||
         gp_DeleteEdge(theGraph, gp_GetTwin(theGraph, gp_FindEdge(theGraph, lower + 2, lower + 4))) != OK ||
         gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 0, lower + 3)) != OK))
        Result = NOTOK;

    if (Result == OK && theGraph->numEdgeHoles == 0)
    {
        gp_ErrorMessage("The deletions left no edge holes to compact.");
        Result = NOTOK;
    }

    // The edge set before compaction, taken while the graph is undirected,
    // since setting a direction below marks it as a digraph for graph6
    if (Result == OK && gp_WriteToString(theGraph, &before, WRITE_G6) != OK)
    {
        gp_ErrorMessage("Unable to write the graph before compaction.");
        Result = NOTOK;
    }

    // Flag the last pair in use, so that its move can be checked
    if (Result == OK)
    {
        eLast = gp_UpperBoundEdges(theGraph) - 2;
        while (eLast >= gp_LowerBoundEdges(theGraph) && gp_EdgeNotInUse(theGraph, eLast))
            eLast -= 2;
        uLast = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, eLast));
        vLast = gp_GetNeighbor(theGraph, eLast);
        gp_SetDirection(theGraph, eLast, EDGEFLAG_DIRECTION_OUTONLY);
    }

    if (Result == OK && _CompactEdgeStorage(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to compact the edge storage.");
        Result = NOTOK;
    }

    if (Result == OK && (theGraph->numEdgeHoles != 0 || gp_GetM(theGraph) != 6 ||
                         gp_UpperBoundEdges(theGraph) != gp_LowerBoundEdges(theGraph) + 12))
    {
        gp_ErrorMessage("Compaction left %d holes, %d edges and an upper bound of %d.",
                        theGraph->numEdgeHoles, gp_GetM(theGraph), gp_UpperBoundEdges(theGraph));
        Result = NOTOK;
    }

    if (Result == OK)
    {
        int eMoved = gp_FindEdge(theGraph, uLast, vLast);

        if (!gp_IsEdge(theGraph, eMoved) || gp_GetDirection(theGraph, eMoved) != EDGEFLAG_DIRECTION_OUTONLY)
        {
            gp_ErrorMessage("Compaction lost the direction of the moved edge.");
            Result = NOTOK;
        }
        else if (gp_ClearEdgeDirectionFlags(theGraph) != OK)
            Result = NOTOK;
    }

    // The edge set after compaction, once the graph is undirected again
    if (Result == OK && gp_WriteToString(theGraph, &after, WRITE_G6) != OK)
    {
        gp_ErrorMessage("Unable to write the graph after compaction.");
        Result = NOTOK;
    }

    if (Result == OK && strcmp(before, after) != 0)
    {
        gp_ErrorMessage("Compaction changed the graph from %s to %s.", before, after);
        Result = NOTOK;
    }

    // Compacting a dense graph, and a NULL graph, behave as documented
    if (Result == OK && (_CompactEdgeStorage(theGraph) != OK))
    {
        gp_ErrorMessage("Compaction of a dense graph gave the wrong result.");
        Result = NOTOK;
    }

    gp_SetQuietMode(QUIETMODE_ALL);
    if (Result == OK && (_CompactEdgeStorage(NULL) == OK))
    {
        gp_ErrorMessage("Compaction of a NULL graph gave the wrong result.");
        Result = NOTOK;
    }
    gp_SetQuietMode(origQuietMode);

    if (before != NULL)
        free(before);
    if (after != NULL)
        free(after);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runSparse6WriteTests()

 The writer must produce the bytes nauty produces: single lines with the
 padding cases the specification singles out, then every graph of order 5
 against nauty's own files in both modes, then the contract of the change
 batch, the edge storage compaction it relies on, and the -s transformation
 of the command line.
 ****************************************************************************/

int runSparse6WriteTests(void)
{
    int Result = OK;
    size_t i = 0;

    // {graph6 line, sparse6 line nauty writes for the same graph}
    char const *lineCases[][2] = {
        // The example in the format specification
        {"Fw??G\n", ":Fa@x^"},
        // Orders 2 and 4 with and without edges, including the graphs whose
        // padding must begin with a 0 bit, since n is a power of two, the
        // last edge ends at vertex n-2 and at least k+1 bits are padded
        {"A?\n", ":A"},
        {"A_\n", ":An"},
        {"C?\n", ":C"},
        {"CC\n", ":Cw"},
        {"CW\n", ":CoJ"},
        {"CU\n", ":Co`"},
        {"C~\n", ":CcKI"},
        // The same padding case at order 16, with five bits to pad
        {"Oo??????????????W????\n", ":O`Bo?n"},
        // Order 5, edgeless and the path
        {"D??\n", ":D"},
        {"DQo\n", ":DgH_^"},
        // Edges that jump over several vertices
        {"DAG\n", ":DkY"},
    };

    // Order 63 uses the four-byte order encoding; the line is copyg's
    int const order63Edges[][2] = {{25, 62}, {49, 51}};
    char const *order63Line = ":~??~xk^pf";

    gp_Message("Start sparse6 write tests");

    for (i = 0; Result == OK && i < (sizeof(lineCases) / sizeof(lineCases[0])); i++)
        Result = runSparse6WriteLineTest(lineCases[i][0], lineCases[i][1]);

    // A graph of order 1 has no graph6 reader path, so it is built directly
    if (Result == OK)
    {
        graphP theGraph = gp_New();
        char *outputStr = NULL;

        if (theGraph == NULL || gp_EnsureVertexCapacity(theGraph, 1) != OK ||
            gp_WriteToString(theGraph, &outputStr, WRITE_SPARSE6) != OK)
        {
            gp_ErrorMessage("Unable to write the graph of order 1 as sparse6.");
            Result = NOTOK;
        }
        else
            Result = compareSparse6Output(outputStr, ":@\n", "the graph of order 1");

        if (outputStr != NULL)
            free(outputStr);
        gp_Free(&theGraph);
    }

    if (Result == OK)
    {
        graphP theGraph = gp_New();
        char *outputStr = NULL;
        int lower = 0;
        char expected[MAXLINE + 1];

        if (theGraph == NULL || gp_EnsureVertexCapacity(theGraph, 63) != OK)
            Result = NOTOK;
        else
        {
            lower = gp_LowerBoundVertexStorage(theGraph);

            for (i = 0; Result == OK && i < (sizeof(order63Edges) / sizeof(order63Edges[0])); i++)
                if (gp_AddEdge(theGraph, lower + order63Edges[i][0], 0, lower + order63Edges[i][1], 0) != OK)
                    Result = NOTOK;
        }

        if (Result == OK && gp_WriteToString(theGraph, &outputStr, WRITE_SPARSE6) != OK)
        {
            gp_ErrorMessage("Unable to write the graph of order 63 as sparse6.");
            Result = NOTOK;
        }

        if (Result == OK)
        {
            snprintf(expected, sizeof(expected), "%s\n", order63Line);
            Result = compareSparse6Output(outputStr, expected, "the graph of order 63");
        }

        if (outputStr != NULL)
            free(outputStr);
        gp_Free(&theGraph);
    }

    if (Result == OK)
        Result = runSparse6WriteRoundTripTest("N5-all.g6", "N5-all.s6", FALSE);

    if (Result == OK)
        Result = runSparse6WriteRoundTripTest("N5-all.g6", "N5-all.inc.s6", TRUE);

    if (Result == OK)
        Result = runSparse6WriteContractTests();

    if (Result == OK)
        Result = runCompactEdgeStorageTests();

    if (Result == OK)
        Result = runSparse6LargeOrderTests();

    // The -s transformation, from a file and from a string, of graph6 and of
    // sparse6 input, against the expected output files
    if (Result == OK && runGraphTransformationTest("-s", "nauty_example.g6", TRUE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-s", "nauty_example.g6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-s", "N5-all.g6", TRUE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-s", "N5-all.g6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-s", "K10.g6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-s", "nauty_example.s6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Sparse6 write tests succeeded.");
    else
        gp_ErrorMessage("Sparse6 write tests failed.");

    return Result;
}

int runSparse6ReadTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    size_t i = 0;

    // {sparse6 input, graph6 encoding of the last graph in it}
    char const *acceptCases[][2] = {
        // The example in the format specification
        {":Fa@x^\n", "Fw??G"},
        // No line terminator, and header with CRLF
        {":Fa@x^", "Fw??G"},
        {">>sparse6<<:Fa@x^\r\n", "Fw??G"},
        // An incomplete final pair is padding
        {":Dw\n", "D??"},
        // Orders 2 and 4, where the padding rule for n = 2^k applies
        {":A\n", "A?"},
        {":An\n", "A_"},
        {":C\n", "C?"},
        {":Cw\n", "CC"},
        {":CwN\n", "CE"},
        {":CwI\n", "CF"},
        {":Con\n", "CQ"},
        {":Co`\n", "CU"},
        {":Coa\n", "CT"},
        {":Co`V\n", "CV"},
        {":CoKN\n", "C]"},
        {":CoKI\n", "C^"},
        {":CcKI\n", "C~"},
        // Incremental lines toggle edges relative to the previous graph
        {":D\n;oN\n", "D?_"},
        {":D\n;oN\n;oN\n", "D??"},
        {":D\n;oN\n:D\n", "D??"},
    };

    // Order 63 uses the four-byte order encoding
    int const order63Edges[][2] = {{25, 62}, {49, 51}};

    char const *rejectCases[] = {
        // An incremental line cannot be the first graph
        ";oN\n",
        // Loop edge, including on the single vertex of an order-1 graph
        // (through the zero-width x field), then parallel edges, the second
        // separated from the first occurrence by another edge
        ":AF\n",
        ":@?\n",
        ":Ab\n",
        ":CWG\n",
        // Bytes outside 63..126 in the edge list. The second and third are
        // chosen so that reading them as data would decode to a valid graph,
        // so they are rejected by the byte range check alone, and the last
        // confirms that byte 0xFF is read as data rather than as EOF
        ":A \n",
        ":D!\n",
        ":D\t\n",
        ":D\xFF\n",
        // Order 0, in the one-byte and in the eight-byte order encoding
        ":?\n",
        ":~~????????\n",
        // Eight-byte orders no graph can have: 2^36 - 1, 2^31, and 2^32 + 5,
        // which would become order 5 if cut to an int, all refused by the
        // reader, then INT_MAX and 178956971, which fit an int but exceed the
        // vertex capacity of a graph
        ":~~~~~~~~\n",
        ":~~A?????\n",
        ":~~C????D\n",
        ":~~@~~~~~\n",
        ":~~?Iiiij\n",
        // A byte out of range (a space) in an eight-byte order, and an order
        // cut short
        ":~~?? ???\n",
        ":~~???\n",
        // A later graph of a different order, an empty line, and a line
        // beginning with neither ':' nor ';'
        ":D\n:C\n",
        ":D\n\n:D\n",
        ":D\nX\n",
        // digraph6, and a header followed by a line terminator
        "&D\n",
        ">>sparse6<<\n:D\n",
    };

    gp_Message("Start sparse6 read tests");

    if (Result == OK && runSparse6LockstepTest("N5-all.g6", "N5-all.s6", FALSE, 34) != OK)
        Result = NOTOK;

    if (Result == OK && runSparse6LockstepTest("N5-all.g6", "N5-all.s6", TRUE, 34) != OK)
        Result = NOTOK;

    if (Result == OK && runSparse6LockstepTest("N5-all.g6", "N5-all.inc.s6", FALSE, 34) != OK)
        Result = NOTOK;

    if (Result == OK && runSparse6LockstepTest("N5-all.g6", "N5-all.inc.s6", TRUE, 34) != OK)
        Result = NOTOK;

    // The single-graph readers gp_Read() and gp_ReadFromString() dispatch
    // sparse6 input to the sparse6 reader
    if (Result == OK && runGraphTransformationTest("-a", "nauty_example.s6", TRUE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-a", "nauty_example.s6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-m", "nauty_example.s6", TRUE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-m", "nauty_example.s6", FALSE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-g", "nauty_example.s6", TRUE) != OK)
        Result = NOTOK;

    if (Result == OK && runGraphTransformationTest("-g", "nauty_example.s6", FALSE) != OK)
        Result = NOTOK;

    for (i = 0; Result == OK && i < sizeof(acceptCases) / sizeof(acceptCases[0]); i++)
    {
        if (runSparse6AcceptTest(acceptCases[i][0], acceptCases[i][1]) != OK)
            Result = NOTOK;
    }

    if (Result == OK && runSparse6AcceptEdgesTest(":~??~xk^pf\n", 63, order63Edges, 2) != OK)
        Result = NOTOK;

    // Order 1, whose vertex field is zero bits wide, is checked directly
    // because the graph6 writer used by the other cases does not encode
    // graphs of order 1. The second input has six one-bit pairs that are
    // all padding, which is the only way through that zero-width field
    // that does not end in a loop
    for (i = 0; Result == OK && i < 2; i++)
    {
        char const *order1Cases[] = {":@\n", ":@~\n"};
        graphP order1Graph = gp_New();
        char *order1Str = copySparse6TestString(order1Cases[i]);

        if (order1Graph == NULL || order1Str == NULL ||
            gp_ReadFromString(order1Graph, order1Str) != OK ||
            gp_GetN(order1Graph) != 1 || gp_GetM(order1Graph) != 0)
        {
            gp_ErrorMessage("Sparse6 input \"%s\" did not decode to the "
                            "graph of order 1 with no edges.",
                            order1Cases[i]);
            Result = NOTOK;
        }

        gp_Free(&order1Graph);
        if (order1Str != NULL)
            free(order1Str);
    }

    // The rejected inputs produce error messages by design, and DEBUG builds
    // report every NOTOK as well, so all output is silenced while the
    // rejections are checked
    gp_SetQuietMode(QUIETMODE_ALL);

    for (i = 0; Result == OK && i < sizeof(rejectCases) / sizeof(rejectCases[0]); i++)
    {
        if (runSparse6RejectTest(rejectCases[i]) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Sparse6 reject case %d was accepted but should "
                            "have been rejected.",
                            (int)i);
            Result = NOTOK;
        }
    }

    // A reader whose initialization failed must accept a fresh
    // initialization, and (under a leak checker) must not have kept the
    // rejected input
    if (Result == OK)
    {
        graphP theGraph = gp_New();
        S6ReadIteratorP theS6ReadIterator = NULL;
        char *badStr = copySparse6TestString(":?\n");
        char *goodStr = copySparse6TestString(":D\n");

        if (theGraph == NULL || badStr == NULL || goodStr == NULL ||
            s6_NewReader((&theS6ReadIterator), theGraph) != OK ||
            s6_InitReaderWithString(theS6ReadIterator, badStr) != NOTOK ||
            s6_InitReaderWithString(theS6ReadIterator, goodStr) != OK ||
            s6_ReadGraph(theS6ReadIterator) != OK ||
            gp_GetN(theGraph) != 5 || gp_GetM(theGraph) != 0)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Sparse6 reader could not be reinitialized after "
                            "a failed initialization.");
            Result = NOTOK;
        }

        s6_FreeReader((&theS6ReadIterator));
        gp_Free(&theGraph);
        if (badStr != NULL)
            free(badStr);
        if (goodStr != NULL)
            free(goodStr);
    }

    gp_SetQuietMode(origQuietMode);

    if (Result == OK)
        gp_Message("Sparse6 read tests succeeded.\n");
    else
        gp_ErrorMessage("Sparse6 read tests failed.");

    return Result;
}

char *copySparse6TestString(char const *s6Str)
{
    size_t len = strlen(s6Str);
    char *theCopy = (char *)malloc(len + 1);

    if (theCopy != NULL)
        memcpy(theCopy, s6Str, len + 1);

    return theCopy;
}

// Reads g6FileName with the graph6 read iterator and s6FileName with the
// sparse6 read iterator in lockstep, and requires every pair of graphs to
// have the same graph6 encoding, both inputs to end on the same graph, and
// the number of graphs to be expectedNumGraphs.
static int runSparse6LockstepTest(char const *g6FileName, char const *s6FileName, int inputInMemFlag, int expectedNumGraphs)
{
    int Result = OK;
    int numGraphs = 0;
    graphP g6Graph = NULL, s6Graph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    S6ReadIteratorP theS6ReadIterator = NULL;
    char *s6InputStr = NULL;

    if ((g6Graph = gp_New()) == NULL || (s6Graph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK &&
        (g6_NewReader((&theG6ReadIterator), g6Graph) != OK ||
         g6_InitReaderWithFileName(theG6ReadIterator, g6FileName) != OK))
        Result = NOTOK;

    if (Result == OK && s6_NewReader((&theS6ReadIterator), s6Graph) != OK)
        Result = NOTOK;

    if (Result == OK)
    {
        if (inputInMemFlag)
        {
            if ((s6InputStr = ReadTextFileIntoString(s6FileName)) == NULL ||
                s6_InitReaderWithString(theS6ReadIterator, s6InputStr) != OK)
                Result = NOTOK;
        }
        else if (s6_InitReaderWithFileName(theS6ReadIterator, s6FileName) != OK)
            Result = NOTOK;
    }

    while (Result == OK)
    {
        char *g6Str = NULL, *s6Str = NULL;

        if (g6_ReadGraph(theG6ReadIterator) != OK || s6_ReadGraph(theS6ReadIterator) != OK)
        {
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator) || s6_EndReached(theS6ReadIterator))
        {
            if (!g6_EndReached(theG6ReadIterator) || !s6_EndReached(theS6ReadIterator))
            {
                gp_ErrorMessage("\"%s\" and \"%s\" do not contain the same "
                                "number of graphs.",
                                g6FileName, s6FileName);
                Result = NOTOK;
            }
            break;
        }

        numGraphs++;

        // The edge count is compared as well because a repeated edge would
        // set the same bit of the graph6 encoding twice and go unnoticed,
        // and the edge storage must be dense (no holes left by incremental
        // deletions), which DrawPlanar requires of any graph it embeds
        if (gp_GetM(g6Graph) != gp_GetM(s6Graph) ||
            gp_UpperBoundEdges(s6Graph) != gp_LowerBoundEdges(s6Graph) + (gp_GetM(s6Graph) << 1) ||
            gp_WriteToString(g6Graph, &g6Str, WRITE_G6) != OK || g6Str == NULL ||
            gp_WriteToString(s6Graph, &s6Str, WRITE_G6) != OK || s6Str == NULL ||
            strcmp(g6Str, s6Str) != 0)
        {
            gp_ErrorMessage("Graph %d of \"%s\" does not match graph %d of "
                            "\"%s\".",
                            numGraphs, s6FileName, numGraphs, g6FileName);
            Result = NOTOK;
        }

        if (g6Str != NULL)
            free(g6Str);
        if (s6Str != NULL)
            free(s6Str);
    }

    if (Result == OK && numGraphs != expectedNumGraphs)
    {
        gp_ErrorMessage("Expected %d graphs in \"%s\" but read %d.",
                        expectedNumGraphs, s6FileName, numGraphs);
        Result = NOTOK;
    }

    if (Result == OK)
        gp_Message("The %d graphs in \"%s\" (read %s) match \"%s\".",
                   numGraphs, s6FileName, inputInMemFlag ? "from a string" : "from the file", g6FileName);

    g6_FreeReader((&theG6ReadIterator));
    s6_FreeReader((&theS6ReadIterator));
    gp_Free(&g6Graph);
    gp_Free(&s6Graph);

    if (s6InputStr != NULL)
        free(s6InputStr);

    return Result;
}

// Reads every graph in s6Str with the sparse6 read iterator and requires the
// last one to have the graph6 encoding expectedG6Line (given without the
// header and line terminator).
static int runSparse6AcceptTest(char const *s6Str, char const *expectedG6Line)
{
    int Result = OK;
    int numGraphs = 0;
    char const *g6Header = ">>graph6<<";
    char *s6Copy = NULL, *actualG6 = NULL, *expectedG6 = NULL;
    graphP theGraph = NULL;
    S6ReadIteratorP theS6ReadIterator = NULL;

    if ((s6Copy = copySparse6TestString(s6Str)) == NULL || (theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK &&
        (s6_NewReader((&theS6ReadIterator), theGraph) != OK ||
         s6_InitReaderWithString(theS6ReadIterator, s6Copy) != OK))
        Result = NOTOK;

    while (Result == OK)
    {
        if (s6_ReadGraph(theS6ReadIterator) != OK)
        {
            Result = NOTOK;
            break;
        }

        if (s6_EndReached(theS6ReadIterator))
            break;

        numGraphs++;
    }

    if (Result == OK && numGraphs == 0)
        Result = NOTOK;

    if (Result == OK)
    {
        expectedG6 = (char *)malloc(strlen(g6Header) + strlen(expectedG6Line) + 2);

        if (expectedG6 == NULL)
            Result = NOTOK;
        else
            sprintf(expectedG6, "%s%s\n", g6Header, expectedG6Line);
    }

    if (Result == OK &&
        (gp_WriteToString(theGraph, &actualG6, WRITE_G6) != OK || actualG6 == NULL ||
         strcmp(actualG6, expectedG6) != 0))
        Result = NOTOK;

    if (Result != OK)
        gp_ErrorMessage("Sparse6 input \"%s\" did not decode to the graph "
                        "with graph6 encoding \"%s\".",
                        s6Str, expectedG6Line);

    s6_FreeReader((&theS6ReadIterator));
    gp_Free(&theGraph);

    if (s6Copy != NULL)
        free(s6Copy);
    if (actualG6 != NULL)
        free(actualG6);
    if (expectedG6 != NULL)
        free(expectedG6);

    return Result;
}

// Reads the graph in s6Str with gp_ReadFromString() and requires it to have
// the same graph6 encoding as the graph of the given order built from the
// given 0-based edge list.
static int runSparse6AcceptEdgesTest(char const *s6Str, int order, int const edges[][2], int numEdges)
{
    int Result = OK;
    char *s6Copy = NULL, *actualG6 = NULL, *expectedG6 = NULL;
    graphP actualGraph = NULL, expectedGraph = NULL;

    if ((s6Copy = copySparse6TestString(s6Str)) == NULL ||
        (actualGraph = gp_New()) == NULL ||
        (expectedGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_EnsureVertexCapacity(expectedGraph, order) != OK)
        Result = NOTOK;

    for (int i = 0; Result == OK && i < numEdges; i++)
    {
        if (gp_DynamicAddEdge(expectedGraph,
                              edges[i][0] + gp_LowerBoundVertexStorage(expectedGraph), 0,
                              edges[i][1] + gp_LowerBoundVertexStorage(expectedGraph), 0) != OK)
            Result = NOTOK;
    }

    if (Result == OK && gp_ReadFromString(actualGraph, s6Copy) != OK)
        Result = NOTOK;

    if (Result == OK &&
        (gp_WriteToString(actualGraph, &actualG6, WRITE_G6) != OK || actualG6 == NULL ||
         gp_WriteToString(expectedGraph, &expectedG6, WRITE_G6) != OK || expectedG6 == NULL ||
         strcmp(actualG6, expectedG6) != 0))
        Result = NOTOK;

    if (Result != OK)
        gp_ErrorMessage("Sparse6 input \"%s\" did not decode to the expected "
                        "graph of order %d with %d edges.",
                        s6Str, order, numEdges);

    gp_Free(&actualGraph);
    gp_Free(&expectedGraph);

    if (s6Copy != NULL)
        free(s6Copy);
    if (actualG6 != NULL)
        free(actualG6);
    if (expectedG6 != NULL)
        free(expectedG6);

    return Result;
}

// Returns OK if the sparse6 read iterator rejects s6Str, either at
// initialization or while reading one of its graphs, and NOTOK if every
// graph in it is accepted.
static int runSparse6RejectTest(char const *s6Str)
{
    int Result = OK;
    int accepted = FALSE;
    char *s6Copy = NULL;
    graphP theGraph = NULL;
    S6ReadIteratorP theS6ReadIterator = NULL;

    if ((s6Copy = copySparse6TestString(s6Str)) == NULL || (theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && s6_NewReader((&theS6ReadIterator), theGraph) != OK)
        Result = NOTOK;

    if (Result == OK && s6_InitReaderWithString(theS6ReadIterator, s6Copy) == OK)
    {
        accepted = TRUE;

        while (TRUE)
        {
            if (s6_ReadGraph(theS6ReadIterator) != OK)
            {
                accepted = FALSE;
                break;
            }

            if (s6_EndReached(theS6ReadIterator))
                break;
        }
    }

    if (accepted)
        Result = NOTOK;

    s6_FreeReader((&theS6ReadIterator));
    gp_Free(&theGraph);

    if (s6Copy != NULL)
        free(s6Copy);

    return Result;
}

/****************************************************************************
 runSparse6LargeOrderTests()

 Orders at each boundary of the order encoding, then orders beyond 100000,
 which read and write up to the vertex capacity of a graph: the largest
 order in the one-byte encoding and the smallest in the four-byte one, the
 largest in the four-byte encoding and the smallest in the eight-byte one,
 a power of two, and an order that needs 19 bits per vertex. Each line is
 one nauty's copyg reproduces byte for byte, and it must decode to its
 edges and encode back to itself. Last, a path on 300000 vertices makes a
 line of about 750 KB, longer than the buffer the writer sends a line out
 through, which must read back to the same graph and write the same line.
 ****************************************************************************/

static int runSparse6LargeOrderTests(void)
{
    typedef struct
    {
        char const *s6Line;
        int order;
        int numEdges;
        int edges[2][2];
    } largeOrderCase;

    largeOrderCase const cases[] = {
        {":}}_N\n", 62, 1, {{0, 61}, {0, 0}}},
        {":~??~~?N\n", 63, 1, {{0, 62}, {0, 0}}},
        {":~}~~_??^n~fv~n\n", 258047, 2, {{0, 1}, {258045, 258046}}},
        {":~~???~??~^~_??N\n", 258048, 1, {{0, 258047}, {0, 0}}},
        {":~~??@???_??^~~_??^\n", 262144, 2, {{0, 1}, {3, 262142}}},
        {":~~??@HN_qRvo??THN]\n", 300000, 2, {{5, 299999}, {299998, 299999}}},
    };
    int Result = OK;

    for (size_t i = 0; Result == OK && i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        char *s6Copy = copySparse6TestString(cases[i].s6Line);
        char *outputStr = NULL;
        graphP theGraph = gp_New();

        if (s6Copy == NULL || theGraph == NULL ||
            gp_ReadFromString(theGraph, s6Copy) != OK ||
            gp_GetN(theGraph) != cases[i].order ||
            gp_GetM(theGraph) != cases[i].numEdges)
            Result = NOTOK;

        for (int e = 0; Result == OK && e < cases[i].numEdges; e++)
        {
            int u = cases[i].edges[e][0] + gp_LowerBoundVertexStorage(theGraph);
            int v = cases[i].edges[e][1] + gp_LowerBoundVertexStorage(theGraph);

            if (!gp_IsNeighbor(theGraph, u, v))
                Result = NOTOK;
        }

        if (Result == OK &&
            (gp_WriteToString(theGraph, &outputStr, WRITE_SPARSE6) != OK || outputStr == NULL))
            Result = NOTOK;

        if (Result == OK)
            Result = compareSparse6Output(outputStr, cases[i].s6Line, "a graph of large order");

        if (Result != OK)
            gp_ErrorMessage("Sparse6 line \"%s\" of order %d did not decode to "
                            "its edges or did not encode back to itself.",
                            cases[i].s6Line, cases[i].order);

        gp_Free(&theGraph);
        if (s6Copy != NULL)
            free(s6Copy);
        if (outputStr != NULL)
            free(outputStr);
    }

    if (Result == OK)
    {
        int const pathOrder = 300000;
        char *firstLine = NULL, *secondLine = NULL;
        graphP pathGraph = gp_New(), readGraph = gp_New();

        if (pathGraph == NULL || readGraph == NULL ||
            gp_EnsureVertexCapacity(pathGraph, pathOrder) != OK)
            Result = NOTOK;

        for (int v = 0; Result == OK && v < pathOrder - 1; v++)
        {
            if (gp_DynamicAddEdge(pathGraph, v + gp_LowerBoundVertexStorage(pathGraph), 0,
                                  v + 1 + gp_LowerBoundVertexStorage(pathGraph), 0) != OK)
                Result = NOTOK;
        }

        if (Result == OK &&
            (gp_WriteToString(pathGraph, &firstLine, WRITE_SPARSE6) != OK || firstLine == NULL ||
             strlen(firstLine) <= 65536 ||
             gp_ReadFromString(readGraph, firstLine) != OK ||
             gp_GetN(readGraph) != pathOrder || gp_GetM(readGraph) != pathOrder - 1 ||
             !gp_IsNeighbor(readGraph, gp_LowerBoundVertexStorage(readGraph) + 149999,
                            gp_LowerBoundVertexStorage(readGraph) + 150000) ||
             gp_WriteToString(readGraph, &secondLine, WRITE_SPARSE6) != OK || secondLine == NULL ||
             strcmp(firstLine, secondLine) != 0))
        {
            gp_ErrorMessage("A sparse6 line longer than the write buffer did not "
                            "survive a write and read round trip.");
            Result = NOTOK;
        }

        gp_Free(&pathGraph);
        gp_Free(&readGraph);
        if (firstLine != NULL)
            free(firstLine);
        if (secondLine != NULL)
            free(secondLine);
    }

    return Result;
}

/****************************************************************************
 runSparse6TestAllGraphsTests()

 The algorithm tests on all graphs, run from incremental sparse6 and plain
 sparse6 input, must give the results they give from graph6 input.
 ****************************************************************************/

int runSparse6TestAllGraphsTests(void)
{
    int retVal = OK;

    // The graphs of n8.mALL.g6, in incremental sparse6 format, must give the
    // results runTestAllGraphsTests() gets from graph6, which exercises the
    // sparse6 read iterator on every algorithm, including planar graph
    // drawing, which requires edge storage without holes.
    if (runTestAllGraphsTest("-p", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("Planarity test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-d", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("Planar graph drawing test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-o", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("Outerplanarity test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-2", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("K2,3 homeomorph search test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-3", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("K3,3 homeomorph search test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-4", "n8.mALL.inc.s6", NULL) != OK)
    {
        gp_ErrorMessage("K4 homeomorph search test on all graphs in incremental sparse6 failed.");
        retVal = NOTOK;
    }

    // Plain sparse6 input, where every line is a whole graph
    if (runTestAllGraphsTest("-p", "N5-all.s6", "-p 34 33 1 SUCCESS") != OK)
    {
        gp_ErrorMessage("Planarity test on all graphs in sparse6 failed.");
        retVal = NOTOK;
    }

    return retVal;
}
