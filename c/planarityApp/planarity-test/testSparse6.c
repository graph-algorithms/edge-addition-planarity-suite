/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <limits.h>
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
int runSparse6LookaheadTests(void);
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
static int compareSparse6TestPairs(void const *a, void const *b);
static int collectSparse6TestPairs(graphP theGraph, int **pPairs, int *pNumPairs);
static int compareSparse6EdgeMultiset(graphP theGraph, int const pairs[][2], int numPairs);
static int runSparse6AcceptMultigraphTest(char const *s6Str, int order, int const pairs[][2], int numPairs, int expectParallelEdges);
static int runSparse6WriteMultigraphLineTest(int order, int const pairs[][2], int numPairs, char const *expectedLine);
static int runSparse6MultigraphContractTests(void);
static int runSparse6PetersenMultigraphTest(void);

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

    // Multigraphs written whole, each line as nauty's own encoder writes it:
    // a pair twice, the pairs of ":CWG" back in sorted order, a graph of
    // genrang -m3 -r3, and the padding case for n = 2^k
    if (Result == OK)
    {
        int const pairsAb[][2] = {{0, 1}, {0, 1}};
        int const pairsCWG[][2] = {{0, 3}, {1, 3}, {0, 3}};
        int const pairsGenrang[][2] = {{0, 3}, {0, 3}, {2, 3}, {1, 4}, {1, 4}, {2, 4}, {0, 5}, {1, 5}, {2, 5}};
        int const pairsCpJ[][2] = {{1, 2}, {1, 2}};

        if (runSparse6WriteMultigraphLineTest(2, pairsAb, 2, ":Ab") != OK ||
            runSparse6WriteMultigraphLineTest(4, pairsCWG, 3, ":Cw@") != OK ||
            runSparse6WriteMultigraphLineTest(6, pairsGenrang, 9, ":Ek?IPI@J") != OK ||
            runSparse6WriteMultigraphLineTest(4, pairsCpJ, 2, ":CpJ") != OK)
            Result = NOTOK;
    }

    if (Result == OK)
        Result = runSparse6MultigraphContractTests();

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
        gp_Message("Sparse6 write tests succeeded.\n");
    else
        gp_ErrorMessage("Sparse6 write tests failed.\n");

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

    // Parallel edges on ':' lines, checked with their multiplicities: a pair
    // twice; a pair repeated with another edge between, so that the repeat
    // is not next to the first occurrence; a line of nauty's genrang -m3 -r3;
    // the padding case for n = 2^k, whose leading 0 bit must not read as a
    // loop; and a ':' line without parallel edges after one with them, which
    // must leave the parallel edge flag clear
    int const pairsAb[][2] = {{0, 1}, {0, 1}};
    int const pairsCWG[][2] = {{0, 3}, {1, 3}, {0, 3}};
    int const pairsGenrang[][2] = {{0, 3}, {0, 3}, {2, 3}, {1, 4}, {1, 4}, {2, 4}, {0, 5}, {1, 5}, {2, 5}};
    int const pairsCpJ[][2] = {{1, 2}, {1, 2}};
    int const pairsK2[][2] = {{0, 1}};

    char const *rejectCases[] = {
        // An incremental line cannot be the first graph
        ";oN\n",
        // Loop edge, including on the single vertex of an order-1 graph
        // (through the zero-width x field)
        ":AF\n",
        ":@?\n",
        // An incremental line after a graph with parallel edges, even an
        // empty one, since incremental sparse6 does not support them (nauty's
        // copyg takes the previous graph modulo 2 and loses edges)
        ":Ab\n;\n",
        ":Ab\n;n\n",
        ":Ek?IPI@J\n;b\n",
        // and after a pair repeated with another edge between its occurrences
        ":CWG\n;\n",
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
        // An edge twice on an incremental line, which lists each changed
        // edge once (nauty's copyg toggles it twice, but no nauty tool
        // writes such a line, and the lookahead could not report it)
        ":D\n;o?~\n",
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

    if (Result == OK &&
        (runSparse6AcceptMultigraphTest(":Ab\n", 2, pairsAb, 2, TRUE) != OK ||
         runSparse6AcceptMultigraphTest(":CWG\n", 4, pairsCWG, 3, TRUE) != OK ||
         runSparse6AcceptMultigraphTest(":Ek?IPI@J\n", 6, pairsGenrang, 9, TRUE) != OK ||
         runSparse6AcceptMultigraphTest(":CpJ\n", 4, pairsCpJ, 2, TRUE) != OK ||
         runSparse6AcceptMultigraphTest(":Ab\n:An\n", 2, pairsK2, 1, FALSE) != OK))
        Result = NOTOK;

    if (Result == OK && runSparse6PetersenMultigraphTest() != OK)
        Result = NOTOK;

    // The reader goes by the ':' line it read, not by the library's parallel
    // edge flag: a ';' line is applied after a simple graph whose flag is set,
    // as an insertion refused at the edge capacity leaves it, and refused
    // after a graph with parallel edges whose flag is clear
    for (i = 0; Result == OK && i < 2; i++)
    {
        char *flagStr = copySparse6TestString(i == 0 ? ":An\n;n\n" : ":Ab\n;\n");
        graphP flagGraph = gp_New();
        S6ReadIteratorP flagReader = NULL;
        int secondRead = NOTOK;

        if (flagStr == NULL || flagGraph == NULL ||
            s6_NewReader((&flagReader), flagGraph) != OK ||
            s6_InitReaderWithString(flagReader, flagStr) != OK ||
            s6_ReadGraph(flagReader) != OK)
            Result = NOTOK;
        else
        {
            if (i == 0)
                flagGraph->graphFlags |= GRAPHFLAGS_PARALLELEDGEDETECTED;
            else
                flagGraph->graphFlags &= ~GRAPHFLAGS_PARALLELEDGEDETECTED;

            gp_SetQuietMode(QUIETMODE_ALL);
            secondRead = s6_ReadGraph(flagReader);
            gp_SetQuietMode(origQuietMode);

            if ((i == 0) != (secondRead == OK))
            {
                gp_ErrorMessage("A ';' line was %s after a graph whose parallel "
                                "edge flag was %s.",
                                i == 0 ? "refused" : "applied",
                                i == 0 ? "set without parallel edges" : "clear with parallel edges");
                Result = NOTOK;
            }
        }

        s6_FreeReader((&flagReader));
        gp_Free(&flagGraph);
        if (flagStr != NULL)
            free(flagStr);
    }

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

// Orders 0-based pairs (smaller, larger) lexicographically.
static int compareSparse6TestPairs(void const *a, void const *b)
{
    int const *pairA = (int const *)a;
    int const *pairB = (int const *)b;

    if (pairA[0] != pairB[0])
        return (pairA[0] < pairB[0]) ? -1 : 1;

    if (pairA[1] != pairB[1])
        return (pairA[1] < pairB[1]) ? -1 : 1;

    return 0;
}

// Collects the edges of theGraph as sorted 0-based pairs (smaller, larger),
// one pair per edge, so that parallel edges appear as repeated pairs, in an
// array the caller frees. An edgeless graph yields a NULL array.
static int collectSparse6TestPairs(graphP theGraph, int **pPairs, int *pNumPairs)
{
    int numEdges = gp_GetM(theGraph);
    int numPairs = 0;
    int lower = gp_LowerBoundVertexStorage(theGraph);
    int *pairs = NULL;

    (*pPairs) = NULL;
    (*pNumPairs) = 0;

    if (numEdges == 0)
        return OK;

    if ((pairs = (int *)malloc((size_t)numEdges * 2 * sizeof(int))) == NULL)
        return NOTOK;

    for (int e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); e += 2)
    {
        int u = 0, v = 0;

        if (!gp_EdgeInUse(theGraph, e))
            continue;

        if (numPairs == numEdges)
        {
            free(pairs);
            return NOTOK;
        }

        u = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)) - lower;
        v = gp_GetNeighbor(theGraph, e) - lower;
        pairs[2 * numPairs] = (u < v) ? u : v;
        pairs[2 * numPairs + 1] = (u < v) ? v : u;
        numPairs++;
    }

    if (numPairs != numEdges)
    {
        free(pairs);
        return NOTOK;
    }

    qsort(pairs, (size_t)numPairs, 2 * sizeof(int), compareSparse6TestPairs);

    (*pPairs) = pairs;
    (*pNumPairs) = numPairs;

    return OK;
}

// Compares the edges of theGraph with the expected 0-based pairs as
// multisets, since the graph6 encoding the other cases compare cannot show
// how many times a pair occurs. The expected pairs may be in any order.
static int compareSparse6EdgeMultiset(graphP theGraph, int const pairs[][2], int numPairs)
{
    int Result = OK;
    int numActual = 0;
    int *actual = NULL, *expected = NULL;

    if (collectSparse6TestPairs(theGraph, &actual, &numActual) != OK || numActual != numPairs)
        Result = NOTOK;

    if (Result == OK && numPairs > 0)
    {
        if ((expected = (int *)malloc((size_t)numPairs * 2 * sizeof(int))) == NULL)
            Result = NOTOK;

        for (int i = 0; Result == OK && i < numPairs; i++)
        {
            expected[2 * i] = (pairs[i][0] < pairs[i][1]) ? pairs[i][0] : pairs[i][1];
            expected[2 * i + 1] = (pairs[i][0] < pairs[i][1]) ? pairs[i][1] : pairs[i][0];
        }

        if (Result == OK)
        {
            qsort(expected, (size_t)numPairs, 2 * sizeof(int), compareSparse6TestPairs);

            if (memcmp(actual, expected, (size_t)numPairs * 2 * sizeof(int)) != 0)
                Result = NOTOK;
        }
    }

    if (actual != NULL)
        free(actual);
    if (expected != NULL)
        free(expected);

    return Result;
}

// Reads every graph in s6Str with the sparse6 read iterator and requires the
// last one to have the given order and exactly the given pairs, repeated
// pairs being parallel edges, and the parallel edge flag set exactly when
// expectParallelEdges is TRUE.
static int runSparse6AcceptMultigraphTest(char const *s6Str, int order, int const pairs[][2], int numPairs, int expectParallelEdges)
{
    int Result = OK;
    int numGraphs = 0;
    int hasParallelEdges = FALSE;
    char *s6Copy = NULL;
    graphP theGraph = NULL;
    S6ReadIteratorP theS6ReadIterator = NULL;

    if ((s6Copy = copySparse6TestString(s6Str)) == NULL || (theGraph = gp_New()) == NULL ||
        s6_NewReader((&theS6ReadIterator), theGraph) != OK ||
        s6_InitReaderWithString(theS6ReadIterator, s6Copy) != OK)
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

    if (Result == OK)
        hasParallelEdges = (gp_GetGraphFlags(theGraph) & GRAPHFLAGS_PARALLELEDGEDETECTED) ? TRUE : FALSE;

    if (Result == OK &&
        (numGraphs == 0 || gp_GetN(theGraph) != order ||
         compareSparse6EdgeMultiset(theGraph, pairs, numPairs) != OK ||
         hasParallelEdges != expectParallelEdges))
        Result = NOTOK;

    if (Result != OK)
        gp_ErrorMessage("Sparse6 input \"%s\" did not decode to the expected "
                        "graph of order %d with %d edges and the parallel "
                        "edge flag %s.",
                        s6Str, order, numPairs, expectParallelEdges ? "set" : "clear");

    s6_FreeReader((&theS6ReadIterator));
    gp_Free(&theGraph);

    if (s6Copy != NULL)
        free(s6Copy);

    return Result;
}

/****************************************************************************
 runSparse6WriteMultigraphLineTest()

 Builds the multigraph of the given order from the 0-based pairs, which
 repeat for its parallel edges, and checks that the sparse6 writer produces
 exactly the line nauty's own encoder writes for it, and that the line
 reads back to the same pairs with the parallel edge flag set.
 ****************************************************************************/

static int runSparse6WriteMultigraphLineTest(int order, int const pairs[][2], int numPairs, char const *expectedLine)
{
    int Result = OK;
    graphP theGraph = NULL, readBack = NULL;
    char *outputStr = NULL;
    char expected[MAXLINE + 1];

    if ((theGraph = gp_New()) == NULL || (readBack = gp_New()) == NULL ||
        gp_EnsureVertexCapacity(theGraph, order) != OK)
        Result = NOTOK;

    for (int i = 0; Result == OK && i < numPairs; i++)
    {
        if (gp_DynamicAddEdge(theGraph, pairs[i][0] + gp_LowerBoundVertexStorage(theGraph), 0,
                              pairs[i][1] + gp_LowerBoundVertexStorage(theGraph), 0) != OK)
            Result = NOTOK;
    }

    if (Result == OK && gp_WriteToString(theGraph, &outputStr, WRITE_SPARSE6) != OK)
    {
        gp_ErrorMessage("Unable to write a multigraph of order %d as sparse6.", order);
        Result = NOTOK;
    }

    if (Result == OK)
    {
        snprintf(expected, sizeof(expected), "%s\n", expectedLine);
        Result = compareSparse6Output(outputStr, expected, "a multigraph");
    }

    if (Result == OK &&
        (gp_ReadFromString(readBack, outputStr) != OK ||
         !(gp_GetGraphFlags(readBack) & GRAPHFLAGS_PARALLELEDGEDETECTED) ||
         compareSparse6EdgeMultiset(readBack, pairs, numPairs) != OK))
    {
        gp_ErrorMessage("The sparse6 line \"%s\" of a multigraph did not read "
                        "back to the same multigraph.",
                        expectedLine);
        Result = NOTOK;
    }

    if (outputStr != NULL)
        free(outputStr);
    gp_Free(&theGraph);
    gp_Free(&readBack);

    return Result;
}

/****************************************************************************
 runSparse6MultigraphContractTests()

 A multigraph is written whole, and after that line no change can be
 stored, not even a deletion or an addition a simple graph would take,
 since incremental sparse6 does not support parallel edges. The second
 instances are then deleted directly, which leaves the library's parallel
 edge flag set; changes are still refused until the graph is written whole,
 and then accepted, as the writer goes by the line it wrote, not by the
 flag. The lines are what nauty writes for these graphs.
 ****************************************************************************/

static int runSparse6MultigraphContractTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    graphP theGraph = NULL;
    S6WriteIteratorP theWriter = NULL;
    char *outputStr = NULL;
    int lower = 0;
    // The multigraph that nauty's genrang -m3 -r3 writes as the first line,
    // with 0-3 and 1-4 twice each; then the graph without the second
    // instances, and a ';' line adding {0, 1}, as copyg -i writes them
    int const pairs[][2] = {{0, 3}, {0, 3}, {2, 3}, {1, 4}, {1, 4}, {2, 4}, {0, 5}, {1, 5}, {2, 5}};
    char const *expected =
        ":Ek?IPI@J\n"
        ":EkAcgCn\n"
        ";b\n";

    if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 6) != OK)
    {
        gp_Free(&theGraph);
        return NOTOK;
    }

    lower = gp_LowerBoundVertexStorage(theGraph);

    for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++)
    {
        if (gp_DynamicAddEdge(theGraph, lower + pairs[i][0], 0, lower + pairs[i][1], 0) != OK)
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

    if (s6_WriteGraph(theWriter) != OK)
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("Unable to write a multigraph whole.");
        Result = NOTOK;
    }

    // The deletion of an edge with one instance and the addition of an
    // absent pair are both refused while the graph has parallel edges
    if (Result == OK &&
        (s6_StoreGraphChange(theWriter, gp_FindEdge(theGraph, lower + 2, lower + 3), NIL, NIL) == OK ||
         s6_StoreGraphChange(theWriter, NIL, lower, lower + 1) == OK))
    {
        gp_SetQuietMode(origQuietMode);
        gp_ErrorMessage("A change was stored for a graph with parallel edges.");
        Result = NOTOK;
    }

    // The second instances of 0-3 and 1-4 are deleted directly; the flag
    // stays set, and the last line written still has parallel edges
    if (Result == OK)
    {
        if (gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower, lower + 3)) != OK ||
            gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 1, lower + 4)) != OK ||
            !(gp_GetGraphFlags(theGraph) & GRAPHFLAGS_PARALLELEDGEDETECTED))
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to delete the second instances of the "
                            "parallel edges.");
            Result = NOTOK;
        }
        else if (s6_StoreGraphChange(theWriter, NIL, lower, lower + 1) == OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("A change was stored after a line with parallel "
                            "edges.");
            Result = NOTOK;
        }
        else if (s6_WriteGraph(theWriter) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to write the graph whole once its parallel "
                            "edges were deleted.");
            Result = NOTOK;
        }
        else if (s6_StoreGraphChange(theWriter, NIL, lower, lower + 1) != OK ||
                 s6_WriteGraph(theWriter) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Unable to write a batch after a whole line "
                            "without parallel edges, with the parallel edge "
                            "flag still set.");
            Result = NOTOK;
        }
    }

    // A graph with parallel edges whose flag is clear, as a duplicate added
    // after gp_CopyAdjacencyLists() leaves it: the line written has a
    // repeated pair, so a change is refused all the same
    if (Result == OK)
    {
        graphP clearGraph = gp_New();
        S6WriteIteratorP clearWriter = NULL;
        char *clearStr = NULL;
        int clearLower = 0;

        if (clearGraph == NULL || gp_EnsureVertexCapacity(clearGraph, 2) != OK)
            Result = NOTOK;
        else
        {
            clearLower = gp_LowerBoundVertexStorage(clearGraph);

            if (gp_AddEdge(clearGraph, clearLower, 0, clearLower + 1, 0) != OK ||
                gp_AddEdge(clearGraph, clearLower, 0, clearLower + 1, 0) != OK)
                Result = NOTOK;
        }

        if (Result == OK)
        {
            clearGraph->graphFlags &= ~GRAPHFLAGS_PARALLELEDGEDETECTED;

            if (s6_NewWriter(&clearWriter, clearGraph) != OK ||
                s6_InitWriterWithString(clearWriter, &clearStr) != OK ||
                s6_WriteGraph(clearWriter) != OK)
                Result = NOTOK;
            else if (s6_StoreGraphChange(clearWriter, gp_FindEdge(clearGraph, clearLower, clearLower + 1), NIL, NIL) == OK)
            {
                gp_SetQuietMode(origQuietMode);
                gp_ErrorMessage("A change was stored after a line with parallel "
                                "edges whose flag was clear.");
                Result = NOTOK;
            }
        }

        s6_FreeWriter(&clearWriter);
        gp_SetQuietMode(origQuietMode);

        if (Result == OK)
            Result = compareSparse6Output(clearStr, ":Ab\n", "a multigraph whose flag is clear");

        if (clearStr != NULL)
            free(clearStr);
        gp_Free(&clearGraph);
    }

    gp_SetQuietMode(origQuietMode);

    s6_FreeWriter(&theWriter);

    if (Result == OK)
        Result = compareSparse6Output(outputStr, expected, "the multigraph contract cases");

    if (outputStr != NULL)
        free(outputStr);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runSparse6PetersenMultigraphTest()

 The Petersen graph with each edge four times is read from its adjacency
 list, which lists each edge in one direction only and so reads as a
 digraph, and its direction flags are cleared; it is written as sparse6
 and read back, and the graph read back must have the parallel edge flag
 set, the same order and edge count, the same degree at every vertex, and
 the same edges with the same multiplicities.
 ****************************************************************************/

static int runSparse6PetersenMultigraphTest(void)
{
    int Result = OK;
    int numPairs = 0;
    int *pairs = NULL;
    char *s6Str = NULL;
    graphP original = NULL, readBack = NULL;

    if ((original = gp_New()) == NULL || (readBack = gp_New()) == NULL ||
        gp_Read(original, "Petersen-with-parallel-edges.txt") != OK ||
        gp_ClearEdgeDirectionFlags(original) != OK ||
        gp_WriteToString(original, &s6Str, WRITE_SPARSE6) != OK ||
        gp_ReadFromString(readBack, s6Str) != OK)
        Result = NOTOK;

    if (Result == OK &&
        (!(gp_GetGraphFlags(readBack) & GRAPHFLAGS_PARALLELEDGEDETECTED) ||
         gp_GetN(readBack) != gp_GetN(original) ||
         gp_GetM(original) != 60 || gp_GetM(readBack) != 60))
        Result = NOTOK;

    for (int v = gp_LowerBoundVertices(original); Result == OK && v < gp_UpperBoundVertices(original); v++)
    {
        if (gp_GetVertexDegree(readBack, v) != gp_GetVertexDegree(original, v))
            Result = NOTOK;
    }

    if (Result == OK &&
        (collectSparse6TestPairs(original, &pairs, &numPairs) != OK ||
         compareSparse6EdgeMultiset(readBack, (int const(*)[2])pairs, numPairs) != OK))
        Result = NOTOK;

    if (Result != OK)
        gp_ErrorMessage("The Petersen graph with parallel edges did not come "
                        "back the same through sparse6.");

    if (pairs != NULL)
        free(pairs);
    if (s6Str != NULL)
        free(s6Str);
    gp_Free(&original);
    gp_Free(&readBack);

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
        {":~~??@HN_qRvo??THN]\n", 300000, 2, {{5, 299999}, {299998, 299999}}}, // codespell:ignore thn
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

/****************************************************************************
 runSparse6LookaheadTests()

 Exercises s6_RetrieveGraphChange() and gp_RetrieveGraphChange(). The
 files are read twice in lockstep, once with the lookahead and once without
 it, so the reader without it is the oracle: before each read, the changes
 retrieved must be edges of the symmetric difference between the graph
 read last and the next graph, each once, deletions named by an edge record
 of the graph in use, additions by two vertices the graph does not join,
 and, when all are retrieved from a ';' line, all of them; after the read,
 both graphs must be the same. With partial retrieval, s6_ReadGraph() must
 finish the line. The string scripts pin the calls in between: what is
 reported before the first graph, before a ':' line, at the end of the
 input, after an empty ';' line, and what is refused.
 ****************************************************************************/

#define LOOKAHEAD_DELETE 0
#define LOOKAHEAD_ADD 1

static int runSparse6LookaheadLockstepTest(char const *s6FileName, int inputInMemFlag, int partial, int expectedNumGraphs);
static char *getSparse6LineKinds(char const *s6Str, int *pNumLines);
static int countSymmetricDifference(graphP aGraph, graphP bGraph);
static int runSparse6LookaheadScript(char const *s6Str, char const *script, int const pairs[][2], char const *expectedG6Line);
static int runGPLookaheadTests(void);

// Returns the first character of each graph line of s6Str, the header
// aside, as a string of ':' and ';', and the number of lines.
static char *getSparse6LineKinds(char const *s6Str, int *pNumLines)
{
    char const *header = ">>sparse6<<";
    char const *p = s6Str;
    size_t numLines = 0, len = strlen(s6Str);
    char *kinds = (char *)malloc(len + 1);

    if (kinds == NULL)
        return NULL;

    if (strncmp(p, header, strlen(header)) == 0)
        p += strlen(header);

    while (*p != '\0')
    {
        kinds[numLines++] = *p;

        while (*p != '\0' && *p != '\n')
            p++;
        if (*p == '\n')
            p++;
    }

    kinds[numLines] = '\0';
    (*pNumLines) = (int)numLines;

    return kinds;
}

// The number of edges in one of the two graphs but not the other. The
// graphs have the same order, so a vertex is the same index in both.
static int countSymmetricDifference(graphP aGraph, graphP bGraph)
{
    int count = 0;
    graphP graphs[2] = {aGraph, bGraph};

    for (int i = 0; i < 2; i++)
    {
        graphP theGraph = graphs[i], otherGraph = graphs[1 - i];

        for (int e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); e += 2)
        {
            if (gp_EdgeNotInUse(theGraph, e))
                continue;

            if (!gp_IsEdge(otherGraph, gp_FindEdge(otherGraph, gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)), gp_GetNeighbor(theGraph, e))))
                count++;
        }
    }

    return count;
}

static int runSparse6LookaheadLockstepTest(char const *s6FileName, int inputInMemFlag, int partial, int expectedNumGraphs)
{
    int Result = OK;
    int numGraphs = 0, numLines = 0, order = 0, lb = 0;
    int numChanges = 0, changeCapacity = 0;
    int *changes = NULL;
    char *seen = NULL, *s6InputStr = NULL, *lineKinds = NULL;
    graphP laGraph = NULL, plainGraph = NULL;
    S6ReadIteratorP laReader = NULL, plainReader = NULL;

    if ((laGraph = gp_New()) == NULL || (plainGraph = gp_New()) == NULL ||
        (s6InputStr = ReadTextFileIntoString(s6FileName)) == NULL ||
        (lineKinds = getSparse6LineKinds(s6InputStr, &numLines)) == NULL)
        Result = NOTOK;

    if (Result == OK &&
        (s6_NewReader((&laReader), laGraph) != OK || s6_NewReader((&plainReader), plainGraph) != OK ||
         s6_InitReaderWithFileName(plainReader, s6FileName) != OK ||
         (inputInMemFlag ? s6_InitReaderWithString(laReader, s6InputStr)
                         : s6_InitReaderWithFileName(laReader, s6FileName)) != OK))
        Result = NOTOK;

    if (Result == OK)
    {
        order = gp_GetN(laGraph);
        lb = gp_LowerBoundVertexStorage(laGraph);
        if ((seen = (char *)calloc((size_t)order * order, 1)) == NULL)
            Result = NOTOK;
    }

    while (Result == OK)
    {
        int e = NIL, u = NIL, v = NIL;
        int limit = partial ? numGraphs % 5 : INT_MAX;
        int lineKind = numGraphs < numLines ? lineKinds[numGraphs] : '\0';

        // Retrieve the changes to the graph read last, all of them or the
        // first few, checking each against that graph
        numChanges = 0;
        while (Result == OK && numChanges < limit)
        {
            int kind = LOOKAHEAD_ADD;

            if (s6_RetrieveGraphChange(laReader, &e, &u, &v) != OK)
            {
                gp_ErrorMessage("Retrieving change %d before graph %d of "
                                "\"%s\" failed.",
                                numChanges + 1, numGraphs + 1, s6FileName);
                Result = NOTOK;
                break;
            }

            if (e == NIL && u == NIL && v == NIL)
                break;

            if (e != NIL)
            {
                if (u != NIL || v != NIL || e < gp_LowerBoundEdges(laGraph) ||
                    e >= gp_UpperBoundEdges(laGraph) || gp_EdgeNotInUse(laGraph, e))
                    Result = NOTOK;
                else
                {
                    kind = LOOKAHEAD_DELETE;
                    u = gp_GetNeighbor(laGraph, gp_GetTwin(laGraph, e));
                    v = gp_GetNeighbor(laGraph, e);
                }
            }
            else if (u == NIL || v == NIL || u >= v || gp_IsEdge(laGraph, gp_FindEdge(laGraph, u, v)))
                Result = NOTOK;

            if (Result != OK)
            {
                gp_ErrorMessage("Change %d retrieved before graph %d of \"%s\" "
                                "is not a deletion of an edge of the graph or "
                                "an addition of an edge it lacks.",
                                numChanges + 1, numGraphs + 1, s6FileName);
                break;
            }

            if (numChanges == changeCapacity)
            {
                int *newChanges = (int *)realloc(changes, (size_t)(changeCapacity * 2 + 16) * 3 * sizeof(int));

                if (newChanges == NULL)
                {
                    Result = NOTOK;
                    break;
                }

                changes = newChanges;
                changeCapacity = changeCapacity * 2 + 16;
            }

            changes[3 * numChanges] = kind;
            changes[3 * numChanges + 1] = u < v ? u : v;
            changes[3 * numChanges + 2] = u < v ? v : u;
            numChanges++;
        }

        // Once a line is used up, retrieving again still reports no change
        if (Result == OK && !partial &&
            (s6_RetrieveGraphChange(laReader, &e, &u, &v) != OK || e != NIL || u != NIL || v != NIL))
        {
            gp_ErrorMessage("A change was retrieved before graph %d of \"%s\" "
                            "after the line was used up.",
                            numGraphs + 1, s6FileName);
            Result = NOTOK;
        }

        // The next graph, read without the lookahead, is the oracle
        if (Result != OK || s6_ReadGraph(plainReader) != OK)
        {
            Result = NOTOK;
            break;
        }

        if (s6_EndReached(plainReader) || lineKind != ';')
        {
            if (numChanges != 0)
            {
                gp_ErrorMessage("A change was retrieved before graph %d of "
                                "\"%s\", which is not an incremental graph.",
                                numGraphs + 1, s6FileName);
                Result = NOTOK;
            }
        }
        else
        {
            for (int i = 0; Result == OK && i < numChanges; i++)
            {
                int kind = changes[3 * i], cu = changes[3 * i + 1], cv = changes[3 * i + 2];
                // gp_IsEdge() is the edge record itself in some builds, so it
                // is made a truth value before it is compared with one
                int inNext = gp_IsEdge(plainGraph, gp_FindEdge(plainGraph, cu, cv)) ? TRUE : FALSE;
                char *mark = seen + (size_t)(cu - lb) * order + (cv - lb);

                if ((kind == LOOKAHEAD_DELETE) == inNext || *mark)
                {
                    gp_ErrorMessage("Change %d retrieved before graph %d of "
                                    "\"%s\", on {%d, %d}, is not a change "
                                    "the graph makes, or is reported twice.",
                                    i + 1, numGraphs + 1, s6FileName, cu, cv);
                    Result = NOTOK;
                }

                *mark = 1;
            }

            for (int i = 0; i < numChanges; i++)
                seen[(size_t)(changes[3 * i + 1] - lb) * order + (changes[3 * i + 2] - lb)] = 0;

            if (Result == OK && !partial && numChanges != countSymmetricDifference(laGraph, plainGraph))
            {
                gp_ErrorMessage("Retrieved %d changes before graph %d of "
                                "\"%s\", which makes %d.",
                                numChanges, numGraphs + 1, s6FileName,
                                countSymmetricDifference(laGraph, plainGraph));
                Result = NOTOK;
            }
        }

        if (Result != OK)
            break;

        if (s6_ReadGraph(laReader) != OK)
        {
            gp_ErrorMessage("Graph %d of \"%s\" could not be read after the "
                            "lookahead.",
                            numGraphs + 1, s6FileName);
            Result = NOTOK;
            break;
        }

        if (s6_EndReached(laReader) || s6_EndReached(plainReader))
        {
            if (!s6_EndReached(laReader) || !s6_EndReached(plainReader))
            {
                gp_ErrorMessage("The lookahead reader and the plain reader "
                                "reached the end of \"%s\" at different "
                                "graphs.",
                                s6FileName);
                Result = NOTOK;
            }
            break;
        }

        numGraphs++;

        {
            char *laStr = NULL, *plainStr = NULL; // codespell:ignore lastr

            if (gp_GetM(laGraph) != gp_GetM(plainGraph) ||
                gp_UpperBoundEdges(laGraph) != gp_LowerBoundEdges(laGraph) + (gp_GetM(laGraph) << 1) ||
                gp_WriteToString(laGraph, &laStr, WRITE_G6) != OK || laStr == NULL || // codespell:ignore lastr
                gp_WriteToString(plainGraph, &plainStr, WRITE_G6) != OK || plainStr == NULL ||
                strcmp(laStr, plainStr) != 0) // codespell:ignore lastr
            {
                gp_ErrorMessage("Graph %d of \"%s\" read after the lookahead "
                                "differs from the graph read without it.",
                                numGraphs, s6FileName);
                Result = NOTOK;
            }

            if (laStr != NULL) // codespell:ignore lastr
                free(laStr);   // codespell:ignore lastr
            if (plainStr != NULL)
                free(plainStr);
        }
    }

    if (Result == OK && numGraphs != expectedNumGraphs)
    {
        gp_ErrorMessage("Expected %d graphs in \"%s\" but read %d.",
                        expectedNumGraphs, s6FileName, numGraphs);
        Result = NOTOK;
    }

    if (Result == OK)
        gp_Message("Lookahead over the %d graphs in \"%s\" (read %s, %s "
                   "retrieval) matches the reader without it.",
                   numGraphs, s6FileName, inputInMemFlag ? "from a string" : "from the file",
                   partial ? "partial" : "full");

    s6_FreeReader((&laReader));
    s6_FreeReader((&plainReader));
    gp_Free(&laGraph);
    gp_Free(&plainGraph);

    if (changes != NULL)
        free(changes);
    if (seen != NULL)
        free(seen);
    if (lineKinds != NULL)
        free(lineKinds);
    if (s6InputStr != NULL)
        free(s6InputStr);

    return Result;
}

/****************************************************************************
 runSparse6LookaheadScript()

 Runs a script of calls against a sparse6 reader of s6Str, one character
 per call: 'r' reads a graph, 'e' reads and must reach the end, 'f' reads
 and must be refused; 'n' retrieves and must get three NILs, 'a' must get
 the addition of the next pair of pairs[], 'd' the deletion of an edge
 record joining the next pair, 'x' must be refused, and 'p' passes a NULL
 out-pointer, which must be refused; 'm' changes the graph directly, by
 deleting an edge if it has one and otherwise adding {0, 1}. The pairs are
 0-based, as in the file. If expectedG6Line is not NULL, the graph must
 then have that graph6 encoding.
 ****************************************************************************/

static int runSparse6LookaheadScript(char const *s6Str, char const *script, int const pairs[][2], char const *expectedG6Line)
{
    int Result = OK;
    int pairIndex = 0;
    char *s6Copy = NULL, *actualG6 = NULL;
    graphP theGraph = NULL;
    S6ReadIteratorP theReader = NULL;
    char const *step = script;

    if ((s6Copy = copySparse6TestString(s6Str)) == NULL || (theGraph = gp_New()) == NULL ||
        s6_NewReader((&theReader), theGraph) != OK ||
        s6_InitReaderWithString(theReader, s6Copy) != OK)
        Result = NOTOK;

    for (; Result == OK && *step != '\0'; step++)
    {
        // The outputs start as values no call reports, so that a call that
        // returns without setting them is caught
        int e = -2, u = -2, v = -2, rv = OK;
        int lb = gp_LowerBoundVertexStorage(theGraph);

        switch (*step)
        {
        case 'r':
        case 'e':
            if (s6_ReadGraph(theReader) != OK || s6_EndReached(theReader) != (*step == 'e'))
                Result = NOTOK;
            break;
        case 'f':
            if (s6_ReadGraph(theReader) == OK)
                Result = NOTOK;
            break;
        case 'n':
            if (s6_RetrieveGraphChange(theReader, &e, &u, &v) != OK || e != NIL || u != NIL || v != NIL)
                Result = NOTOK;
            break;
        case 'a':
            if (s6_RetrieveGraphChange(theReader, &e, &u, &v) != OK || e != NIL ||
                u != pairs[pairIndex][0] + lb || v != pairs[pairIndex][1] + lb)
                Result = NOTOK;
            pairIndex++;
            break;
        case 'd':
            if (s6_RetrieveGraphChange(theReader, &e, &u, &v) != OK || e == NIL || u != NIL || v != NIL ||
                gp_EdgeNotInUse(theGraph, e) ||
                gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)) + gp_GetNeighbor(theGraph, e) !=
                    pairs[pairIndex][0] + pairs[pairIndex][1] + 2 * lb ||
                (gp_GetNeighbor(theGraph, e) != pairs[pairIndex][0] + lb &&
                 gp_GetNeighbor(theGraph, e) != pairs[pairIndex][1] + lb))
                Result = NOTOK;
            pairIndex++;
            break;
        case 'x':
            if (s6_RetrieveGraphChange(theReader, &e, &u, &v) == OK)
                Result = NOTOK;
            break;
        case 'p':
            if (s6_RetrieveGraphChange(theReader, NULL, &u, &v) == OK ||
                s6_RetrieveGraphChange(theReader, &e, &u, NULL) == OK)
                Result = NOTOK;
            break;
        case 'm':
            e = gp_LowerBoundEdges(theGraph);
            while (e < gp_UpperBoundEdges(theGraph) && gp_EdgeNotInUse(theGraph, e))
                e += 2;
            rv = e < gp_UpperBoundEdges(theGraph) ? gp_DeleteEdge(theGraph, e)
                                                  : gp_DynamicAddEdge(theGraph, lb, 0, lb + 1, 0);
            if (rv != OK)
                Result = NOTOK;
            break;
        default:
            Result = NOTOK;
            break;
        }
    }

    if (Result == OK && expectedG6Line != NULL)
    {
        char const *g6Header = ">>graph6<<";

        if (gp_WriteToString(theGraph, &actualG6, WRITE_G6) != OK || actualG6 == NULL ||
            strncmp(actualG6, g6Header, strlen(g6Header)) != 0 ||
            strncmp(actualG6 + strlen(g6Header), expectedG6Line, strlen(expectedG6Line)) != 0 ||
            actualG6[strlen(g6Header) + strlen(expectedG6Line)] != '\n')
            Result = NOTOK;
    }

    // A step that fails is followed by the increment of the loop, so step
    // is one past it; at the end of the script, it is the graph that failed
    if (Result != OK)
        gp_ErrorMessage("Lookahead script \"%s\" on sparse6 input \"%s\" "
                        "failed at step %d of %d.",
                        script, s6Str, (int)(step - script), (int)strlen(script));

    s6_FreeReader((&theReader));
    gp_Free(&theGraph);

    if (s6Copy != NULL)
        free(s6Copy);
    if (actualG6 != NULL)
        free(actualG6);

    return Result;
}

/****************************************************************************
 runGPLookaheadTests()

 gp_RetrieveGraphChange() reports no change for graph6 input, at any point,
 and for sparse6 input the same changes as s6_RetrieveGraphChange(), and it
 refuses NULL parameters and an uninitialized reader.
 ****************************************************************************/

static int runGPLookaheadTests(void)
{
    int Result = OK;
    int numGraphs = 0;
    int e = NIL, u = NIL, v = NIL;
    graphP gpGraph = NULL, s6Graph = NULL;
    GPReadIteratorP gpReader = NULL;
    S6ReadIteratorP s6Reader = NULL;

    // Refusals before any input
    if ((gpGraph = gp_New()) == NULL || gp_NewReader((&gpReader), gpGraph) != OK ||
        gp_RetrieveGraphChange(gpReader, &e, &u, &v) == OK ||
        gp_RetrieveGraphChange(NULL, &e, &u, &v) == OK ||
        s6_RetrieveGraphChange(NULL, &e, &u, &v) == OK)
    {
        gp_ErrorMessage("A change was retrieved from a reader with no input.");
        Result = NOTOK;
    }

    gp_FreeReader((&gpReader));
    gp_Free(&gpGraph);

    // Graph6 input has no incremental graphs
    if (Result == OK &&
        ((gpGraph = gp_New()) == NULL || gp_NewReader((&gpReader), gpGraph) != OK ||
         gp_InitReaderWithFileName(gpReader, "N5-all.g6") != OK))
        Result = NOTOK;

    while (Result == OK)
    {
        // The outputs start as values no call reports, so that a call that
        // returns without setting them is caught
        e = u = v = -2;

        if (gp_RetrieveGraphChange(gpReader, &e, &u, &v) != OK || e != NIL || u != NIL || v != NIL ||
            gp_RetrieveGraphChange(gpReader, NULL, &u, &v) == OK)
        {
            gp_ErrorMessage("gp_RetrieveGraphChange() did not report no change "
                            "before graph %d of \"N5-all.g6\".",
                            numGraphs + 1);
            Result = NOTOK;
            break;
        }

        if (gp_ReadGraph(gpReader) != OK)
        {
            Result = NOTOK;
            break;
        }

        if (gp_EndReached(gpReader))
            break;

        numGraphs++;
    }

    if (Result == OK && numGraphs != 34)
        Result = NOTOK;

    gp_FreeReader((&gpReader));
    gp_Free(&gpGraph);

    // Sparse6 input is handed to the sparse6 reader
    numGraphs = 0;
    if (Result == OK &&
        ((gpGraph = gp_New()) == NULL || (s6Graph = gp_New()) == NULL ||
         gp_NewReader((&gpReader), gpGraph) != OK ||
         gp_InitReaderWithFileName(gpReader, "N5-all.inc.s6") != OK ||
         s6_NewReader((&s6Reader), s6Graph) != OK ||
         s6_InitReaderWithFileName(s6Reader, "N5-all.inc.s6") != OK))
        Result = NOTOK;

    while (Result == OK)
    {
        int ge = NIL, gu = NIL, gv = NIL;

        do
        {
            ge = gu = gv = -2;
            e = u = v = -3;

            if (gp_RetrieveGraphChange(gpReader, &ge, &gu, &gv) != OK ||
                s6_RetrieveGraphChange(s6Reader, &e, &u, &v) != OK ||
                ge != e || gu != u || gv != v)
            {
                gp_ErrorMessage("gp_RetrieveGraphChange() and "
                                "s6_RetrieveGraphChange() differ before graph "
                                "%d of \"N5-all.inc.s6\".",
                                numGraphs + 1);
                Result = NOTOK;
            }
        } while (Result == OK && (e != NIL || u != NIL));

        if (Result != OK || gp_ReadGraph(gpReader) != OK || s6_ReadGraph(s6Reader) != OK)
        {
            Result = NOTOK;
            break;
        }

        if (gp_EndReached(gpReader))
            break;

        numGraphs++;
    }

    if (Result == OK && numGraphs != 34)
        Result = NOTOK;

    gp_FreeReader((&gpReader));
    s6_FreeReader((&s6Reader));
    gp_Free(&gpGraph);
    gp_Free(&s6Graph);

    // A ';' line after a graph with parallel edges is refused at the gp_
    // level as at the s6_ level
    if (Result == OK)
    {
        char *multiStr = copySparse6TestString(":Ab\n;n\n");

        e = u = v = -2;

        if (multiStr == NULL || (gpGraph = gp_New()) == NULL ||
            gp_NewReader((&gpReader), gpGraph) != OK ||
            gp_InitReaderWithString(gpReader, multiStr) != OK ||
            gp_ReadGraph(gpReader) != OK ||
            gp_RetrieveGraphChange(gpReader, &e, &u, &v) == OK)
        {
            gp_ErrorMessage("gp_RetrieveGraphChange() did not refuse a ';' "
                            "line after a graph with parallel edges.");
            Result = NOTOK;
        }

        gp_FreeReader((&gpReader));
        gp_Free(&gpGraph);
        if (multiStr != NULL)
            free(multiStr);
    }

    if (Result == OK)
        gp_Message("gp_RetrieveGraphChange() reports no change for graph6 "
                   "input and hands sparse6 input to the sparse6 reader.");

    return Result;
}

int runSparse6LookaheadTests(void)
{
    int Result = OK;
    unsigned origQuietMode = gp_GetQuietMode();
    size_t i = 0;

    int const pairs04[][2] = {{0, 4}};
    int const pairs01[][2] = {{0, 1}};
    int const pairs04and14[][2] = {{0, 4}, {1, 4}};

    // {sparse6 input, script, pairs, graph6 encoding of the final graph}
    struct
    {
        char const *s6Str;
        char const *script;
        int const (*pairs)[2];
        char const *expectedG6Line;
    } scripts[] = {
        // Nothing incremental before the first graph, after a used-up line,
        // at the end of the input, and after the end
        {":D\n;oN\n", "nrannrnnen", pairs04, "D?_"},
        // An empty ';' line reports no change and is still a graph, and a
        // ':' line after it is found and read
        {":D\n;\n:D\n", "rnrnre", NULL, "D??"},
        // A deletion is reported as an edge record of the edge
        {":D\n;oN\n;oN\n", "rrdnre", pairs04, "D??"},
        // A ':' line found by the lookahead after a ':' line
        {":A\n:A\n;n\n", "rnranre", pairs01, "A_"},
        // s6_ReadGraph() finishes a line the lookahead has begun, and then
        // reads the ':' line after it
        {":D\n;o@~\n", "rare", pairs04and14, "D?o"},
        {":D\n;o@~\n:D\n", "rarnre", pairs04, "D??"},
        // A NULL out-pointer is refused without spoiling the reader
        {":D\n;oN\n", "rpare", pairs04, "D?_"},
        // A direct change to the graph before a ':' line does not matter
        {":D\n:D\n", "rmre", NULL, "D??"},
        // but it does once an incremental line has been retrieved, even an
        // empty one
        {":An\n;\n", "rnmxf", NULL, NULL},
        // Refused: a pair twice on a ';' line, a loop, a byte out of range,
        // a line that begins with neither ':' nor ';', and a direct change
        // to the graph after the last read, before or between retrievals
        // or before the read, with or without a retrieval
        {":D\n;o?~\n", "raxfx", pairs04, NULL},
        {":D\n;o?~\n", "rf", NULL, NULL},
        {":D\n;B~\n", "rxf", NULL, NULL},
        {":D\n;o \n", "rxf", NULL, NULL},
        {":D\nX\n", "rxf", NULL, NULL},
        {":D\n;oN\n", "ramf", pairs04, NULL},
        {":D\n;o@~\n", "ramxf", pairs04, NULL},
        {":D\n;oN\n", "rmxf", NULL, NULL},
        {":D\n;oN\n", "rmf", NULL, NULL},
        // Refused: a ';' line after a graph with parallel edges, empty or
        // not, by the lookahead and by the read
        {":Ab\n;n\n", "rxf", NULL, NULL},
        {":Ab\n;\n", "rxf", NULL, NULL},
        {":Ab\n;n\n", "rf", NULL, NULL},
        // A ':' line after a graph with parallel edges is found and read,
        // and ';' lines apply again after it
        {":Ab\n:An\n;n\n", "rnrdnre", pairs01, "A?"},
        // A direct change before the first read is replaced by the first
        // graph, as by every later ':' line, so it cannot leave the graph
        // with a parallel edge that the line did not have
        {":An\n;n\n", "mrdnre", pairs01, "A?"},
    };

    gp_Message("Start sparse6 lookahead tests");

    if (Result == OK && runSparse6LookaheadLockstepTest("N5-all.inc.s6", FALSE, FALSE, 34) != OK)
        Result = NOTOK;
    if (Result == OK && runSparse6LookaheadLockstepTest("N5-all.inc.s6", TRUE, TRUE, 34) != OK)
        Result = NOTOK;
    if (Result == OK && runSparse6LookaheadLockstepTest("n8.mALL.inc.s6", TRUE, FALSE, 12346) != OK)
        Result = NOTOK;
    if (Result == OK && runSparse6LookaheadLockstepTest("n8.mALL.inc.s6", FALSE, TRUE, 12346) != OK)
        Result = NOTOK;

    // The refusals report errors, which are expected
    gp_SetQuietMode(QUIETMODE_ALL);

    for (i = 0; Result == OK && i < sizeof(scripts) / sizeof(scripts[0]); i++)
    {
        if (runSparse6LookaheadScript(scripts[i].s6Str, scripts[i].script, scripts[i].pairs,
                                      scripts[i].expectedG6Line) != OK)
        {
            gp_SetQuietMode(origQuietMode);
            gp_ErrorMessage("Lookahead script %d failed.", (int)i + 1);
            Result = NOTOK;
        }
    }

    if (Result == OK && runGPLookaheadTests() != OK)
        Result = NOTOK;

    gp_SetQuietMode(origQuietMode);

    if (Result == OK)
        gp_Message("Sparse6 lookahead tests succeeded.\n");
    else
        gp_ErrorMessage("Sparse6 lookahead tests FAILED.\n");

    return Result;
}
