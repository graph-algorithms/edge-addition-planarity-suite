/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <string.h>

#include "../planarity.h"
#include "../../graphLib/planarityRelated/graphDrawPlanar.private.h"
#include "../../graphLib/homeomorphSearch/graphK33Search.private.h"
#include "../../graphLib/homeomorphSearch/graphK4Search.private.h"
#include "../../graphLib/extensionSystem/graphExtensions.h"

int runExtensionCopyTests(void);

/****************************************************************************
 The extensions that keep data per edge record, with the means to reach
 that data, so that one test covers them all
 ****************************************************************************/

static char *getDrawPlanarEdgeData(void *context)
{
    return (char *)((DrawPlanarContext *)context)->E;
}

static char *getK33SearchEdgeData(void *context)
{
    return (char *)((K33SearchContext *)context)->E;
}

static char *getK4SearchEdgeData(void *context)
{
    return (char *)((K4SearchContext *)context)->E;
}

typedef struct
{
    char const *name;
    int (*extendWith)(graphP theGraph);
    int *pID;
    char *(*getEdgeData)(void *context);
    size_t recordSize;
} EdgeDataExtension;

// Returns the start of the edge data of the extension in theGraph, or NULL
static char *findEdgeData(graphP theGraph, EdgeDataExtension const *ext)
{
    void *context = NULL;

    gp_FindExtension(theGraph, *ext->pID, &context);

    return context == NULL ? NULL : ext->getEdgeData(context);
}

// Makes a graph of the given order and edge capacity, or the default one
// for 0, extended with the extension
static graphP newExtendedGraph(EdgeDataExtension const *ext, int order, int edgeCapacity)
{
    graphP theGraph = gp_New();

    if (theGraph == NULL ||
        (edgeCapacity > 0 && gp_EnsureEdgeCapacity(theGraph, edgeCapacity) != OK) ||
        gp_EnsureVertexCapacity(theGraph, order) != OK || ext->extendWith(theGraph) != OK)
        gp_Free(&theGraph);

    return theGraph;
}

// Fills the edge data of the extension in theGraph with a byte value
static int fillEdgeData(graphP theGraph, EdgeDataExtension const *ext, int value)
{
    char *data = findEdgeData(theGraph, ext);

    if (data == NULL)
        return NOTOK;

    memset(data, value, (size_t)gp_UpperBoundEdgeStorage(theGraph) * ext->recordSize);
    return OK;
}

// The copy must hold the source's edge data, filled with SOURCE_FILL, in the
// source's edge records, and the edge data of a freshly extended graph of
// its own edge storage in any edge records beyond them
#define SOURCE_FILL 0x5A
#define DESTINATION_FILL 0xA5

static int checkCopiedEdgeData(graphP copy, int srcStorage, EdgeDataExtension const *ext, char const *how)
{
    char *copyData = findEdgeData(copy, ext);
    int copyStorage = gp_UpperBoundEdgeStorage(copy);
    int Result = OK;

    if (copyData == NULL || copyStorage < srcStorage)
    {
        gp_ErrorMessage("Unable to check the %s edge data %s.", ext->name, how);
        return NOTOK;
    }

    for (size_t i = 0; i < (size_t)srcStorage * ext->recordSize; i++)
    {
        if ((unsigned char)copyData[i] != SOURCE_FILL)
        {
            gp_ErrorMessage("The %s edge data %s differs from the source's.", ext->name, how);
            return NOTOK;
        }
    }

    if (copyStorage > srcStorage)
    {
        graphP fresh = newExtendedGraph(ext, gp_GetN(copy), gp_GetEdgeCapacity(copy));
        char *freshData = fresh != NULL ? findEdgeData(fresh, ext) : NULL;

        if (freshData == NULL || gp_UpperBoundEdgeStorage(fresh) != copyStorage)
        {
            gp_ErrorMessage("Unable to make a fresh %s graph to check the edge data %s.", ext->name, how);
            Result = NOTOK;
        }
        else if (memcmp(copyData + (size_t)srcStorage * ext->recordSize, freshData + (size_t)srcStorage * ext->recordSize,
                        (size_t)(copyStorage - srcStorage) * ext->recordSize) != 0)
        {
            gp_ErrorMessage("The %s edge data %s beyond the source's edge records is not "
                            "initialized.",
                            ext->name, how);
            Result = NOTOK;
        }

        gp_Free(&fresh);
    }

    return Result;
}

// Copies a graph of order 10 and edge capacity srcCapacity, extended, into
// one of edge capacity dstCapacity, extended too, whose edge data is filled
// beforehand so that records left as they were are noticed
static int testCopy(EdgeDataExtension const *ext, int srcCapacity, int dstCapacity, char const *how)
{
    int Result = OK;
    graphP src = newExtendedGraph(ext, 10, srcCapacity), dst = newExtendedGraph(ext, 10, dstCapacity);

    if (src == NULL || dst == NULL || fillEdgeData(src, ext, SOURCE_FILL) != OK ||
        fillEdgeData(dst, ext, DESTINATION_FILL) != OK)
    {
        gp_ErrorMessage("Unable to make the %s graphs to copy %s.", ext->name, how);
        Result = NOTOK;
    }
    else if (gp_CopyGraph(dst, src) != OK)
    {
        gp_ErrorMessage("Unable to copy the %s graph %s.", ext->name, how);
        Result = NOTOK;
    }
    else
        Result = checkCopiedEdgeData(dst, gp_UpperBoundEdgeStorage(src), ext, how);

    gp_Free(&src);
    gp_Free(&dst);

    return Result;
}

// Duplicates a graph of order 10 and edge capacity srcCapacity, extended
static int testDup(EdgeDataExtension const *ext, int srcCapacity, char const *how)
{
    int Result = OK;
    graphP src = newExtendedGraph(ext, 10, srcCapacity), dup = NULL;

    if (src == NULL || fillEdgeData(src, ext, SOURCE_FILL) != OK)
    {
        gp_ErrorMessage("Unable to make the %s graph to duplicate %s.", ext->name, how);
        Result = NOTOK;
    }
    else if ((dup = gp_DupGraph(src)) == NULL)
    {
        gp_ErrorMessage("Unable to duplicate the %s graph %s.", ext->name, how);
        Result = NOTOK;
    }
    else
        Result = checkCopiedEdgeData(dup, gp_UpperBoundEdgeStorage(src), ext, how);

    gp_Free(&src);
    gp_Free(&dup);

    return Result;
}

// gp_DupExtensions() into a graph of the given order and edge capacity must
// be refused, leaving that graph without the extension
static int testDupExtensionsRefused(EdgeDataExtension const *ext, int dstOrder, int dstCapacity, char const *how)
{
    int Result = OK;
    unsigned quietModeCache = gp_GetQuietMode();
    graphP src = newExtendedGraph(ext, 10, 1000), dst = gp_New();

    if (src == NULL || dst == NULL || gp_EnsureEdgeCapacity(dst, dstCapacity) != OK ||
        gp_EnsureVertexCapacity(dst, dstOrder) != OK)
    {
        gp_ErrorMessage("Unable to make the %s graphs to duplicate the extensions %s.", ext->name, how);
        Result = NOTOK;
    }
    else
    {
        gp_SetQuietMode(QUIETMODE_ALL);
        if (gp_DupExtensions(dst, src) == OK || findEdgeData(dst, ext) != NULL)
        {
            gp_SetQuietMode(quietModeCache);
            gp_ErrorMessage("The %s extension was duplicated %s.", ext->name, how);
            Result = NOTOK;
        }
        gp_SetQuietMode(quietModeCache);
    }

    gp_Free(&src);
    gp_Free(&dst);

    return Result;
}

/****************************************************************************
 runExtensionCopyTests()

 For each extension that keeps data per edge record: copying a graph into
 one with more or the same edge capacity, and duplicating a graph that has
 less or more edge capacity than the default for its order, must copy the
 edge data from the source's edge records only, and leave the edge data of
 any further edge records of the destination in its initial state.
 Duplicating the extensions into a graph of another order, or with less
 edge storage than the source, must be refused.
 ****************************************************************************/

int runExtensionCopyTests(void)
{
    int Result = OK;
    EdgeDataExtension const exts[] = {
        {"DrawPlanar", gp_ExtendWith_DrawPlanar, &DRAWPLANAR_ID, getDrawPlanarEdgeData, sizeof(DrawPlanar_EdgeRec)},
        {"K3,3 search", gp_ExtendWith_K33Search, &K33SEARCH_ID, getK33SearchEdgeData, sizeof(K33Search_EdgeRec)},
        {"K4 search", gp_ExtendWith_K4Search, &K4SEARCH_ID, getK4SearchEdgeData, sizeof(K4Search_EdgeRec)},
    };

    gp_Message("Testing the copy of extension edge data between graphs of different edge capacities.");

    for (size_t x = 0; Result == OK && x < sizeof(exts) / sizeof(exts[0]); x++)
    {
        EdgeDataExtension const *ext = &exts[x];

        if (testCopy(ext, 0, 1000, "into more edge capacity") != OK ||
            testCopy(ext, 1000, 1000, "into the same edge capacity") != OK ||
            testDup(ext, 5, "with less than the default edge capacity") != OK ||
            testDup(ext, 1000, "with more than the default edge capacity") != OK ||
            testDupExtensionsRefused(ext, 20, 1000, "into a graph of another order") != OK ||
            testDupExtensionsRefused(ext, 10, 100, "into less edge storage") != OK)
            Result = NOTOK;
    }

    if (Result == OK)
        gp_Message("Test succeeded.\n");
    else
        gp_ErrorMessage("Failed test of copying extension edge data between graphs of different edge capacities.\n");

    return Result;
}
