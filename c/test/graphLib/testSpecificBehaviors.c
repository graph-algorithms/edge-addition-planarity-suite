/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"
#include "../../graphLib/io/strOrFile.h"

/* Imported functions */
/* Library-private functions called directl by runCompactEdgeStorageTests() */
extern int _CompactEdgeStorage(graphP theGraph);

/* Private function declarations (exported within system) */
int runSpecificBehaviorsTests(void);

/* Private functions */
static int runFaceListTest(void);
static int runAddInsertEdgeTests(void);
static int runReadWithExtensionAtEofTest(void);
static int runHighByteRoundTripTest(void);
static int runCapacityLimitTests(void);
static int runCompactEdgeStorageTests(void);

/****************************************************************************
 runSpecificBehaviorsTests()
 ****************************************************************************/

int runSpecificBehaviorsTests(void)
{
    int Result = OK;

    gp_Message("Starting specific behaviors tests");

    if (runFaceListTest() != OK)
        Result = NOTOK;
    else if (runAddInsertEdgeTests() != OK)
        Result = NOTOK;
    else if (runReadWithExtensionAtEofTest() != OK)
        Result = NOTOK;
    else if (runHighByteRoundTripTest() != OK)
        Result = NOTOK;
    else if (runCapacityLimitTests() != OK)
        Result = NOTOK;
    else if (runCompactEdgeStorageTests() != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Finished specific behaviors tests.\n");
    else
        gp_Message("Error specific behaviors tests.\n");

    return Result;
}

/****************************************************************************
 runFaceListTest()
 ****************************************************************************/

int runFaceListTest(void)
{
    graphP theGraph = NULL, origGraph = NULL;
    char *faceList = NULL, *drawing = NULL;
    char const *infileName = NULL, *expectedOutfileName = NULL, *expectedDrawingFileName = NULL;
    int embedResult, retVal = OK;

    infileName = "faceListComponents.txt";
    expectedOutfileName = "faceListComponents.out.txt";
    expectedDrawingFileName = "faceListComponents.Drawing.txt";

    gp_Message("Starting Face List Test");

    if ((theGraph = gp_New()) == NULL ||
        gp_Read(theGraph, infileName) != OK ||
        (origGraph = gp_DupGraph(theGraph)) == NULL ||
        gp_ExtendWith_DrawPlanar(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to set up the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    embedResult = gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR);
    if (embedResult != OK ||
        gp_TestEmbedResultIntegrity(theGraph, origGraph, embedResult) != OK)
    {
        gp_ErrorMessage("Unable to embed the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_SortVertices(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to restore original vertex labelling.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CountEmbeddingFaces(theGraph) != 7)
    {
        gp_ErrorMessage("Unexpected face count for the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CreateEmbeddingFaceList(theGraph, &faceList) != OK || faceList == NULL)
    {
        gp_ErrorMessage("Unable to create the face list sample output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (TextFileMatchesString(expectedOutfileName, faceList) != TRUE)
    {
        gp_ErrorMessage("Face list sample output did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_DrawPlanar_RenderToString(theGraph, &drawing) != OK || drawing == NULL ||
        TextFileMatchesString(expectedDrawingFileName, drawing) != TRUE)
    {
        gp_ErrorMessage("Face list sample drawing did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    gp_Message("Finished Face List Test.\n");

runFaceListTest_Cleanup:

    if (drawing != NULL)
    {
        free(drawing);
        drawing = NULL;
    }

    if (faceList != NULL)
    {
        free(faceList);
        faceList = NULL;
    }

    gp_Free(&origGraph);
    gp_Free(&theGraph);

    return retVal;
}

/****************************************************************************
 runAddInsertEdgeTests()
 ****************************************************************************/

int runAddInsertEdgeTests(void)
{
    graphP theGraph = NULL;
    int initialEdgeCapacity;
    int edgeToDelete;
    int v;

    gp_Message("Starting Add/Insert Edge Tests");

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for add/insert edge tests.");
        return NOTOK;
    }

    if (gp_EnsureEdgeCapacity(theGraph, 2) != OK ||
        gp_EnsureVertexCapacity(theGraph, 3) != OK)
    {
        gp_ErrorMessage("Unable to initialize graph for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    v = gp_LowerBoundVertices(theGraph);
    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);

    if (gp_AddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_AddEdge(theGraph, v + 1, 1, v + 2, 0) != OK)
    {
        gp_ErrorMessage("Unable to fill initial edge capacity.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_InsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != AT_EDGE_CAPACITY_LIMIT)
    {
        gp_ErrorMessage("gp_InsertEdge() did not report the edge capacity limit.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_DynamicInsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != OK ||
        gp_GetEdgeCapacity(theGraph) <= initialEdgeCapacity ||
        gp_GetM(theGraph) != 3)
    {
        gp_ErrorMessage("gp_DynamicInsertEdge() did not grow edge capacity and insert once.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    edgeToDelete = gp_FindEdge(theGraph, v, v + 1);
    if (edgeToDelete == NIL || gp_DeleteEdge(theGraph, edgeToDelete) != OK)
    {
        gp_ErrorMessage("Unable to create an edge hole for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);
    if (gp_DynamicAddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_GetEdgeCapacity(theGraph) != initialEdgeCapacity ||
        gp_GetM(theGraph) != 3 ||
        gp_FindEdge(theGraph, v, v + 1) == NIL)
    {
        gp_ErrorMessage("gp_DynamicAddEdge() did not reuse an available edge hole.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    gp_Message("Finished Add/Insert Edge Tests.\n");

    gp_Free(&theGraph);

    return OK;
}

/********************************************************************
 runReadWithExtensionAtEofTest()

 Reads a graph with an extension attached before gp_Read(), which
 exercises the end-of-input handling in _ReadGraph(): the extra-data
 check must see a true EOF rather than a fabricated byte.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is unsigned, (char)EOF == 255, so this
 read handed a spurious 0xFF extra-data byte to the extension's
 post-processor and failed on a valid input file.
 ********************************************************************/

int runReadWithExtensionAtEofTest(void)
{
    int Result = OK;
    graphP theGraph = NULL;

    gp_Message("Test EOF Handling (for platforms that have char unsigned).");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_ExtendWith_DrawPlanar(theGraph) != OK)
        Result = NOTOK;

    if (Result == OK && gp_Read(theGraph, "maxPlanar5.txt") != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    gp_Free(&theGraph);

    return Result;
}

/********************************************************************
 runHighByteRoundTripTest()

 Reads the byte 0xFF from a string container directly and through the
 unget buffer, and confirms it is never mistaken for EOF.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is signed, a literal 0xFF byte fetched
 from the string container sign-extended to EOF, and the same
 happened to bytes round-tripped through the unget buffer.
 ********************************************************************/

int runHighByteRoundTripTest(void)
{
    int Result = OK;
    int currChar = EOF;
    unsigned char eofChar = 0xFF;
    char highByteStr[4];
    // char const highByteStr[] = {'a', (char)0xFF, 'b', '\0'};
    strOrFileP inputContainer = NULL;

    gp_Message("Test Support of 0xFF Byte.");

    highByteStr[0] = 'a';
    highByteStr[1] = (char)eofChar;
    highByteStr[2] = 'b';
    highByteStr[3] = '\0';

    if ((inputContainer = sf_NewInputContainer(highByteStr, NULL)) == NULL)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'a')
        Result = NOTOK;

    if (Result == OK)
    {
        currChar = sf_getc(inputContainer);
        if (currChar != 0xFF)
            Result = NOTOK;
    }

    if (Result == OK && sf_ungetc(currChar, inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'b')
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != EOF)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    if (inputContainer != NULL)
        sf_Free(&inputContainer);

    return Result;
}

/********************************************************************
 runCapacityLimitTests()

 Exercises the issue #325 guards: a vertex count or edge capacity
 whose capacity arithmetic cannot be represented in int must be
 rejected by the public wrappers before any allocation is attempted.

 The first rejected vertex count on the default edge capacity is
 (INT_MAX - 2) / 4 / DEFAULT_EDGE_CAPACITY_FACTOR + 1, and the first
 rejected edge capacity is (INT_MAX - 2) / 4 + 1; the largest
 acceptable edge capacity is tested on the accept side too, which is
 safe because storing a capacity before gp_EnsureVertexCapacity()
 allocates nothing.
 ********************************************************************/

int runCapacityLimitTests(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    int firstBadN = (int)((((long long)INT_MAX - 2) / 4) / DEFAULT_EDGE_CAPACITY_FACTOR + 1);
    int maxGoodEdgeCapacity = (int)(((long long)INT_MAX - 2) / 4);
    unsigned quietModeCache = gp_GetQuietMode();

    gp_Message("Testing rejection of excessive vertex and edge capacity requests.");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    // The first tests below intentionally cause a NOTOK to come from the APIs,
    // so we set fully quiet mode to prevent error messages on expected behaviors.
    gp_SetQuietMode(QUIETMODE_ALL);

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, INT_MAX) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, firstBadN) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity + 1) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    // The end of tests intended to produce NOTOK has been reached, so we can go back
    // to normal quiet mode behavior.
    gp_SetQuietMode(quietModeCache);

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity) != OK)
        Result = NOTOK;

    gp_Free(&theGraph);

    // The guards must not disturb ordinary use
    if (Result == OK)
    {
        if ((theGraph = gp_New()) == NULL)
            Result = NOTOK;

        if (Result == OK && gp_EnsureVertexCapacity(theGraph, 5) != OK)
            Result = NOTOK;

        gp_Free(&theGraph);
    }

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    return Result;
}

/****************************************************************************
 runCompactEdgeStorageTests()

 Deletes edges of K5 so that holes lie in the middle and at the end of the
 edge storage, recorded by either record of their pairs, and checks that
 _CompactEdgeStorage() removes every hole while keeping the edge set, the
 edge count, the flags of the moved edge and the order of every adjacency
 list, which a depth first search done before the compaction relies on.
 A loop in the last pair is moved too, to the front of its list, and an
 edge to be moved that is hidden makes the compaction refuse.
 ****************************************************************************/

// Writes the neighbors of each vertex of a graph of order 5, in adjacency
// list order, to five rows filled out with NIL
static void getNeighborOrder(graphP theGraph, int order[5][5])
{
    int lower = gp_LowerBoundVertexStorage(theGraph);

    for (int v = 0; v < 5; v++)
    {
        int i = 0;

        for (int e = gp_GetFirstEdge(theGraph, lower + v); gp_IsEdge(theGraph, e) && i < 4;
             e = gp_GetNextEdge(theGraph, e))
            order[v][i++] = gp_GetNeighbor(theGraph, e);
        while (i < 5)
            order[v][i++] = NIL;
    }
}

int runCompactEdgeStorageTests(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    char *before = NULL, *after = NULL;
    int lower = 0;
    int eLast = NIL, uLast = NIL, vLast = NIL;
    int orderBefore[5][5], orderAfter[5][5];
    unsigned origQuietMode = gp_GetQuietMode();

    if ((theGraph = gp_New()) == NULL || gp_EnsureVertexCapacity(theGraph, 5) != OK)
    {
        gp_Free(&theGraph);
        return NOTOK;
    }

    lower = gp_LowerBoundVertexStorage(theGraph);

    // Each edge goes to the front of the list of its lesser endpoint and to
    // the end of the other list, so that each edge the compaction moves is
    // first in one list, ahead of another edge, and last in the other
    for (int u = 0; Result == OK && u < 5; u++)
        for (int v = u + 1; Result == OK && v < 5; v++)
            if (gp_AddEdge(theGraph, lower + u, 0, lower + v, 1) != OK)
                Result = NOTOK;

    // Delete the pair before the last by its odd record and then the last
    // pair, which leaves a hole as the last pair of the storage in use, then
    // a middle pair by its odd record and another by its even record
    if (Result == OK &&
        (gp_DeleteEdge(theGraph, gp_GetTwin(theGraph, gp_FindEdge(theGraph, lower + 2, lower + 4))) != OK ||
         gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 3, lower + 4)) != OK ||
         gp_DeleteEdge(theGraph, gp_GetTwin(theGraph, gp_FindEdge(theGraph, lower + 1, lower + 2))) != OK ||
         gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 0, lower + 3)) != OK))
        Result = NOTOK;

    if (Result == OK &&
        (theGraph->numEdgeHoles != 3 || gp_EdgeInUse(theGraph, gp_UpperBoundEdges(theGraph) - 2)))
    {
        gp_ErrorMessage("The deletions did not leave three edge holes, the last "
                        "pair being one.");
        Result = NOTOK;
    }

    // The edge set before compaction, taken while the graph is undirected,
    // since setting a direction below marks it as a digraph for graph6
    if (Result == OK && gp_WriteToString(theGraph, &before, WRITE_GRAPH6) != OK)
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
        getNeighborOrder(theGraph, orderBefore);
    }

    if (Result == OK && _CompactEdgeStorage(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to compact the edge storage.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        getNeighborOrder(theGraph, orderAfter);
        if (memcmp(orderBefore, orderAfter, sizeof(orderBefore)) != 0)
        {
            gp_ErrorMessage("Compaction changed the order of an adjacency list.");
            Result = NOTOK;
        }
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
    if (Result == OK && gp_WriteToString(theGraph, &after, WRITE_GRAPH6) != OK)
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

    // Test edge deletion and compaction of loop edges
    if (Result == OK)
    {
        int eLoop = NIL, eFirst = NIL;

        gp_ResetGraphStorage(theGraph);

        if (gp_AddEdge(theGraph, lower, 0, lower + 1, 0) != OK ||
            gp_AddEdge(theGraph, lower + 1, 0, lower + 2, 0) != OK ||
            gp_AddEdge(theGraph, lower + 2, 0, lower + 2, 0) != OK)
            Result = NOTOK;

        // Try deleting loop edge when it's the last added edge
        if (Result == OK)
        {
            if (gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 2, lower + 2)) != OK ||
                gp_GetVertexDegree(theGraph, lower) != 1 ||
                gp_GetVertexDegree(theGraph, lower + 1) != 2 ||
                gp_GetVertexDegree(theGraph, lower + 2) != 1 ||
                theGraph->numEdgeHoles != 0 || gp_GetM(theGraph) != 2 ||
                gp_AddEdge(theGraph, lower + 2, 0, lower + 2, 0) != OK)
            {
                Result = NOTOK;
            }
        }

        // Try deleting loop edge when it's not the last added edge
        if (Result == OK)
        {
            if (gp_AddEdge(theGraph, lower, 0, lower + 2, 0) != OK ||
                gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 2, lower + 2)) != OK ||
                gp_GetVertexDegree(theGraph, lower) != 2 ||
                gp_GetVertexDegree(theGraph, lower + 1) != 2 ||
                gp_GetVertexDegree(theGraph, lower + 2) != 2 ||
                theGraph->numEdgeHoles != 1 || gp_GetM(theGraph) != 3)
            {
                Result = NOTOK;
            }

            if (Result == OK)
            {
                if (_CompactEdgeStorage(theGraph) != OK ||
                    theGraph->numEdgeHoles != 0 || gp_GetM(theGraph) != 3 ||
                    gp_GetVertexDegree(theGraph, lower) != 2 ||
                    gp_GetVertexDegree(theGraph, lower + 1) != 2 ||
                    gp_GetVertexDegree(theGraph, lower + 2) != 2 ||
                    gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower, lower + 2)) != OK ||
                    gp_AddEdge(theGraph, lower + 2, 0, lower + 2, 0) != OK)
                {
                    Result = NOTOK;
                }
            }
        }

        // Make the loop edge directed
        if (Result == OK)
        {
            eLoop = gp_FindEdge(theGraph, lower + 2, lower + 2);
            theGraph->E[eLoop].flags |= EDGEFLAG_DIRECTION_OUTONLY;
            theGraph->E[gp_GetTwin(theGraph, eLoop)].flags |= EDGEFLAG_DIRECTION_INONLY;
            eFirst = gp_LowerBoundEdges(theGraph);
        }

        // A loop in the last pair, behind the hole of the first, is moved into
        // that hole and stays a loop, with the flags of both its records
        if (Result == OK &&
            (gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower, lower + 1)) != OK ||
             _CompactEdgeStorage(theGraph) != OK ||
             theGraph->numEdgeHoles != 0 || gp_GetM(theGraph) != 2 ||
             gp_GetNeighbor(theGraph, eFirst) != lower + 2 ||
             gp_GetNeighbor(theGraph, eFirst + 1) != lower + 2 ||
             gp_GetVertexDegree(theGraph, lower + 2) != 3 ||
             (gp_GetDirection(theGraph, eFirst) | gp_GetDirection(theGraph, eFirst + 1)) !=
                 (EDGEFLAG_DIRECTION_OUTONLY | EDGEFLAG_DIRECTION_INONLY)))
        {
            gp_ErrorMessage("Compaction of a loop gave the wrong result.");
            Result = NOTOK;
        }
    }

    // An edge to be moved that is hidden makes the compaction refuse,
    // changing nothing, and so the planar drawing too; once the edge is
    // restored, the compaction goes ahead
    if (Result == OK)
    {
        int eHidden = NIL;

        gp_ResetGraphStorage(theGraph);

        if (gp_AddEdge(theGraph, lower, 0, lower + 1, 0) != OK ||
            gp_AddEdge(theGraph, lower + 1, 0, lower + 2, 0) != OK ||
            gp_AddEdge(theGraph, lower + 2, 0, lower + 3, 0) != OK ||
            gp_DeleteEdge(theGraph, gp_FindEdge(theGraph, lower + 1, lower + 2)) != OK ||
            gp_ExtendWith_DrawPlanar(theGraph) != OK)
            Result = NOTOK;

        if (Result == OK)
        {
            eHidden = gp_FindEdge(theGraph, lower + 2, lower + 3);
            gp_HideEdge(theGraph, eHidden);

            // Set all to quiet due to intended failure call
            gp_SetQuietMode(QUIETMODE_ALL);

            if (_CompactEdgeStorage(theGraph) == OK || gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR) != NOTOK ||
                theGraph->numEdgeHoles != 1 || gp_UpperBoundEdges(theGraph) != gp_LowerBoundEdges(theGraph) + 6 ||
                gp_GetEmbedFlags(theGraph) != 0)
            {
                gp_ErrorMessage("Compaction moved a hidden edge.");
                Result = NOTOK;
            }

            gp_SetQuietMode(origQuietMode);

            gp_RestoreEdge(theGraph, eHidden);
        }

        if (Result == OK && (_CompactEdgeStorage(theGraph) != OK || theGraph->numEdgeHoles != 0 ||
                             gp_GetVertexDegree(theGraph, lower + 3) != 1))
        {
            gp_ErrorMessage("Compaction after restoring a hidden edge gave the wrong result.");
            Result = NOTOK;
        }
    }

    if (before != NULL)
        free(before);
    if (after != NULL)
        free(after);
    gp_Free(&theGraph);

    return Result;
}
