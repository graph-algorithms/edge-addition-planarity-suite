/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "strOrFile.h"

#include "d6-write-iterator.h"

/* Imported functions */
extern int _g6_GetNumCharsForOrder(int order);
extern size_t _d6_GetNumCharsForEncoding(int order);
extern void _d6_EncodeOrder(char *orderChars, int order);
extern int _s6_NoEdgeIsHidden(graphP theGraph);

/* Private function declarations (exported within system) */
int _d6_WriteGraphToStrOrFile(graphP theGraph, strOrFileP *pOutputContainer);

/* Private functions */
int _d6_InitWriterWithStrOrFile(D6WriteIteratorP theD6WriteIterator, strOrFileP *pOutputContainer);
int _d6_InitWriter(D6WriteIteratorP theD6WriteIterator);
int _d6_IsWriterInitialized(D6WriteIteratorP theD6WriteIterator, int reportUninitializedParts);
int _d6_EncodeGraph(D6WriteIteratorP theD6WriteIterator);
int _d6_SetArcBit(char *graphEncodingChars, int order, int u, int v);
int _d6_WriteEncodedGraph(D6WriteIteratorP theD6WriteIterator);

/********************************************************************
 Package private structure declaration for write iterator
 ********************************************************************/
typedef struct strOrFileStruct strOrFileStruct;
typedef strOrFileStruct *strOrFileP;

struct D6WriteIteratorStruct
{
    strOrFileP outputContainer;

    int order;
    int numCharsForOrder;
    size_t numCharsForGraphEncoding;
    size_t currGraphBuffSize;
    char *currGraphBuff;

    graphP currGraph;
};

/********************************************************************
 Public and package private method implementations for write iterator
 ********************************************************************/

int d6_NewWriter(D6WriteIteratorP *pD6WriteIterator, graphP theGraph)
{
    if (pD6WriteIterator == NULL)
    {
        gp_ErrorMessage("Unable to allocate D6WriteIterator, as pointer to "
                        "which to assign address of memory allocated for "
                        "D6WriteIterator is NULL.");
        return NOTOK;
    }

    if ((*pD6WriteIterator) != NULL)
    {
        gp_ErrorMessage("D6WriteIterator is not NULL and therefore can't be allocated.");
        return NOTOK;
    }

    if (theGraph == NULL || gp_GetN(theGraph) <= 0)
    {
        gp_ErrorMessage("Must allocate and initialize graph with an order "
                        "greater than 0 to use the D6WriteIterator.");
        return NOTOK;
    }

    // order, numCharsForOrder, numCharsForGraphEncoding, and currGraphBuffSize
    // all set to 0
    (*pD6WriteIterator) = (D6WriteIteratorP)calloc(1, sizeof(D6WriteIteratorStruct));

    if ((*pD6WriteIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for D6WriteIterator.");
        return NOTOK;
    }

    (*pD6WriteIterator)->outputContainer = NULL;
    (*pD6WriteIterator)->currGraphBuff = NULL;
    (*pD6WriteIterator)->currGraph = theGraph;

    return OK;
}

int _d6_IsWriterInitialized(D6WriteIteratorP theD6WriteIterator, int reportUninitializedParts)
{
    int writerIsInitialized = TRUE;

    if (theD6WriteIterator == NULL)
    {
        if (reportUninitializedParts)
            gp_ErrorMessage("D6WriteIterator is NULL.");
        writerIsInitialized = FALSE;
    }
    else
    {
        if (!sf_IsValidStrOrFile(theD6WriteIterator->outputContainer))
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6WriteIterator's outputContainer is not valid.");
            writerIsInitialized = FALSE;
        }
        if (theD6WriteIterator->currGraphBuff == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6WriteIterator's currGraphBuff is NULL.");
            writerIsInitialized = FALSE;
        }
        if (theD6WriteIterator->currGraph == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6WriteIterator's currGraph is NULL.");
            writerIsInitialized = FALSE;
        }
        else if (gp_GetN(theD6WriteIterator->currGraph) == 0)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6WriteIterator's currGraph does not "
                                "contain a valid graph.");
            writerIsInitialized = FALSE;
        }
    }

    return writerIsInitialized;
}

int d6_InitWriterWithString(D6WriteIteratorP theD6WriteIterator, char **pOutputString)
{
    strOrFileP outputContainer = NULL;

    if (theD6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6WriteIterator must be non-NULL.");
        return NOTOK;
    }

    if (_d6_IsWriterInitialized(theD6WriteIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize writer, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (pOutputString == NULL)
    {
        gp_ErrorMessage("Unable to initialize writer with string, as pointer "
                        "to which to assign address of output string is NULL.");
        return NOTOK;
    }

    if ((*pOutputString) != NULL)
    {
        gp_ErrorMessage("Unable to initialize writer with string, as pointer "
                        "to which to assign address of output string points to "
                        "allocated memory.");
        return NOTOK;
    }

    if ((outputContainer = sf_NewOutputContainer(pOutputString, NULL)) == NULL)
    {
        gp_ErrorMessage("Unable to initialize writer with string, as we failed "
                        "to allocate the outputContainer.");
        return NOTOK;
    }

    return _d6_InitWriterWithStrOrFile(theD6WriteIterator, (&outputContainer));
}

int d6_InitWriterWithFileName(D6WriteIteratorP theD6WriteIterator, char *outputFileName)
{
    strOrFileP outputContainer = NULL;

    if (theD6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6WriteIterator must be non-NULL.");
        return NOTOK;
    }

    if (_d6_IsWriterInitialized(theD6WriteIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize writer, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (outputFileName == NULL || strlen(outputFileName) == 0)
    {
        gp_ErrorMessage("Unable to initialize writer with NULL or empty output "
                        "file name.");
        return NOTOK;
    }

    if ((outputContainer = sf_NewOutputContainer(NULL, outputFileName)) == NULL)
    {
        gp_ErrorMessage("Unable to initialize writer with file name, as we "
                        "failed to allocate the outputContainer.");
        return NOTOK;
    }

    return _d6_InitWriterWithStrOrFile(theD6WriteIterator, (&outputContainer));
}

int _d6_InitWriterWithStrOrFile(D6WriteIteratorP theD6WriteIterator, strOrFileP *pOutputContainer)
{
    int Result = OK;

    if (pOutputContainer == NULL || !sf_IsValidStrOrFile((*pOutputContainer)))
    {
        gp_ErrorMessage("Unable to initialize writer with invalid strOrFile "
                        "output container.");
        if (pOutputContainer != NULL && (*pOutputContainer) != NULL)
        {
            sf_SetOutputErrorFlag((*pOutputContainer));
            sf_Free(pOutputContainer);
        }
        return NOTOK;
    }

    if (theD6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6WriteIterator must be non-NULL.");
        sf_SetOutputErrorFlag((*pOutputContainer));
        sf_Free(pOutputContainer);
        return NOTOK;
    }

    if (_d6_IsWriterInitialized(theD6WriteIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize writer, as it was already "
                        "previously initialized.");
        sf_SetOutputErrorFlag((*pOutputContainer));
        sf_Free(pOutputContainer);
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    theD6WriteIterator->outputContainer = (*pOutputContainer);
    // We have taken ownership of the outputContainer, and so we have set the
    // caller's pointer to NULL. The writer is responsible for freeing this
    // output container.
    (*pOutputContainer) = NULL;

    // A writer that fails to initialize discards and frees the output it
    // took, so that it can be initialized again without losing it
    Result = _d6_InitWriter(theD6WriteIterator);
    if (Result != OK)
    {
        d6_SetOutputErrorFlag(theD6WriteIterator);
        sf_Free(&theD6WriteIterator->outputContainer);
        if (theD6WriteIterator->currGraphBuff != NULL)
        {
            free(theD6WriteIterator->currGraphBuff);
            theD6WriteIterator->currGraphBuff = NULL;
        }
    }

    return Result;
}

void d6_SetOutputErrorFlag(D6WriteIteratorP theD6WriteIterator)
{
    if (theD6WriteIterator != NULL && theD6WriteIterator->outputContainer != NULL)
        sf_SetOutputErrorFlag(theD6WriteIterator->outputContainer);
}

// Writes the header, as the graph6 and sparse6 writers write theirs, and
// sizes the line buffer for the order of the writer's graph, which every
// graph it writes must have.
int _d6_InitWriter(D6WriteIteratorP theD6WriteIterator)
{
    char const *d6Header = ">>digraph6<<";

    theD6WriteIterator->order = gp_GetN(theD6WriteIterator->currGraph);

    if (theD6WriteIterator->order > 100000)
    {
        gp_ErrorMessage("Graphs of order n > 100000 are not supported at this time.");
        return NOTOK;
    }

    theD6WriteIterator->numCharsForOrder = _g6_GetNumCharsForOrder(theD6WriteIterator->order);
    theD6WriteIterator->numCharsForGraphEncoding = _d6_GetNumCharsForEncoding(theD6WriteIterator->order);

    if (theD6WriteIterator->numCharsForGraphEncoding == 0 ||
        theD6WriteIterator->numCharsForGraphEncoding > (size_t)INT_MAX - 1 - theD6WriteIterator->numCharsForOrder - 3)
    {
        gp_ErrorMessage("Graphs of order %d are too large to write.", theD6WriteIterator->order);
        return NOTOK;
    }

    if (sf_fputs(d6Header, theD6WriteIterator->outputContainer) < 0)
    {
        gp_ErrorMessage("Unable to initialize writer due to failure to fputs "
                        "header to outputContainer.");
        return NOTOK;
    }

    // Must add 3 bytes for newline, possible carriage return, and null terminator
    theD6WriteIterator->currGraphBuffSize = 1 + theD6WriteIterator->numCharsForOrder + theD6WriteIterator->numCharsForGraphEncoding + 3;
    theD6WriteIterator->currGraphBuff = (char *)calloc(theD6WriteIterator->currGraphBuffSize, sizeof(char));

    if (theD6WriteIterator->currGraphBuff == NULL)
    {
        gp_ErrorMessage("Unable to initialize writer due to failure to "
                        "allocate memory for currGraphBuff.");
        return NOTOK;
    }

    return OK;
}

/********************************************************************
 d6_WriteGraph()

 Writes the writer's graph as one digraph6 line. An undirected edge is
 written as the two arcs u -> v and v -> u, which the reader reads back
 as one undirected edge, and a directed edge as its one arc.

 The format cannot represent parallel edges, including the two edges
 directed u -> v and v -> u, which would be written as the two arcs of
 a 2-cycle and read back as one undirected edge, nor can the reader
 read a loop, so a graph with either is not written. Neither is a graph with hidden edges, which are not in the
 adjacency lists that gp_HasParallelEdges() inspects; they are to be
 restored first.

 Returns OK on success, NOTOK on failure, in which case the output
 error flag is set, so that a partial output is not kept.
 ********************************************************************/
int d6_WriteGraph(D6WriteIteratorP theD6WriteIterator)
{
    graphP theGraph = NULL;

    if (!_d6_IsWriterInitialized(theD6WriteIterator, TRUE))
    {
        gp_ErrorMessage("Unable to write graph because D6WriteIterator is not initialized.");
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    theGraph = theD6WriteIterator->currGraph;

    if (gp_GetN(theGraph) != theD6WriteIterator->order)
    {
        gp_ErrorMessage("Unable to write a graph of order %d, as the writer "
                        "was initialized for graphs of order %d.",
                        gp_GetN(theGraph), theD6WriteIterator->order);
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    if (!_s6_NoEdgeIsHidden(theGraph))
    {
        gp_ErrorMessage("Unable to write a graph with hidden edges; restore "
                        "them first.");
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    // Two edges directed u -> v and v -> u count as parallel here, since
    // they would be written as the two arcs of a 2-cycle and read back as
    // one undirected edge
    if (gp_HasParallelEdges(theGraph, TRUE))
    {
        gp_ErrorMessage("D6 format doesn't support parallel edges.");
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    if (_d6_EncodeGraph(theD6WriteIterator) != OK)
    {
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    if (_d6_WriteEncodedGraph(theD6WriteIterator) != OK)
    {
        gp_ErrorMessage("Unable to write d6 encoded graph to output container.");
        d6_SetOutputErrorFlag(theD6WriteIterator);
        return NOTOK;
    }

    return OK;
}

// Sets bit u*order+v of the adjacency matrix, for the arc u -> v, in an
// encoding whose bytes do not yet carry the offset of 63. Returns NOTOK
// if the bit is set already, which only a parallel edge could do.
int _d6_SetArcBit(char *graphEncodingChars, int order, int u, int v)
{
    size_t bitIndex = (size_t)u * (size_t)order + (size_t)v;
    char bit = (char)(1u << (5 - (int)(bitIndex % 6)));

    if (graphEncodingChars[bitIndex / 6] & bit)
        return NOTOK;

    graphEncodingChars[bitIndex / 6] |= bit;
    return OK;
}

int _d6_EncodeGraph(D6WriteIteratorP theD6WriteIterator)
{
    char *d6Encoding = theD6WriteIterator->currGraphBuff;
    char *graphEncodingChars = NULL;
    graphP theGraph = theD6WriteIterator->currGraph;
    int order = theD6WriteIterator->order;
    int numCharsForOrder = theD6WriteIterator->numCharsForOrder;
    int lowerBound = gp_LowerBoundVertexStorage(theGraph);
    int u = NIL, v = NIL, direction = 0;

    // memset ensures all bits are zero, which means we only need to set the bits
    // that correspond to an arc; this also takes care of padding zeroes for us
    memset(d6Encoding, 0, theD6WriteIterator->currGraphBuffSize * sizeof(char));

    d6Encoding[0] = '&';
    _d6_EncodeOrder(d6Encoding + 1, order);

    graphEncodingChars = d6Encoding + 1 + numCharsForOrder;

    for (int e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); e += 2)
    {
        if (!gp_EdgeInUse(theGraph, e))
            continue;

        // Edge record e is in the adjacency list of u and leads to v. The
        // in-memory vertex storage may be 1-based or 0-based, so we subtract
        // the index of the first vertex in storage because the .d6 format is
        // 0-based
        u = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)) - lowerBound;
        v = gp_GetNeighbor(theGraph, e) - lowerBound;
        direction = gp_GetDirection(theGraph, e);

        if (u == v)
        {
            gp_ErrorMessage("Unable to write a loop edge on vertex %d, which "
                            "digraph6 input does not support.",
                            u + lowerBound);
            return NOTOK;
        }

        if (u < 0 || u >= order || v < 0 || v >= order)
        {
            gp_ErrorMessage("Unable to write edge {%d, %d}, which is not "
                            "between two of the %d vertices of the graph.",
                            u + lowerBound, v + lowerBound, order);
            return NOTOK;
        }

        if (direction == EDGEFLAG_DIRECTION_MASK)
        {
            gp_ErrorMessage("Unable to write edge {%d, %d}, which is marked "
                            "as directed both ways.",
                            u + lowerBound, v + lowerBound);
            return NOTOK;
        }

        // An edge that is directed into u is only the arc v -> u, an edge
        // directed out of u only the arc u -> v, and an undirected edge both
        if ((direction != EDGEFLAG_DIRECTION_INONLY && _d6_SetArcBit(graphEncodingChars, order, u, v) != OK) ||
            (direction != EDGEFLAG_DIRECTION_OUTONLY && _d6_SetArcBit(graphEncodingChars, order, v, u) != OK))
        {
            gp_ErrorMessage("Unable to write edge {%d, %d}, which is "
                            "parallel to another edge.",
                            u + lowerBound, v + lowerBound);
            return NOTOK;
        }
    }

    // Bytes corresponding to graph order have already been modified to
    // correspond to printable ascii character (i.e. by adding 63); must
    // now do the same for bytes corresponding to the adjacency matrix
    for (size_t i = 0; i < theD6WriteIterator->numCharsForGraphEncoding; i++)
        graphEncodingChars[i] += 63;

    return OK;
}

int _d6_WriteEncodedGraph(D6WriteIteratorP theD6WriteIterator)
{
    if (sf_fputs(theD6WriteIterator->currGraphBuff, theD6WriteIterator->outputContainer) < 0)
    {
        gp_ErrorMessage("Failed to output all characters of d6 encoding.");
        return NOTOK;
    }

    if (sf_fputs("\n", theD6WriteIterator->outputContainer) < 0)
    {
        gp_ErrorMessage("Failed to put line terminator after d6 encoding.");
        return NOTOK;
    }

    return OK;
}

// If the writer is initialized with string, then when we free the writer this
// method will give the allocated string back to the user.
// NOTE: This setting will occur if any writer operations returned NOTOK, so the
// caller is responsible for checking if the string is NULL and freeing it in
// all cases.
void d6_FreeWriter(D6WriteIteratorP *pD6WriteIterator)
{
    if (pD6WriteIterator != NULL && (*pD6WriteIterator) != NULL)
    {
        if ((*pD6WriteIterator)->outputContainer != NULL)
            sf_Free((&((*pD6WriteIterator)->outputContainer)));

        (*pD6WriteIterator)->order = 0;

        if ((*pD6WriteIterator)->currGraphBuff != NULL)
        {
            free((*pD6WriteIterator)->currGraphBuff);
            (*pD6WriteIterator)->currGraphBuff = NULL;
        }

        // N.B. The D6WriteIterator doesn't "own" the graph, so we don't free it.
        (*pD6WriteIterator)->currGraph = NULL;

        free((*pD6WriteIterator));
        (*pD6WriteIterator) = NULL;
    }
}

int _d6_WriteGraphToStrOrFile(graphP theGraph, strOrFileP *pOutputContainer)
{
    D6WriteIteratorP theD6WriteIterator = NULL;

    if (pOutputContainer == NULL || !sf_IsValidStrOrFile((*pOutputContainer)))
    {
        gp_ErrorMessage("Invalid D6 output container.");
        return NOTOK;
    }

    if (d6_NewWriter((&theD6WriteIterator), theGraph) != OK)
    {
        gp_ErrorMessage("Unable to allocate D6WriteIterator.");
        return NOTOK;
    }

    // NOTE: (*pOutputContainer) will be NULL after we return from this call,
    // since the write iterator will take ownership of the output container.
    if (_d6_InitWriterWithStrOrFile(theD6WriteIterator, pOutputContainer) != OK)
    {
        gp_ErrorMessage("Unable to initialize D6WriteIterator.");
        d6_FreeWriter((&theD6WriteIterator));
        return NOTOK;
    }

    if (d6_WriteGraph(theD6WriteIterator) != OK)
    {
        gp_ErrorMessage("Unable to write graph using D6WriteIterator.");
        d6_FreeWriter((&theD6WriteIterator));
        return NOTOK;
    }

    // The writer owns the container, so its file is closed here, where a
    // failure to close, which d6_FreeWriter() cannot report, is returned
    if (sf_closeFile(theD6WriteIterator->outputContainer) != OK)
    {
        gp_ErrorMessage("Unable to close the digraph6 output.");
        d6_FreeWriter((&theD6WriteIterator));
        return NOTOK;
    }

    d6_FreeWriter((&theD6WriteIterator));

    return OK;
}
