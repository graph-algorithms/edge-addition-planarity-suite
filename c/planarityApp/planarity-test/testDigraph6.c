/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "../planarity.h"

/* Called from planarityCommandLine.c */
int testPetersenDigraph6(void);
int testDigraph6Iterators(void);
int runDigraph6Tests(void);

/* Defined in planarityCommandLine.c */
int runTestAllGraphsTest(char const *commandString, char const *infileName, char const *expectedValidationStr, int (*mutatorFunc)(graphP));
int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag);

// An arc u -> v of a test graph when directed is TRUE, otherwise the
// undirected edge {u, v}; vertices are numbered from 0, as in the format
typedef struct
{
    int u, v, directed;
} digraph6TestArc;

static char *copyDigraph6TestString(char const *str);
static int checkDigraph6Arcs(graphP theGraph, int order, digraph6TestArc const arcs[], int numArcs, char const *what);
static int compareDigraph6Arcs(graphP theGraph, graphP theOtherGraph, char const *what);
static graphP makeDigraph6TestGraph(int order, digraph6TestArc const arcs[], int numArcs);
static int runDigraph6AcceptTest(char const *d6Str, int order, digraph6TestArc const arcs[], int numArcs);
static int runDigraph6RejectTest(char const *d6Str);
static int runDigraph6ReadTests(void);
static int runDigraph6InitRetryTests(void);
static int runDigraph6WriteLineTest(int order, digraph6TestArc const arcs[], int numArcs, char const *expectedOutput);
static int runDigraph6WriteRefusalTest(graphP theGraph, char const *what);
static int runDigraph6WriteTests(void);
static int runDigraph6FileTest(char const *inFileName, char const *expectedFileName, int expectedNumGraphs);
static int runHasParallelEdgesTests(void);

/****************************************************************************
 copyDigraph6TestString()

 Returns an allocated copy of a test string, for the read methods, which
 take a modifiable string, or NULL if the copy cannot be allocated.
 ****************************************************************************/

static char *copyDigraph6TestString(char const *str)
{
    char *copy = (char *)malloc(strlen(str) + 1);

    if (copy != NULL)
        strcpy(copy, str);

    return copy;
}

/****************************************************************************
 checkDigraph6Arcs()

 Checks that theGraph has the given order and exactly the given arcs and
 undirected edges, with the direction flags of both edge records of each
 one, and that the graph is flagged as having directed edges exactly when
 one of them is directed.
 ****************************************************************************/

static int checkDigraph6Arcs(graphP theGraph, int order, digraph6TestArc const arcs[], int numArcs, char const *what)
{
    int lowerBound = gp_LowerBoundVertexStorage(theGraph);
    int anyDirected = FALSE;

    if (gp_GetN(theGraph) != order || gp_GetM(theGraph) != numArcs)
    {
        gp_ErrorMessage("%s: expected order %d and %d edges, got order %d and %d edges.",
                        what, order, numArcs, gp_GetN(theGraph), gp_GetM(theGraph));
        return NOTOK;
    }

    for (int i = 0; i < numArcs; i++)
    {
        int e = gp_FindEdge(theGraph, arcs[i].u + lowerBound, arcs[i].v + lowerBound);
        int expectedDirection = arcs[i].directed ? EDGEFLAG_DIRECTION_OUTONLY : 0;
        int expectedTwinDirection = arcs[i].directed ? EDGEFLAG_DIRECTION_INONLY : 0;

        if (!gp_IsEdge(theGraph, e) ||
            (int)gp_GetDirection(theGraph, e) != expectedDirection ||
            (int)gp_GetDirection(theGraph, gp_GetTwin(theGraph, e)) != expectedTwinDirection)
        {
            gp_ErrorMessage("%s: the edge between %d and %d is missing or has "
                            "the wrong direction.",
                            what, arcs[i].u, arcs[i].v);
            return NOTOK;
        }

        if (arcs[i].directed)
            anyDirected = TRUE;
    }

    if (((gp_GetGraphFlags(theGraph) & GRAPHFLAGS_DIRECTEDEDGEDETECTED) ? TRUE : FALSE) != anyDirected)
    {
        gp_ErrorMessage("%s: the graph's directed edge flag is wrong.", what);
        return NOTOK;
    }

    return OK;
}

/****************************************************************************
 compareDigraph6Arcs()

 Checks that two graphs of the same order and edge count have the same
 edges, each with the same direction flag in the same adjacency list.
 Neither graph may have parallel edges, so the edges correspond one to one.
 ****************************************************************************/

static int compareDigraph6Arcs(graphP theGraph, graphP theOtherGraph, char const *what)
{
    if (gp_GetN(theGraph) != gp_GetN(theOtherGraph) || gp_GetM(theGraph) != gp_GetM(theOtherGraph) ||
        (gp_GetGraphFlags(theGraph) & GRAPHFLAGS_DIRECTEDEDGEDETECTED) != (gp_GetGraphFlags(theOtherGraph) & GRAPHFLAGS_DIRECTEDEDGEDETECTED))
    {
        gp_ErrorMessage("%s: the graphs differ in order, edge count or "
                        "directed edge flag.",
                        what);
        return NOTOK;
    }

    for (int v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); v++)
    {
        for (int e = gp_GetFirstEdge(theGraph, v); gp_IsEdge(theGraph, e); e = gp_GetNextEdge(theGraph, e))
        {
            int f = gp_FindEdge(theOtherGraph, v, gp_GetNeighbor(theGraph, e));

            if (!gp_IsEdge(theOtherGraph, f) || gp_GetDirection(theOtherGraph, f) != gp_GetDirection(theGraph, e))
            {
                gp_ErrorMessage("%s: the edge from %d to %d differs.",
                                what, v, gp_GetNeighbor(theGraph, e));
                return NOTOK;
            }
        }
    }

    return OK;
}

/****************************************************************************
 makeDigraph6TestGraph()

 Returns a new graph of the given order with the given arcs and undirected
 edges, or NULL on failure.
 ****************************************************************************/

static graphP makeDigraph6TestGraph(int order, digraph6TestArc const arcs[], int numArcs)
{
    graphP theGraph = gp_New();
    int lowerBound = 0;

    if (theGraph == NULL || gp_EnsureVertexCapacity(theGraph, order) != OK)
    {
        gp_Free(&theGraph);
        return NULL;
    }

    lowerBound = gp_LowerBoundVertexStorage(theGraph);

    for (int i = 0; i < numArcs; i++)
    {
        if (gp_AddEdge(theGraph, arcs[i].u + lowerBound, 0, arcs[i].v + lowerBound, 0) != OK)
        {
            gp_Free(&theGraph);
            return NULL;
        }

        if (arcs[i].directed)
        {
            gp_SetDirection(theGraph, gp_GetFirstEdge(theGraph, arcs[i].u + lowerBound), EDGEFLAG_DIRECTION_OUTONLY);
        }
    }

    return theGraph;
}

/****************************************************************************
 runDigraph6AcceptTest()

 Reads a digraph6 string with gp_ReadFromString(), which must recognize it
 as digraph6, and checks the graph read against the expected arcs.
 ****************************************************************************/

static int runDigraph6AcceptTest(char const *d6Str, int order, digraph6TestArc const arcs[], int numArcs)
{
    graphP theGraph = gp_New();
    char *inputStr = copyDigraph6TestString(d6Str);
    int Result = OK;

    if (theGraph == NULL || inputStr == NULL)
        Result = NOTOK;
    else if (gp_ReadFromString(theGraph, inputStr) != OK)
    {
        gp_ErrorMessage("Unable to read the digraph6 string \"%s\".", d6Str);
        Result = NOTOK;
    }
    else
        Result = checkDigraph6Arcs(theGraph, order, arcs, numArcs, d6Str);

    if (inputStr != NULL)
        free(inputStr);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runDigraph6RejectTest()

 Reads every graph of a digraph6 string with the digraph6 read iterator,
 the same path through which the accepted strings are read, and returns
 OK only if initialization or one of the reads fails.
 ****************************************************************************/

static int runDigraph6RejectTest(char const *d6Str)
{
    graphP theGraph = gp_New();
    D6ReadIteratorP theReader = NULL;
    char *inputStr = copyDigraph6TestString(d6Str);
    int quietModeCache = gp_GetQuietMode();
    int rejected = FALSE;

    if (theGraph == NULL || inputStr == NULL || d6_NewReader(&theReader, theGraph) != OK)
    {
        if (inputStr != NULL)
            free(inputStr);
        gp_Free(&theGraph);
        return NOTOK;
    }

    gp_SetQuietMode(QUIETMODE_ALL);

    if (d6_InitReaderWithString(theReader, inputStr) != OK)
        rejected = TRUE;
    else
    {
        while (!d6_EndReached(theReader))
        {
            if (d6_ReadGraph(theReader) != OK)
            {
                rejected = TRUE;
                break;
            }
        }
    }

    gp_SetQuietMode(quietModeCache);

    d6_FreeReader(&theReader);
    free(inputStr);
    gp_Free(&theGraph);

    if (!rejected)
    {
        gp_ErrorMessage("The invalid digraph6 input \"%s\" was accepted.", d6Str);
        return NOTOK;
    }

    return OK;
}

static int runDigraph6ReadTests(void)
{
    // The example of nauty's formats.txt: 0->2, 0->4, 3->1 and 3->4
    digraph6TestArc const nautyExample[] = {{0, 2, TRUE}, {0, 4, TRUE}, {3, 1, TRUE}, {3, 4, TRUE}};
    // Bits 0110: the arcs 0->1 and 1->0, which are one undirected edge
    digraph6TestArc const twoCycle[] = {{0, 1, FALSE}};
    // Bits 0010: the arc 1->0 alone
    digraph6TestArc const backwardArc[] = {{1, 0, TRUE}};
    // Bits 010 001 010: 0->1 alone, and both arcs between 1 and 2
    digraph6TestArc const mixed[] = {{0, 1, TRUE}, {1, 2, FALSE}};
    char *order63Str = NULL;
    int Result = OK;

    char const *rejectCases[] = {
        // A loop on vertex 0 (bits 1000)
        "&A_\n",
        // A padding bit set (bits 0000 10)
        "&AA\n",
        // A byte out of range, a line too long and one too short, and a first
        // line that ends after the order, or within the matrix, with no newline
        "&A \n",
        "&AGG\n",
        "&A\n",
        "&A",
        "&DI?AO",
        // Order 0, which the graph library does not support
        "&?\n",
        // A later graph of another order, one whose line has the length of
        // the first but begins a longer order, and a later line without '&'
        "&AG\n&B??\n",
        "&@?\n&~?\n",
        "&AG\nAG\n",
        // A first line without '&', a truncated or wrong header, and the
        // header followed by a line terminator rather than the first graph
        "AG\n",
        ">>digraph6<\n",
        ">>digraph7<<&AG\n",
        ">>digraph6<<\n&AG\n",
        // Orders above 100000: an eight-byte order, and 100001
        "&~~???????\n",
        "&~WY`\n",
    };

    gp_Message("Starting digraph6 read tests");

    if (runDigraph6AcceptTest("&DI?AO?\n", 5, nautyExample, 4) != OK ||
        runDigraph6AcceptTest(">>digraph6<<&DI?AO?\n", 5, nautyExample, 4) != OK ||
        runDigraph6AcceptTest("&DI?AO?\r\n", 5, nautyExample, 4) != OK ||
        runDigraph6AcceptTest("&DI?AO?", 5, nautyExample, 4) != OK ||
        runDigraph6AcceptTest("&AW\n", 2, twoCycle, 1) != OK ||
        runDigraph6AcceptTest("&AG\n", 2, backwardArc, 1) != OK ||
        runDigraph6AcceptTest("&BPO\n", 3, mixed, 2) != OK ||
        runDigraph6AcceptTest("&@?\n", 1, NULL, 0) != OK)
        Result = NOTOK;

    // Order 63 takes the four-byte order and 662 bytes for its 3969 bits
    if (Result == OK)
    {
        size_t len = strlen("&~??~") + 662 + 2;

        if ((order63Str = (char *)malloc(len)) == NULL)
            Result = NOTOK;
        else
        {
            strcpy(order63Str, "&~??~");
            memset(order63Str + 5, '?', 662);
            strcpy(order63Str + 5 + 662, "\n");
            Result = runDigraph6AcceptTest(order63Str, 63, NULL, 0);
            free(order63Str);
        }
    }

    for (size_t i = 0; Result == OK && i < sizeof(rejectCases) / sizeof(rejectCases[0]); i++)
        Result = runDigraph6RejectTest(rejectCases[i]);

    if (Result == OK)
        gp_Message("Finished digraph6 read tests.\n");

    return Result;
}

/****************************************************************************
 runDigraph6InitRetryTests()

 A reader or writer whose initialization fails frees the input or output
 it took, so a second initialization neither loses it nor is refused: the
 reader refuses a graph6 file and then reads a digraph6 one, and the writer
 refuses twice a graph of an order above 100000. A lost container shows as
 a leak under AddressSanitizer.
 ****************************************************************************/

static int runDigraph6InitRetryTests(void)
{
    digraph6TestArc const nautyExample[] = {{0, 2, TRUE}, {0, 4, TRUE}, {3, 1, TRUE}, {3, 4, TRUE}};
    graphP theGraph = gp_New();
    D6ReadIteratorP theReader = NULL;
    D6WriteIteratorP theWriter = NULL;
    char *actualOutput = NULL;
    int quietModeCache = gp_GetQuietMode();
    int Result = OK;

    if (theGraph == NULL || d6_NewReader(&theReader, theGraph) != OK)
        Result = NOTOK;
    else
    {
        gp_SetQuietMode(QUIETMODE_ALL);
        if (d6_InitReaderWithFileName(theReader, "nauty_example.g6") == OK)
            Result = NOTOK;
        gp_SetQuietMode(quietModeCache);

        if (Result != OK ||
            d6_InitReaderWithFileName(theReader, "nauty_example.d6") != OK ||
            d6_ReadGraph(theReader) != OK ||
            checkDigraph6Arcs(theGraph, 5, nautyExample, 4, "the reader initialized again") != OK)
        {
            gp_ErrorMessage("The digraph6 reader did not read after a failed initialization.");
            Result = NOTOK;
        }
    }

    d6_FreeReader(&theReader);
    gp_Free(&theGraph);

    if (Result == OK)
    {
        if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 100001) != OK ||
            d6_NewWriter(&theWriter, theGraph) != OK)
            Result = NOTOK;
        else
        {
            gp_SetQuietMode(QUIETMODE_ALL);
            for (int i = 0; i < 2 && Result == OK; i++)
            {
                if (d6_InitWriterWithString(theWriter, &actualOutput) == OK || actualOutput != NULL)
                {
                    gp_ErrorMessage("The digraph6 writer accepted a graph of order 100001.");
                    Result = NOTOK;
                }
            }
            gp_SetQuietMode(quietModeCache);
        }

        d6_FreeWriter(&theWriter);
        if (actualOutput != NULL)
            free(actualOutput);
        gp_Free(&theGraph);
    }

    return Result;
}

/****************************************************************************
 runDigraph6WriteLineTest()

 Writes a graph made of the given arcs and undirected edges and compares
 the output, header included, with the expected output.
 ****************************************************************************/

static int runDigraph6WriteLineTest(int order, digraph6TestArc const arcs[], int numArcs, char const *expectedOutput)
{
    graphP theGraph = makeDigraph6TestGraph(order, arcs, numArcs);
    char *actualOutput = NULL;
    int Result = OK;

    if (theGraph == NULL)
        return NOTOK;

    if (gp_WriteToString(theGraph, &actualOutput, WRITE_DIGRAPH6) != OK ||
        actualOutput == NULL || strcmp(actualOutput, expectedOutput) != 0)
    {
        gp_ErrorMessage("The digraph6 output \"%s\" differs from the expected \"%s\".",
                        actualOutput == NULL ? "" : actualOutput, expectedOutput);
        Result = NOTOK;
    }

    if (actualOutput != NULL)
        free(actualOutput);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runDigraph6WriteRefusalTest()

 Checks that the graph is not written, and that no output is returned.
 ****************************************************************************/

static int runDigraph6WriteRefusalTest(graphP theGraph, char const *what)
{
    char *actualOutput = NULL;
    int quietModeCache = gp_GetQuietMode();
    int Result = OK;

    gp_SetQuietMode(QUIETMODE_ALL);
    if (gp_WriteToString(theGraph, &actualOutput, WRITE_DIGRAPH6) == OK || actualOutput != NULL)
        Result = NOTOK;
    gp_SetQuietMode(quietModeCache);

    if (Result != OK)
        gp_ErrorMessage("The digraph6 writer did not refuse %s.", what);

    if (actualOutput != NULL)
        free(actualOutput);

    return Result;
}

static int runDigraph6WriteTests(void)
{
    digraph6TestArc const nautyExample[] = {{0, 2, TRUE}, {0, 4, TRUE}, {3, 1, TRUE}, {3, 4, TRUE}};
    digraph6TestArc const twoCycle[] = {{0, 1, FALSE}};
    digraph6TestArc const backwardArc[] = {{1, 0, TRUE}};
    digraph6TestArc const mixed[] = {{0, 1, TRUE}, {1, 2, FALSE}};
    digraph6TestArc const parallelUndirected[] = {{0, 1, FALSE}, {1, 2, FALSE}, {0, 1, FALSE}};
    digraph6TestArc const parallelOpposite[] = {{0, 1, TRUE}, {1, 0, TRUE}};
    digraph6TestArc const loop[] = {{0, 1, FALSE}, {2, 2, FALSE}};
    graphP theGraph = NULL;
    D6WriteIteratorP theWriter = NULL;
    char *actualOutput = NULL;
    int quietModeCache = 0;
    int e = NIL;
    int Result = OK;

    gp_Message("Starting digraph6 write tests");

    if (runDigraph6WriteLineTest(5, nautyExample, 4, ">>digraph6<<&DI?AO?\n") != OK ||
        runDigraph6WriteLineTest(2, twoCycle, 1, ">>digraph6<<&AW\n") != OK ||
        runDigraph6WriteLineTest(2, backwardArc, 1, ">>digraph6<<&AG\n") != OK ||
        runDigraph6WriteLineTest(3, mixed, 2, ">>digraph6<<&BPO\n") != OK ||
        runDigraph6WriteLineTest(1, NULL, 0, ">>digraph6<<&@?\n") != OK)
        Result = NOTOK;

    // Parallel edges, including two edges in opposite directions, which
    // would be read back as one undirected edge, and a loop
    if (Result == OK)
    {
        digraph6TestArc const *const refused[] = {parallelUndirected, parallelOpposite, loop};
        int const numArcs[] = {3, 2, 2};
        char const *const what[] = {"parallel undirected edges", "opposite directed edges", "a loop"};

        for (int i = 0; Result == OK && i < 3; i++)
        {
            if ((theGraph = makeDigraph6TestGraph(3, refused[i], numArcs[i])) == NULL)
                Result = NOTOK;
            else
                Result = runDigraph6WriteRefusalTest(theGraph, what[i]);
            gp_Free(&theGraph);
        }
    }

    // A hidden edge is refused until it is restored
    if (Result == OK)
    {
        if ((theGraph = makeDigraph6TestGraph(3, mixed, 2)) == NULL)
            Result = NOTOK;
        else
        {
            e = gp_GetFirstEdge(theGraph, gp_LowerBoundVertexStorage(theGraph));
            gp_HideEdge(theGraph, e);
            Result = runDigraph6WriteRefusalTest(theGraph, "a hidden edge");
            gp_RestoreEdge(theGraph, e);

            if (Result == OK && gp_WriteToString(theGraph, &actualOutput, WRITE_DIGRAPH6) != OK)
            {
                gp_ErrorMessage("The digraph6 writer refused a graph whose hidden edge was restored.");
                Result = NOTOK;
            }
            if (actualOutput != NULL)
            {
                free(actualOutput);
                actualOutput = NULL;
            }
        }
        gp_Free(&theGraph);
    }

    // A failed write discards what the iterator wrote before it, so the
    // output string is not returned
    if (Result == OK)
    {
        if ((theGraph = makeDigraph6TestGraph(3, mixed, 2)) == NULL ||
            d6_NewWriter(&theWriter, theGraph) != OK ||
            d6_InitWriterWithString(theWriter, &actualOutput) != OK ||
            d6_WriteGraph(theWriter) != OK)
            Result = NOTOK;
        else
        {
            quietModeCache = gp_GetQuietMode();
            gp_SetQuietMode(QUIETMODE_ALL);
            if (gp_AddEdge(theGraph, gp_LowerBoundVertexStorage(theGraph), 0, gp_LowerBoundVertexStorage(theGraph) + 1, 0) != OK ||
                d6_WriteGraph(theWriter) == OK)
                Result = NOTOK;
            gp_SetQuietMode(quietModeCache);
        }

        d6_FreeWriter(&theWriter);
        if (Result == OK && actualOutput != NULL)
        {
            gp_ErrorMessage("The digraph6 output was returned after a failed write.");
            Result = NOTOK;
        }
        if (actualOutput != NULL)
            free(actualOutput);
        gp_Free(&theGraph);
    }

    if (Result == OK)
        gp_Message("Finished digraph6 write tests.\n");

    return Result;
}

/****************************************************************************
 runDigraph6FileTest()

 Reads every graph of inFileName with the generalized read iterator and
 writes each with the digraph6 write iterator, then compares the output,
 byte for byte apart from line terminators, with expectedFileName, which
 nauty wrote.
 ****************************************************************************/

static int runDigraph6FileTest(char const *inFileName, char const *expectedFileName, int expectedNumGraphs)
{
    graphP theGraph = gp_New();
    GPReadIteratorP theReader = NULL;
    D6WriteIteratorP theWriter = NULL;
    char *actualOutput = NULL;
    int numGraphs = 0;
    int Result = OK;

    if (theGraph == NULL ||
        gp_NewReader(&theReader, theGraph) != OK ||
        gp_InitReaderWithFileName(theReader, inFileName) != OK)
        Result = NOTOK;

    while (Result == OK)
    {
        if (gp_ReadGraph(theReader) != OK)
            Result = NOTOK;
        else if (gp_EndReached(theReader))
            break;
        else if (theWriter == NULL &&
                 (d6_NewWriter(&theWriter, theGraph) != OK ||
                  d6_InitWriterWithString(theWriter, &actualOutput) != OK))
            Result = NOTOK;
        else if (d6_WriteGraph(theWriter) != OK)
            Result = NOTOK;
        else
            numGraphs++;
    }

    d6_FreeWriter(&theWriter);

    if (Result == OK && (numGraphs != expectedNumGraphs || TextFileMatchesString(expectedFileName, actualOutput) != TRUE))
    {
        gp_ErrorMessage("Writing the %d graphs of %s as digraph6 did not "
                        "reproduce %s.",
                        numGraphs, inFileName, expectedFileName);
        Result = NOTOK;
    }

    if (actualOutput != NULL)
        free(actualOutput);
    gp_FreeReader(&theReader);
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runHasParallelEdgesTests()

 Checks gp_HasParallelEdges() on simple graphs and on graphs with each kind
 of parallel edges, with in-only edges included and left out, and that it
 leaves every visited flag clear.
 ****************************************************************************/

static int runHasParallelEdgesTests(void)
{
    // In the path 0-1-2, vertex 2 is a neighbor of 1, whose list also holds
    // 0, so the marks of one vertex's neighbors must be cleared before the
    // next vertex's are set
    digraph6TestArc const path[] = {{0, 1, FALSE}, {1, 2, FALSE}};
    digraph6TestArc const directedPath[] = {{0, 1, TRUE}, {1, 2, TRUE}};
    digraph6TestArc const parallelUndirected[] = {{0, 1, FALSE}, {1, 2, FALSE}, {2, 1, FALSE}};
    // The two arcs of a directed 2-cycle are parallel only when in-only
    // edges are included
    digraph6TestArc const parallelOpposite[] = {{0, 1, FALSE}, {1, 2, TRUE}, {2, 1, TRUE}};
    digraph6TestArc const parallelSameWay[] = {{0, 1, FALSE}, {1, 2, TRUE}, {1, 2, TRUE}};
    digraph6TestArc const parallelMixed[] = {{0, 1, FALSE}, {1, 2, FALSE}, {1, 2, TRUE}};
    digraph6TestArc const parallelMixedBackward[] = {{0, 1, FALSE}, {1, 2, FALSE}, {2, 1, TRUE}};
    digraph6TestArc const oneLoop[] = {{0, 1, FALSE}, {1, 1, FALSE}};
    digraph6TestArc const twoLoops[] = {{0, 1, FALSE}, {1, 1, FALSE}, {1, 1, FALSE}};
    struct
    {
        digraph6TestArc const *arcs;
        int numArcs;
        int expectedIncluding, expectedExcluding;
    } const cases[] = {
        {path, 2, FALSE, FALSE},
        {directedPath, 2, FALSE, FALSE},
        {parallelUndirected, 3, TRUE, TRUE},
        {parallelOpposite, 3, TRUE, FALSE},
        {parallelSameWay, 3, TRUE, TRUE},
        {parallelMixed, 3, TRUE, TRUE},
        {parallelMixedBackward, 3, TRUE, TRUE},
        {oneLoop, 2, FALSE, FALSE},
        {twoLoops, 3, TRUE, TRUE},
    };
    graphP theGraph = NULL;
    int Result = OK;

    if (gp_HasParallelEdges(NULL, TRUE) != FALSE || gp_HasParallelEdges(NULL, FALSE) != FALSE)
    {
        gp_ErrorMessage("gp_HasParallelEdges(NULL) did not return FALSE.");
        return NOTOK;
    }

    for (size_t i = 0; Result == OK && i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        if ((theGraph = makeDigraph6TestGraph(3, cases[i].arcs, cases[i].numArcs)) == NULL)
            return NOTOK;

        // Each mode is called with every visited flag set and must leave
        // them all clear, also after an early TRUE
        for (int m = 0; Result == OK && m < 2; m++)
        {
            int const includingInOnlyEdges = (m == 0) ? TRUE : FALSE;
            int const expected = (m == 0) ? cases[i].expectedIncluding : cases[i].expectedExcluding;
            int got = FALSE;

            for (int v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); v++)
                gp_SetVisited(theGraph, v);

            if ((got = gp_HasParallelEdges(theGraph, includingInOnlyEdges)) != expected)
            {
                gp_ErrorMessage("gp_HasParallelEdges() with includingInOnlyEdges %d returned %d for case %d, want %d.",
                                includingInOnlyEdges, got, (int)i, expected);
                Result = NOTOK;
            }

            for (int v = gp_LowerBoundVertices(theGraph); Result == OK && v < gp_UpperBoundVertices(theGraph); v++)
            {
                if (gp_GetVisited(theGraph, v))
                {
                    gp_ErrorMessage("gp_HasParallelEdges() with includingInOnlyEdges %d left vertex %d visited in case %d.",
                                    includingInOnlyEdges, v, (int)i);
                    Result = NOTOK;
                }
            }
        }

        gp_Free(&theGraph);
    }

    return Result;
}

/****************************************************************************
 testPetersenDigraph6()

 Reads the Petersen digraph, whose every edge is directed, writes it as
 digraph6 and reads that back, and checks that the two graphs have the
 same edges with the same directions.
 ****************************************************************************/

int testPetersenDigraph6(void)
{
    graphP theGraph = gp_New();
    graphP theReadBackGraph = gp_New();
    char const *inputFileName = NULL;
    char *d6Str = NULL;
    int Result = OK;

    gp_Message("Testing the digraph6 write and read of the Petersen digraph");

#ifdef USE_1BASEDARRAYS
    inputFileName = "Petersen.digraph.txt";
#else
    inputFileName = "Petersen.digraph.0-based.txt";
#endif

    if (theGraph == NULL || theReadBackGraph == NULL)
        Result = NOTOK;
    else if (gp_Read(theGraph, inputFileName) != OK ||
             gp_WriteToString(theGraph, &d6Str, WRITE_DIGRAPH6) != OK ||
             gp_ReadFromString(theReadBackGraph, d6Str) != OK)
    {
        gp_ErrorMessage("Unable to write the Petersen digraph as digraph6 and read it back.");
        Result = NOTOK;
    }
    else if (gp_GetM(theGraph) != 15 || compareDigraph6Arcs(theGraph, theReadBackGraph, "Petersen digraph") != OK)
        Result = NOTOK;

    if (d6Str != NULL)
        free(d6Str);
    gp_Free(&theGraph);
    gp_Free(&theReadBackGraph);

    return Result;
}

/****************************************************************************
 testDigraph6Iterators()

 Reads each graph of n8.mALL.g6, directs every edge from its lower to its
 higher vertex and writes the graph with the digraph6 write iterator. Then
 reads the digraph6 output with the digraph6 read iterator, checks each
 graph's edge count and directions, and embeds a copy of it, checking the
 result's integrity. Planarity ignores the directions, so the counts are
 those of the undirected graphs.
 ****************************************************************************/

int testDigraph6Iterators(void)
{
    graphP theGraph = gp_New();
    G6ReadIteratorP theG6Reader = NULL;
    D6WriteIteratorP theWriter = NULL;
    D6ReadIteratorP theD6Reader = NULL;
    graphP theEmbedding = NULL;
    char *d6Str = NULL;
    int *numEdges = NULL;
    int numGraphs = 0, numRead = 0, numOK = 0, numNonembeddable = 0;
    int Result = OK;

    gp_Message("Testing the digraph6 read and write iterators on n8.mALL.g6");

    if (theGraph == NULL || (numEdges = (int *)malloc(12346 * sizeof(int))) == NULL ||
        g6_NewReader(&theG6Reader, theGraph) != OK ||
        g6_InitReaderWithFileName(theG6Reader, "n8.mALL.g6") != OK)
        Result = NOTOK;

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6Reader) != OK)
            Result = NOTOK;
        else if (g6_EndReached(theG6Reader))
            break;
        else if (numGraphs == 12346)
            Result = NOTOK;
        else
        {
            for (int v = gp_LowerBoundVertices(theGraph); v < gp_UpperBoundVertices(theGraph); v++)
            {
                for (int e = gp_GetFirstEdge(theGraph, v); gp_IsEdge(theGraph, e); e = gp_GetNextEdge(theGraph, e))
                {
                    if (gp_GetNeighbor(theGraph, e) > v)
                    {
                        gp_SetDirection(theGraph, e, EDGEFLAG_DIRECTION_OUTONLY);
                    }
                }
            }

            numEdges[numGraphs++] = gp_GetM(theGraph);

            if (theWriter == NULL &&
                (d6_NewWriter(&theWriter, theGraph) != OK ||
                 d6_InitWriterWithString(theWriter, &d6Str) != OK))
                Result = NOTOK;
            else if (d6_WriteGraph(theWriter) != OK)
                Result = NOTOK;
        }
    }

    d6_FreeWriter(&theWriter);
    g6_FreeReader(&theG6Reader);
    gp_Free(&theGraph);

    if (Result == OK && ((theGraph = gp_New()) == NULL ||
                         d6_NewReader(&theD6Reader, theGraph) != OK ||
                         d6_InitReaderWithString(theD6Reader, d6Str) != OK))
        Result = NOTOK;

    while (Result == OK)
    {
        if (d6_ReadGraph(theD6Reader) != OK)
            Result = NOTOK;
        else if (d6_EndReached(theD6Reader))
            break;
        else if (numRead == numGraphs || gp_GetM(theGraph) != numEdges[numRead] ||
                 ((gp_GetGraphFlags(theGraph) & GRAPHFLAGS_DIRECTEDEDGEDETECTED) ? 1 : 0) != (numEdges[numRead] > 0 ? 1 : 0))
        {
            gp_ErrorMessage("Graph %d read from digraph6 has the wrong edge count or directed edge flag.", numRead + 1);
            Result = NOTOK;
        }
        else
        {
            for (int v = gp_LowerBoundVertices(theGraph); Result == OK && v < gp_UpperBoundVertices(theGraph); v++)
            {
                for (int e = gp_GetFirstEdge(theGraph, v); gp_IsEdge(theGraph, e); e = gp_GetNextEdge(theGraph, e))
                {
                    unsigned expectedDirection = gp_GetNeighbor(theGraph, e) > v ? EDGEFLAG_DIRECTION_OUTONLY : EDGEFLAG_DIRECTION_INONLY;

                    if (gp_GetDirection(theGraph, e) != expectedDirection)
                    {
                        gp_ErrorMessage("Graph %d read from digraph6 has an edge with the wrong direction.", numRead + 1);
                        Result = NOTOK;
                        break;
                    }
                }
            }

            if (Result == OK && (theEmbedding = gp_DupGraph(theGraph)) == NULL)
                Result = NOTOK;

            if (Result == OK)
            {
                int embedResult = gp_Embed(theEmbedding, EMBEDFLAGS_PLANAR);

                if ((embedResult != OK && embedResult != NONEMBEDDABLE) ||
                    gp_TestEmbedResultIntegrity(theEmbedding, theGraph, embedResult) != embedResult)
                {
                    gp_ErrorMessage("Embedding graph %d read from digraph6 failed.", numRead + 1);
                    Result = NOTOK;
                }
                else if (embedResult == OK)
                    numOK++;
                else
                    numNonembeddable++;
            }

            gp_Free(&theEmbedding);
            numRead++;
        }
    }

    if (Result == OK && (numGraphs != 12346 || numRead != numGraphs || numOK != 6966 || numNonembeddable != 5380))
    {
        gp_ErrorMessage("Expected 12346 graphs, 6966 planar and 5380 not, but "
                        "wrote %d and read %d, of which %d planar and %d not.",
                        numGraphs, numRead, numOK, numNonembeddable);
        Result = NOTOK;
    }

    d6_FreeReader(&theD6Reader);
    gp_Free(&theGraph);
    if (d6Str != NULL)
        free(d6Str);
    if (numEdges != NULL)
        free(numEdges);

    return Result;
}

/****************************************************************************
 runDigraph6Tests()

 Tests reading and writing digraph6, gp_HasParallelEdges(), the samples
 that nauty wrote, the transformations to and from digraph6, and the
 algorithms on all the oriented graphs of order 6 and all the digraphs of
 order 5, which they must treat as the undirected graphs beneath them.
 ****************************************************************************/

int runDigraph6Tests(void)
{
    int Result = OK;

    gp_Message("Starting digraph6 tests");

    if (runDigraph6ReadTests() != OK)
        Result = NOTOK;
    else if (runDigraph6InitRetryTests() != OK)
        Result = NOTOK;
    else if (runDigraph6WriteTests() != OK)
        Result = NOTOK;
    else if (runHasParallelEdgesTests() != OK)
    {
        gp_ErrorMessage("gp_HasParallelEdges() tests failed.");
        Result = NOTOK;
    }
    // N4-all.d6 and N5-digraphs.d6 are every digraph of order 4 and of order
    // 5 as nauty's directg wrote them, so with 2-cycles, which read as
    // undirected edges; N6-oriented.d6 is every oriented graph of order 6
    // (directg -o); N5-all.d6 is nauty's copyg -z of N5-all.g6
    else if (runDigraph6FileTest("N4-all.d6", "N4-all.d6", 218) != OK ||
             runDigraph6FileTest("N5-digraphs.d6", "N5-digraphs.d6", 9608) != OK ||
             runDigraph6FileTest("N6-oriented.d6", "N6-oriented.d6", 21480) != OK ||
             runDigraph6FileTest("N5-all.g6", "N5-all.d6", 34) != OK)
        Result = NOTOK;
    else if (runGraphTransformationTest("-d", "nauty_example.g6", TRUE) != OK ||
             runGraphTransformationTest("-d", "nauty_example.g6", FALSE) != OK ||
             runGraphTransformationTest("-d", "N5-all.g6", FALSE) != OK ||
             runGraphTransformationTest("-d", "Petersen.digraph.txt", TRUE) != OK ||
             runGraphTransformationTest("-d", "Petersen.digraph.0-based.txt", FALSE) != OK ||
             runGraphTransformationTest("-d", "nauty_example.d6", TRUE) != OK ||
             runGraphTransformationTest("-d", "nauty_example.d6", FALSE) != OK ||
             runGraphTransformationTest("-a", "nauty_example.d6", TRUE) != OK ||
             runGraphTransformationTest("-a", "nauty_example.d6", FALSE) != OK)
        Result = NOTOK;
    // The expected counts are those of the 156 graphs of order 6, each
    // weighted by the number of its orientations in N6-oriented.d6
    else if (runTestAllGraphsTest("-p", "N6-oriented.d6", "-p 21480 17212 4268 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-d", "N6-oriented.d6", "-d 21480 17212 4268 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-o", "N6-oriented.d6", "-o 21480 3724 17756 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-2", "N6-oriented.d6", "-2 21480 3952 17528 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-3", "N6-oriented.d6", "-3 21480 17860 3620 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-4", "N6-oriented.d6", "-4 21480 4662 16818 SUCCESS", NULL) != OK)
    {
        gp_ErrorMessage("Testing the algorithms on N6-oriented.d6 failed.");
        Result = NOTOK;
    }
    // Likewise for the 34 graphs of order 5 weighted by the number of their
    // digraphs in N5-digraphs.d6, whose 2-cycles read as undirected edges
    else if (runTestAllGraphsTest("-p", "N5-digraphs.d6", "-p 9608 9026 582 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-d", "N5-digraphs.d6", "-d 9608 9026 582 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-o", "N5-digraphs.d6", "-o 9608 3211 6397 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-2", "N5-digraphs.d6", "-2 9608 3667 5941 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-3", "N5-digraphs.d6", "-3 9608 9608 0 SUCCESS", NULL) != OK ||
             runTestAllGraphsTest("-4", "N5-digraphs.d6", "-4 9608 3560 6048 SUCCESS", NULL) != OK)
    {
        gp_ErrorMessage("Testing the algorithms on N5-digraphs.d6 failed.");
        Result = NOTOK;
    }

    if (Result == OK)
        gp_Message("Finished digraph6 tests.\n");

    return Result;
}
