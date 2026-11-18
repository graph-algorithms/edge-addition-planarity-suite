/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Imported functions */

/* Private function declarations (exported within system) */
int runHideRestoreIdentifyContractTests(void);

/* Private functions */
static int runHideRestoreTests(void);
static int runHideRestoreTest(graphP theGraph);

static int runIdentifyContractTests(void);
static int runIdentifyContractTest(graphP theGraph);

/****************************************************************************
 runHideRestoreIdentifyContractTests()
 ****************************************************************************/

int runHideRestoreIdentifyContractTests(void)
{
    int Result = OK;

    gp_Message("Starting hide, restore, identify, and contract tests");

    if (runHideRestoreTests() != OK)
        Result = NOTOK;
    else if (runIdentifyContractTests() != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Finished hide, restore, identify, and contract tests.\n");
    else
        gp_Message("Error detected in hide, restore, identify, and contract tests.\n");

    return Result;
}

/****************************************************************************
 runHideRestoreTests()
 ****************************************************************************/

int runHideRestoreTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Hide/Restore Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for hide/restore tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for hide/restore tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for hide/restore tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runHideRestoreTest(theGraph) != OK)
        {
            gp_ErrorMessage("Hide/restore test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Hide/Restore Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runHideRestoreTest()
 ****************************************************************************/

int runHideRestoreTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int v;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runHideRestoreTest() received NULL graph.");
        return NOTOK;
    }

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before hide/restore test.");
        Result = NOTOK;
    }

    for (v = gp_LowerBoundVertices(theGraph); Result == OK && v < gp_UpperBoundVertices(theGraph); ++v)
    {
        if (gp_HideVertex(theGraph, v) != OK)
        {
            gp_ErrorMessage("gp_HideVertex() failed during hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during hide/restore test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after hide/restore test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}

/****************************************************************************
 runIdentifyContractTests()
 ****************************************************************************/

int runIdentifyContractTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Identify/Contract Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for identify/contract tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for identify/contract tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for identify/contract tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runIdentifyContractTest(theGraph) != OK)
        {
            gp_ErrorMessage("Identify/contract test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Identify/Contract Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

/****************************************************************************
 runIdentifyContractTest()
 ****************************************************************************/

int runIdentifyContractTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int vertexOffset;
    int pairs[7][2];
    int i;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runIdentifyContractTest() received NULL graph.");
        return NOTOK;
    }

    vertexOffset = gp_LowerBoundVertices(theGraph);

    pairs[0][0] = vertexOffset;
    pairs[0][1] = vertexOffset + 1;
    pairs[1][0] = vertexOffset + 2;
    pairs[1][1] = vertexOffset + 3;
    pairs[2][0] = vertexOffset + 4;
    pairs[2][1] = vertexOffset + 5;
    pairs[3][0] = vertexOffset + 6;
    pairs[3][1] = vertexOffset + 7;
    pairs[4][0] = vertexOffset;
    pairs[4][1] = vertexOffset + 2;
    pairs[5][0] = vertexOffset + 4;
    pairs[5][1] = vertexOffset + 6;
    pairs[6][0] = vertexOffset;
    pairs[6][1] = vertexOffset + 4;

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before identify/contract test.");
        Result = NOTOK;
    }

    for (i = 0; Result == OK && i < 7; i++)
    {
        if (gp_IdentifyVertices(theGraph, pairs[i][0], pairs[i][1], NIL) != OK)
        {
            gp_ErrorMessage("gp_IdentifyVertices() failed during identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during identify/contract test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after identify/contract test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}
