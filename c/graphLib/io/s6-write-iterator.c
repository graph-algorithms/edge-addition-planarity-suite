/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "strOrFile.h"

#include "s6-write-iterator.h"

// For the modification counter of the graph
#include "../graph.private.h"

/* Imported functions */
extern int _s6_GetNumBitsForVertex(int order);
extern int _CompactEdgeStorage(graphP theGraph);

/* Private function declarations (exported within system) */
int _s6_WriteGraphToStrOrFile(graphP theGraph, strOrFileP *pOutputContainer);

/* Private functions */
int _s6_InitWriterWithStrOrFile(S6WriteIteratorP theS6WriteIterator, strOrFileP *pOutputContainer);
int _s6_InitWriter(S6WriteIteratorP theS6WriteIterator);
int _s6_IsWriterInitialized(S6WriteIteratorP theS6WriteIterator, int reportUninitializedParts);
int _s6_IsDigraph(S6WriteIteratorP theS6WriteIterator);
int _s6_GrowChangeBatch(S6WriteIteratorP theS6WriteIterator);
size_t _s6_ChangeKeySlot(S6WriteIteratorP theS6WriteIterator, long long key);
int _s6_ChangeKeyInsert(S6WriteIteratorP theS6WriteIterator, long long key);
void _s6_ClearChangeBatch(S6WriteIteratorP theS6WriteIterator);
int _s6_NoEdgeIsHidden(graphP theGraph);
int _s6_CollectEdges(graphP theGraph, int **pPairs, int *pNumPairs);
int _s6_ComparePairs(void const *a, void const *b);
int _s6_EncodeLine(S6WriteIteratorP theS6WriteIterator, char lineChar, int const *pairs, int numPairs, int stride);
int _s6_ApplyChangeBatch(S6WriteIteratorP theS6WriteIterator);

int _s6_WriteGraphToFile(graphP theGraph, char *s6OutputFileName);
int _s6_WriteGraphToString(graphP theGraph, char **pOutputStr);

/********************************************************************
 Package private structure declaration for write iterator

 The formats are specified in
 https://users.cecs.anu.edu.au/~bdm/data/formats.txt and decoded by
 s6-read-iterator.c. A ':' line encodes a whole graph as N(n), the
 order encoding of graph6, followed by a bit stream of (b, x) pairs
 of 1 + k bits each, k being the number of bits needed to represent
 n-1. A ';' line encodes the symmetric difference between the graph
 on the previous line and this one, with the same bit stream and no
 order.

 The writer produces the bytes nauty produces for the same graph: the
 edges are emitted in the order of their larger endpoint, then their
 smaller one, and the padding of the last byte follows the rule of
 the specification, so that a file written here and one written by
 copyg differ only in the ">>sparse6<<" header, which this writer
 always writes, as the graph6 writer writes its own.

 A change batch holds the edges the caller has stored since the last
 write, as pairs of endpoints in the 0-based numbering of the file,
 with the kind of change. It is what the next ';' line encodes, and
 it is applied to the graph only when that line has been written, so
 until then the graph is the one on the previous line. The batch is
 relative to that graph: an edge added or deleted directly between
 writes is not in the batch, and the modification counter of the
 graph, sampled at each write, is how s6_WriteGraph() finds out that
 this has happened and refuses to write a ';' line that the reader
 could not apply.
 ********************************************************************/
struct S6WriteIteratorStruct
{
    strOrFileP outputContainer;

    graphP currGraph;

    int order;
    // Number of bits used to encode a vertex index, i.e. the number of
    // bits needed to represent order-1 (zero for a graph of order 1)
    int numBitsForVertex;

    int numGraphsWritten;

    // The change batch: three ints per change, the endpoints u < v in
    // the 0-based numbering of the file and the kind of change
    int *changes;
    int numChanges;
    int changeCapacity;

    // Open addressing table of the pair keys in the batch, so that a
    // pair stored twice in one batch is refused in constant time. The
    // key of {u, v} is u * order + v + 1, so that zero means empty.
    long long *changeKeys;
    size_t changeTableSize;

    // The modification counter of the graph as it was after the last
    // write, which is the graph the batch is relative to
    unsigned long long countAtLastWrite;

    // Set when a write failed after part of its effect was produced,
    // after which neither the output nor the graph is a state that a
    // further write could continue from
    int writerFailed;

    // Buffer in which a line is encoded before it is written
    char *lineBuff;
    size_t lineBuffSize;
};

#define S6_CHANGE_DELETE 0
#define S6_CHANGE_ADD 1

#define S6_INITIAL_CHANGE_CAPACITY 16

/********************************************************************
 Public and package private method implementations for write iterator
 ********************************************************************/

int s6_NewWriter(S6WriteIteratorP *pS6WriteIterator, graphP theGraph)
{
    if (pS6WriteIterator == NULL)
    {
        gp_ErrorMessage("Unable to allocate S6WriteIterator, as pointer to "
                        "which to assign address of memory allocated for "
                        "S6WriteIterator is NULL.");
        return NOTOK;
    }

    if ((*pS6WriteIterator) != NULL)
    {
        gp_ErrorMessage("S6WriteIterator is not NULL and therefore can't be "
                        "allocated.");
        return NOTOK;
    }

    if (theGraph == NULL || gp_GetN(theGraph) <= 0)
    {
        gp_ErrorMessage("Must allocate and initialize graph with an order "
                        "greater than 0 to use the S6WriteIterator.");
        return NOTOK;
    }

    // order, numBitsForVertex, numGraphsWritten, the batch counts, the
    // snapshot, the failure flag and the buffer size all set to 0
    (*pS6WriteIterator) = (S6WriteIteratorP)calloc(1, sizeof(S6WriteIteratorStruct));

    if ((*pS6WriteIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for S6WriteIterator.");
        return NOTOK;
    }

    (*pS6WriteIterator)->outputContainer = NULL;
    (*pS6WriteIterator)->currGraph = theGraph;
    (*pS6WriteIterator)->changes = NULL;
    (*pS6WriteIterator)->changeKeys = NULL;
    (*pS6WriteIterator)->lineBuff = NULL;

    return OK;
}

int _s6_IsWriterInitialized(S6WriteIteratorP theS6WriteIterator, int reportUninitializedParts)
{
    int writerIsInitialized = TRUE;

    if (theS6WriteIterator == NULL)
    {
        if (reportUninitializedParts)
            gp_ErrorMessage("S6WriteIterator is NULL.");
        writerIsInitialized = FALSE;
    }
    else
    {
        if (!sf_IsValidStrOrFile(theS6WriteIterator->outputContainer))
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6WriteIterator's outputContainer is not valid.");
            writerIsInitialized = FALSE;
        }
        if (theS6WriteIterator->order <= 0)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6WriteIterator's graph order has not been "
                                "determined.");
            writerIsInitialized = FALSE;
        }
        if (theS6WriteIterator->currGraph == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6WriteIterator's currGraph is NULL.");
            writerIsInitialized = FALSE;
        }
    }

    return writerIsInitialized;
}

int _s6_IsDigraph(S6WriteIteratorP theS6WriteIterator)
{
    return (gp_GetGraphFlags(theS6WriteIterator->currGraph) & GRAPHFLAGS_DIRECTEDEDGEDETECTED) ? TRUE : FALSE;
}

int s6_InitWriterWithString(S6WriteIteratorP theS6WriteIterator, char **pOutputString)
{
    strOrFileP outputContainer = NULL;

    if (theS6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6WriteIterator must be non-NULL.");
        return NOTOK;
    }

    if (_s6_IsWriterInitialized(theS6WriteIterator, FALSE))
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

    return _s6_InitWriterWithStrOrFile(theS6WriteIterator, (&outputContainer));
}

int s6_InitWriterWithFileName(S6WriteIteratorP theS6WriteIterator, char *outputFileName)
{
    strOrFileP outputContainer = NULL;

    if (theS6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6WriteIterator must be non-NULL.");
        return NOTOK;
    }

    if (_s6_IsWriterInitialized(theS6WriteIterator, FALSE))
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

    return _s6_InitWriterWithStrOrFile(theS6WriteIterator, (&outputContainer));
}

int _s6_InitWriterWithStrOrFile(S6WriteIteratorP theS6WriteIterator, strOrFileP *pOutputContainer)
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

    if (theS6WriteIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6WriteIterator must be non-NULL.");
        sf_SetOutputErrorFlag((*pOutputContainer));
        sf_Free(pOutputContainer);
        return NOTOK;
    }

    if (_s6_IsWriterInitialized(theS6WriteIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize writer, as it was already "
                        "previously initialized.");
        sf_SetOutputErrorFlag((*pOutputContainer));
        sf_Free(pOutputContainer);
        s6_SetOutputErrorFlag(theS6WriteIterator);
        return NOTOK;
    }

    theS6WriteIterator->outputContainer = (*pOutputContainer);
    // We have taken ownership of the outputContainer, and so we have set the
    // caller's pointer to NULL. The writer is responsible for freeing this
    // output container.
    (*pOutputContainer) = NULL;

    Result = _s6_InitWriter(theS6WriteIterator);
    if (Result != OK)
    {
        // Return the writer to its state before this call, so that a later
        // initialization neither leaks this container nor writes after a
        // header that may not have been written
        s6_SetOutputErrorFlag(theS6WriteIterator);
        sf_Free(&(theS6WriteIterator->outputContainer));
        theS6WriteIterator->order = 0;
        theS6WriteIterator->numBitsForVertex = 0;
    }

    return Result;
}

void s6_SetOutputErrorFlag(S6WriteIteratorP theS6WriteIterator)
{
    if (theS6WriteIterator != NULL && theS6WriteIterator->outputContainer != NULL)
        sf_SetOutputErrorFlag(theS6WriteIterator->outputContainer);
}

int _s6_InitWriter(S6WriteIteratorP theS6WriteIterator)
{
    char const *s6Header = ">>sparse6<<";
    int order = gp_GetN(theS6WriteIterator->currGraph);

    if (sf_fputs(s6Header, theS6WriteIterator->outputContainer) < 0)
    {
        gp_ErrorMessage("Unable to initialize writer due to failure to fputs "
                        "header to outputContainer.");
        return NOTOK;
    }

    theS6WriteIterator->order = order;
    theS6WriteIterator->numBitsForVertex = _s6_GetNumBitsForVertex(order);

    return OK;
}

/********************************************************************
 The change batch
 ********************************************************************/

size_t _s6_ChangeKeySlot(S6WriteIteratorP theS6WriteIterator, long long key)
{
    // Fibonacci hashing of the key into the table, whose size is a power of two
    unsigned long long hash = (unsigned long long)key * 11400714819323198485ULL;
    size_t mask = theS6WriteIterator->changeTableSize - 1;
    size_t slot = (size_t)(hash >> 32) & mask;

    while (theS6WriteIterator->changeKeys[slot] != 0 &&
           theS6WriteIterator->changeKeys[slot] != key)
        slot = (slot + 1) & mask;

    return slot;
}

// Inserts the key, or returns NOTOK if it is already in the table
int _s6_ChangeKeyInsert(S6WriteIteratorP theS6WriteIterator, long long key)
{
    size_t slot = _s6_ChangeKeySlot(theS6WriteIterator, key);

    if (theS6WriteIterator->changeKeys[slot] == key)
        return NOTOK;

    theS6WriteIterator->changeKeys[slot] = key;

    return OK;
}

// Doubles the batch and rebuilds its key table at four times the new
// capacity, so that probing stays short. Both allocations are made before
// either is published, so that a failure leaves the batch as it was, and
// the capacity stays below INT_MAX / 3 so that the three ints per change
// are always addressable.
int _s6_GrowChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    long long newCapacity = (theS6WriteIterator->changeCapacity == 0)
                                ? S6_INITIAL_CHANGE_CAPACITY
                                : (long long)theS6WriteIterator->changeCapacity * 2;
    size_t newTableSize = 1;
    int *newChanges = NULL;
    long long *newKeys = NULL;

    if (newCapacity > INT_MAX / 3 ||
        (size_t)newCapacity > SIZE_MAX / (4 * sizeof(long long)))
    {
        gp_ErrorMessage("Unable to grow the change batch of the sparse6 writer "
                        "beyond %d changes.",
                        theS6WriteIterator->changeCapacity);
        return NOTOK;
    }

    while (newTableSize < (size_t)newCapacity * 4)
        newTableSize <<= 1;

    newKeys = (long long *)calloc(newTableSize, sizeof(long long));
    if (newKeys == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the change table of the "
                        "sparse6 writer.");
        return NOTOK;
    }

    newChanges = (int *)realloc(theS6WriteIterator->changes, (size_t)newCapacity * 3 * sizeof(int));
    if (newChanges == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the change batch of the "
                        "sparse6 writer.");
        free(newKeys);
        return NOTOK;
    }

    theS6WriteIterator->changes = newChanges;
    theS6WriteIterator->changeCapacity = (int)newCapacity;

    if (theS6WriteIterator->changeKeys != NULL)
        free(theS6WriteIterator->changeKeys);

    theS6WriteIterator->changeKeys = newKeys;
    theS6WriteIterator->changeTableSize = newTableSize;

    // Re-enter the pairs already in the batch, which are distinct
    for (int i = 0; i < theS6WriteIterator->numChanges; i++)
    {
        int u = theS6WriteIterator->changes[3 * i];
        int v = theS6WriteIterator->changes[3 * i + 1];

        if (_s6_ChangeKeyInsert(theS6WriteIterator, (long long)u * theS6WriteIterator->order + v + 1) != OK)
            return NOTOK;
    }

    return OK;
}

void _s6_ClearChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    theS6WriteIterator->numChanges = 0;

    if (theS6WriteIterator->changeKeys != NULL)
        memset(theS6WriteIterator->changeKeys, 0, theS6WriteIterator->changeTableSize * sizeof(long long));
}

/********************************************************************
 s6_StoreGraphChange()

 Stores one change to be written by the next s6_WriteGraph() as part
 of an incremental (';') line: a deletion when e is an edge record
 and u and v are NIL, or an addition when e is NIL and u and v are
 vertices. Any other combination is refused.

 A deletion is stored as the endpoints of e rather than as e, so that
 no stored change is invalidated by another. The change is validated
 against the graph as it is now, which is the graph on the previous
 line as long as nothing has modified it directly: the edge must be
 in use for a deletion, and absent for an addition, since parallel
 edges are not supported; a loop is refused, and so is a pair that
 the batch already holds, because the line would then toggle the edge
 twice. Nothing can be stored before the first full (':') line has
 been written, since a ';' line is relative to the line before it.

 Returns OK on success, NOTOK otherwise, leaving the batch unchanged.
 ********************************************************************/

int s6_StoreGraphChange(S6WriteIteratorP theS6WriteIterator, int e, int u, int v)
{
    graphP theGraph = NULL;
    int kind = S6_CHANGE_ADD;
    int uFile = 0, vFile = 0;
    int *change = NULL;

    if (!_s6_IsWriterInitialized(theS6WriteIterator, TRUE))
    {
        gp_ErrorMessage("Unable to store a change because the S6WriteIterator "
                        "is not initialized.");
        return NOTOK;
    }

    theGraph = theS6WriteIterator->currGraph;

    if (theS6WriteIterator->writerFailed)
    {
        gp_ErrorMessage("Unable to store a change because an earlier write failed.");
        return NOTOK;
    }

    if (theS6WriteIterator->numGraphsWritten == 0)
    {
        gp_ErrorMessage("Unable to store a change before the first graph has "
                        "been written, as an incremental line is relative to "
                        "the line before it.");
        return NOTOK;
    }

    if (_s6_IsDigraph(theS6WriteIterator))
    {
        gp_ErrorMessage("Sparse6 format doesn't support digraphs.");
        return NOTOK;
    }

    if (gp_IsEdge(theGraph, e) && u == NIL && v == NIL)
    {
        int eFound = NIL;

        if (e < gp_LowerBoundEdges(theGraph) || e >= gp_UpperBoundEdges(theGraph) ||
            gp_EdgeNotInUse(theGraph, e))
        {
            gp_ErrorMessage("Unable to store the deletion of edge %d, which is "
                            "not an edge in use.",
                            e);
            return NOTOK;
        }

        kind = S6_CHANGE_DELETE;
        u = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e));
        v = gp_GetNeighbor(theGraph, e);

        // A hidden edge is in use but in no adjacency list, so the graph
        // the reader sees does not contain it; its deletion is refused
        // rather than written as a toggle that would add it
        eFound = gp_FindEdge(theGraph, u, v);
        if (eFound != e && eFound != gp_GetTwin(theGraph, e))
        {
            gp_ErrorMessage("Unable to store the deletion of edge %d, which "
                            "is hidden or a parallel edge.",
                            e);
            return NOTOK;
        }
    }
    else if (!gp_IsEdge(theGraph, e) && u != NIL && v != NIL)
    {
        // The vertex storage also holds virtual vertices, which the file
        // cannot name, so only the real vertices are accepted
        if (u < gp_LowerBoundVertices(theGraph) || u >= gp_UpperBoundVertices(theGraph) ||
            v < gp_LowerBoundVertices(theGraph) || v >= gp_UpperBoundVertices(theGraph))
        {
            gp_ErrorMessage("Unable to store the addition of edge {%d, %d}, "
                            "as a vertex is out of range.",
                            u, v);
            return NOTOK;
        }

        if (u == v)
        {
            gp_ErrorMessage("Unable to store the addition of a loop edge on "
                            "vertex %d, which is not supported.",
                            u);
            return NOTOK;
        }

        if (gp_IsEdge(theGraph, gp_FindEdge(theGraph, u, v)))
        {
            gp_ErrorMessage("Unable to store the addition of edge {%d, %d}, "
                            "which is already in the graph; parallel edges "
                            "are not supported.",
                            u, v);
            return NOTOK;
        }

        kind = S6_CHANGE_ADD;
    }
    else
    {
        gp_ErrorMessage("Invalid parameters: a change is either an edge e to "
                        "delete with u and v NIL, or an edge {u, v} to add "
                        "with e NIL.");
        return NOTOK;
    }

    // The file is 0-based, but in-memory storage may not be
    uFile = u - gp_LowerBoundVertexStorage(theGraph);
    vFile = v - gp_LowerBoundVertexStorage(theGraph);

    if (uFile > vFile)
    {
        int temp = uFile;
        uFile = vFile;
        vFile = temp;
    }

    if (theS6WriteIterator->numChanges == theS6WriteIterator->changeCapacity &&
        _s6_GrowChangeBatch(theS6WriteIterator) != OK)
        return NOTOK;

    if (_s6_ChangeKeyInsert(theS6WriteIterator, (long long)uFile * theS6WriteIterator->order + vFile + 1) != OK)
    {
        gp_ErrorMessage("Unable to store a second change of edge {%d, %d} in "
                        "one batch.",
                        u, v);
        return NOTOK;
    }

    change = theS6WriteIterator->changes + 3 * theS6WriteIterator->numChanges;
    change[0] = uFile;
    change[1] = vFile;
    change[2] = kind;
    theS6WriteIterator->numChanges++;

    return OK;
}

/********************************************************************
 Encoding
 ********************************************************************/

// Returns TRUE if every edge record in use is in an adjacency list, i.e.
// no edge is hidden, so that the graph in memory is the graph the file
// will describe. The walk costs what encoding the graph costs.
int _s6_NoEdgeIsHidden(graphP theGraph)
{
    long long numRecordsInLists = 0;

    for (int v = gp_LowerBoundVertexStorage(theGraph); v < gp_UpperBoundVertexStorage(theGraph); v++)
        for (int e = gp_GetFirstEdge(theGraph, v); gp_IsEdge(theGraph, e); e = gp_GetNextEdge(theGraph, e))
            numRecordsInLists++;

    return (numRecordsInLists == 2LL * gp_GetM(theGraph)) ? TRUE : FALSE;
}

// Collects the edges of the graph as pairs (min, max) of endpoints in
// the 0-based numbering of the file, two ints per edge, sorted as the
// format orders them, in an array the caller frees. An edgeless graph
// yields a NULL array. A loop, a parallel edge, or an edge on a virtual
// vertex cannot be written, since the reader would refuse or lose it,
// so each is reported and refused here.
int _s6_CollectEdges(graphP theGraph, int **pPairs, int *pNumPairs)
{
    int numEdges = gp_GetM(theGraph);
    int *pairs = NULL;
    int numPairs = 0;
    int lowerBound = gp_LowerBoundVertexStorage(theGraph);
    int order = gp_GetN(theGraph);

    (*pPairs) = NULL;
    (*pNumPairs) = 0;

    if (numEdges == 0)
        return OK;

    if ((size_t)numEdges > SIZE_MAX / (2 * sizeof(int)))
        return NOTOK;

    pairs = (int *)malloc((size_t)numEdges * 2 * sizeof(int));
    if (pairs == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory to collect the edges.");
        return NOTOK;
    }

    for (int e = gp_LowerBoundEdges(theGraph); e < gp_UpperBoundEdges(theGraph); e += 2)
    {
        int u = 0, v = 0;

        if (!gp_EdgeInUse(theGraph, e))
            continue;

        if (numPairs == numEdges)
        {
            gp_ErrorMessage("The graph has more edges in use than its edge count.");
            free(pairs);
            return NOTOK;
        }

        u = gp_GetNeighbor(theGraph, gp_GetTwin(theGraph, e)) - lowerBound;
        v = gp_GetNeighbor(theGraph, e) - lowerBound;

        if (u == v)
        {
            gp_ErrorMessage("Unable to write a loop edge on vertex %d, which "
                            "sparse6 input does not support.",
                            u + lowerBound);
            free(pairs);
            return NOTOK;
        }

        if (u < 0 || u >= order || v < 0 || v >= order)
        {
            gp_ErrorMessage("Unable to write edge {%d, %d}, which is not "
                            "between two of the %d vertices of the graph.",
                            u + lowerBound, v + lowerBound, order);
            free(pairs);
            return NOTOK;
        }

        pairs[2 * numPairs] = (u < v) ? u : v;
        pairs[2 * numPairs + 1] = (u < v) ? v : u;
        numPairs++;
    }

    if (numPairs != numEdges)
    {
        gp_ErrorMessage("The graph has fewer edges in use than its edge count.");
        free(pairs);
        return NOTOK;
    }

    // The specification orders the pairs by their larger endpoint; the sort
    // is skipped for fewer than two pairs, so that no library ever sees a
    // null array. Sorted, a parallel edge is a repeated pair.
    if (numPairs > 1)
        qsort(pairs, (size_t)numPairs, 2 * sizeof(int), _s6_ComparePairs);

    for (int p = 1; p < numPairs; p++)
    {
        if (pairs[2 * p] == pairs[2 * p - 2] && pairs[2 * p + 1] == pairs[2 * p - 1])
        {
            gp_ErrorMessage("Unable to write parallel edges between vertices "
                            "%d and %d, which sparse6 input does not support.",
                            pairs[2 * p] + lowerBound, pairs[2 * p + 1] + lowerBound);
            free(pairs);
            return NOTOK;
        }
    }

    (*pPairs) = pairs;
    (*pNumPairs) = numPairs;

    return OK;
}

// Orders pairs by their larger endpoint, then by their smaller one,
// which is the order in which nauty emits them
int _s6_ComparePairs(void const *a, void const *b)
{
    int const *pairA = (int const *)a;
    int const *pairB = (int const *)b;

    if (pairA[1] != pairB[1])
        return (pairA[1] < pairB[1]) ? -1 : 1;

    if (pairA[0] != pairB[0])
        return (pairA[0] < pairB[0]) ? -1 : 1;

    return 0;
}

// A line is written out through a buffer of this many bytes, so that a line
// of any length, which for a graph near the largest order can exceed what
// one sf_fputs() accepts, needs no more memory than that
#define S6_LINE_CHUNK_SIZE 65536

typedef struct
{
    char *out;
    size_t pos;
    unsigned long long acc;
    int numBits;
    strOrFileP outputContainer;
    int failed;
} s6BitWriter;

// Writes the bytes gathered so far to the output container, and remembers
// a failure so that the caller can report it once the line is done
static void _s6_FlushBytes(s6BitWriter *writer)
{
    if (writer->pos == 0)
        return;

    writer->out[writer->pos] = '\0';
    if (!writer->failed && sf_fputs(writer->out, writer->outputContainer) < 0)
        writer->failed = TRUE;
    writer->pos = 0;
}

// Appends one byte, keeping room in the buffer for the null terminator
static void _s6_PutByte(s6BitWriter *writer, char byte)
{
    if (writer->pos == S6_LINE_CHUNK_SIZE - 1)
        _s6_FlushBytes(writer);

    writer->out[writer->pos++] = byte;
}

// Appends the low width bits of value to the bit stream, writing each
// completed group of six bits as a byte in the range 63 to 126
static void _s6_PutBits(s6BitWriter *writer, unsigned int value, int width)
{
    writer->acc = (writer->acc << width) | (value & ((1ULL << width) - 1));
    writer->numBits += width;

    while (writer->numBits >= 6)
    {
        writer->numBits -= 6;
        _s6_PutByte(writer, (char)(((writer->acc >> writer->numBits) & 63) + 63));
    }

    writer->acc &= (1ULL << writer->numBits) - 1;
}

/********************************************************************
 _s6_EncodeLine()

 Encodes the given pairs, sorted by _s6_ComparePairs(), as one line
 of the file beginning with lineChar, which is ':' for a whole graph,
 in which case the order is written before the edges, or ';' for the
 symmetric difference from the previous line, which has no order.

 The bit stream follows the decoding procedure of the specification
 in reverse. With v the vertex the decoder tracks, starting at 0, an
 edge {i, j} with i < j is emitted as (0, i) if j is v, as (1, i) if
 j is v+1, since the b bit moves v there, and otherwise as (1, j)
 followed by (0, i), the first pair moving v to j. The bits are
 padded to a multiple of six with 1 bits, except in the one case the
 specification singles out: when n is a power of two, the last edge
 has n-2 as its larger endpoint and there are at least k+1 bits to
 pad, the 1 bits would decode as the loop {n-1, n-1}, so the padding
 begins with a 0 bit.

 The pairs are read at the given stride, which lets the change batch,
 which keeps a third int per pair, be encoded in place.

 The line, including its terminator, is written to the output
 container in pieces of at most S6_LINE_CHUNK_SIZE bytes, so its length
 is limited only by the order and the number of edges. Returns OK on
 success, NOTOK otherwise.
 ********************************************************************/

int _s6_EncodeLine(S6WriteIteratorP theS6WriteIterator, char lineChar, int const *pairs, int numPairs, int stride)
{
    const int order = theS6WriteIterator->order;
    const int numBitsForVertex = theS6WriteIterator->numBitsForVertex;
    s6BitWriter writer;
    int lastj = 0;

    if (theS6WriteIterator->lineBuff == NULL)
    {
        theS6WriteIterator->lineBuff = (char *)malloc(S6_LINE_CHUNK_SIZE);

        if (theS6WriteIterator->lineBuff == NULL)
        {
            gp_ErrorMessage("Unable to allocate memory for the sparse6 line.");
            return NOTOK;
        }

        theS6WriteIterator->lineBuffSize = S6_LINE_CHUNK_SIZE;
    }

    writer.out = theS6WriteIterator->lineBuff;
    writer.pos = 0;
    writer.acc = 0;
    writer.numBits = 0;
    writer.outputContainer = theS6WriteIterator->outputContainer;
    writer.failed = FALSE;

    _s6_PutByte(&writer, lineChar);

    if (lineChar == ':')
    {
        // The order is encoded exactly as in graph6: one byte for n <= 62,
        // 126 and three bytes carrying 18 bits for n <= 258047, and 126 126
        // and six bytes carrying 36 bits beyond that
        if (order <= 62)
            _s6_PutByte(&writer, (char)(order + 63));
        else if (order <= 258047)
        {
            _s6_PutByte(&writer, 126);
            for (int shift = 12; shift >= 0; shift -= 6)
                _s6_PutByte(&writer, (char)(((order >> shift) & 63) + 63));
        }
        else
        {
            _s6_PutByte(&writer, 126);
            _s6_PutByte(&writer, 126);
            for (int shift = 30; shift >= 0; shift -= 6)
                _s6_PutByte(&writer, (char)((((long long)order >> shift) & 63) + 63));
        }
    }

    // Once a piece of the line fails to go out, encoding the rest is wasted
    for (int p = 0; p < numPairs && !writer.failed; p++)
    {
        const int i = pairs[stride * p];
        const int j = pairs[stride * p + 1];

        if (j == lastj)
        {
            _s6_PutBits(&writer, 0, 1);
            _s6_PutBits(&writer, (unsigned int)i, numBitsForVertex);
        }
        else if (j == lastj + 1)
        {
            _s6_PutBits(&writer, 1, 1);
            _s6_PutBits(&writer, (unsigned int)i, numBitsForVertex);
            lastj = j;
        }
        else
        {
            _s6_PutBits(&writer, 1, 1);
            _s6_PutBits(&writer, (unsigned int)j, numBitsForVertex);
            _s6_PutBits(&writer, 0, 1);
            _s6_PutBits(&writer, (unsigned int)i, numBitsForVertex);
            lastj = j;
        }
    }

    if (writer.numBits > 0)
    {
        const int numPadBits = 6 - writer.numBits;

        if (order == (1 << numBitsForVertex) && lastj == order - 2 && numPadBits >= numBitsForVertex + 1)
            _s6_PutBits(&writer, (1u << (numPadBits - 1)) - 1, numPadBits);
        else
            _s6_PutBits(&writer, (1u << numPadBits) - 1, numPadBits);
    }

    _s6_PutByte(&writer, '\n');
    _s6_FlushBytes(&writer);

    if (writer.failed)
    {
        gp_ErrorMessage("Failed to output all characters of the sparse6 line.");
        return NOTOK;
    }

    return OK;
}

/********************************************************************
 _s6_ApplyChangeBatch()

 Applies the batch to the graph once its ';' line has been written,
 so that the graph is the one on that line, and compacts the edge
 storage, which the deletions leave with holes that some algorithms
 refuse. Each change is looked up by its endpoints, and its kind must
 still match the graph, which it does unless the graph was modified
 directly, in which case the write should not have happened.
 ********************************************************************/

int _s6_ApplyChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    graphP theGraph = theS6WriteIterator->currGraph;
    const int lowerBound = gp_LowerBoundVertexStorage(theGraph);

    for (int c = 0; c < theS6WriteIterator->numChanges; c++)
    {
        int const *change = theS6WriteIterator->changes + 3 * c;
        const int u = change[0] + lowerBound;
        const int v = change[1] + lowerBound;
        const int e = gp_FindEdge(theGraph, u, v);

        if (change[2] == S6_CHANGE_DELETE)
        {
            if (!gp_IsEdge(theGraph, e) || gp_DeleteEdge(theGraph, e) != OK)
            {
                gp_ErrorMessage("Unable to delete edge {%d, %d} while applying "
                                "the written changes to the graph.",
                                u, v);
                return NOTOK;
            }
        }
        else
        {
            if (gp_IsEdge(theGraph, e) || gp_DynamicAddEdge(theGraph, u, 0, v, 0) != OK)
            {
                gp_ErrorMessage("Unable to add edge {%d, %d} while applying "
                                "the written changes to the graph.",
                                u, v);
                return NOTOK;
            }
        }
    }

    return _CompactEdgeStorage(theGraph);
}

/********************************************************************
 s6_WriteGraph()

 Writes the graph as the next line of the output. With no changes
 stored since the last write, the whole graph is written as a ':'
 line, which is also how a graph that was modified directly is
 written. With changes stored, they are written as a ';' line and then
 applied to the graph, provided the graph is still the one written
 last: if it was modified directly since, the batch is not relative
 to the graph the reader holds, so the write is refused and the batch
 is discarded, and a full write is the way to continue.

 A graph with hidden edges is refused in either case, since the file
 could describe neither the graph with them nor the graph without
 them consistently with the changes validated against it.

 Returns OK on success, NOTOK otherwise. A refusal leaves the output
 as it was, so writing can go on. A failure while writing or applying
 a ';' line leaves the output and the graph in states no further
 write could continue from, so the output is marked as failed and
 later calls are refused.
 ********************************************************************/

int s6_WriteGraph(S6WriteIteratorP theS6WriteIterator)
{
    graphP theGraph = NULL;
    int *pairs = NULL;
    int numPairs = 0;
    int Result = OK;

    if (!_s6_IsWriterInitialized(theS6WriteIterator, TRUE))
    {
        gp_ErrorMessage("Unable to write graph because S6WriteIterator is not initialized.");
        return NOTOK;
    }

    theGraph = theS6WriteIterator->currGraph;

    if (theS6WriteIterator->writerFailed)
    {
        gp_ErrorMessage("Unable to write graph because an earlier write failed.");
        return NOTOK;
    }

    if (_s6_IsDigraph(theS6WriteIterator))
    {
        gp_ErrorMessage("Sparse6 format doesn't support digraphs.");
        return NOTOK;
    }

    if (gp_GetN(theGraph) != theS6WriteIterator->order)
    {
        gp_ErrorMessage("Unable to write a graph of order %d, as the writer "
                        "was initialized for graphs of order %d.",
                        gp_GetN(theGraph), theS6WriteIterator->order);
        return NOTOK;
    }

    if (theS6WriteIterator->numGraphsWritten == INT_MAX)
    {
        gp_ErrorMessage("Unable to write more than %d graphs to one sparse6 "
                        "output.",
                        INT_MAX);
        return NOTOK;
    }

    if (!_s6_NoEdgeIsHidden(theGraph))
    {
        gp_ErrorMessage("Unable to write a graph with hidden edges; restore "
                        "them first.");
        _s6_ClearChangeBatch(theS6WriteIterator);
        return NOTOK;
    }

    if (theS6WriteIterator->numChanges == 0)
    {
        if (_s6_CollectEdges(theGraph, &pairs, &numPairs) != OK)
            return NOTOK;

        Result = _s6_EncodeLine(theS6WriteIterator, ':', pairs, numPairs, 2);

        if (pairs != NULL)
            free(pairs);

        if (Result != OK)
        {
            theS6WriteIterator->writerFailed = TRUE;
            s6_SetOutputErrorFlag(theS6WriteIterator);
            return NOTOK;
        }
    }
    else
    {
        int numAdditions = 0;

        if (theGraphModificationCount(theGraph) != theS6WriteIterator->countAtLastWrite)
        {
            gp_ErrorMessage("Unable to write the stored changes, as the graph "
                            "was modified directly since the last write; the "
                            "changes are discarded, so write the whole graph "
                            "first.");
            _s6_ClearChangeBatch(theS6WriteIterator);
            return NOTOK;
        }

        // Room for the additions is made before the line is written, so
        // that applying the batch afterwards cannot fail for lack of it
        for (int c = 0; c < theS6WriteIterator->numChanges; c++)
            if (theS6WriteIterator->changes[3 * c + 2] == S6_CHANGE_ADD)
                numAdditions++;

        // A failure here has already touched the edge storage, so it is not
        // a refusal the caller can retry
        if (numAdditions > 0 &&
            gp_EnsureEdgeCapacity(theGraph, gp_GetM(theGraph) + numAdditions) != OK)
        {
            gp_ErrorMessage("Unable to ensure the edge capacity needed to "
                            "apply the stored changes.");
            theS6WriteIterator->writerFailed = TRUE;
            s6_SetOutputErrorFlag(theS6WriteIterator);
            return NOTOK;
        }

        // The batch is encoded in place: its pairs are sorted like those of
        // a whole graph, and the kind of a change plays no part in the line
        if (theS6WriteIterator->numChanges > 1)
            qsort(theS6WriteIterator->changes, (size_t)theS6WriteIterator->numChanges,
                  3 * sizeof(int), _s6_ComparePairs);

        Result = _s6_EncodeLine(theS6WriteIterator, ';', theS6WriteIterator->changes, theS6WriteIterator->numChanges, 3);

        // Once the line is out, the graph must become the graph on that line;
        // a failure in either step leaves nothing to continue from
        if (Result != OK || _s6_ApplyChangeBatch(theS6WriteIterator) != OK)
        {
            theS6WriteIterator->writerFailed = TRUE;
            s6_SetOutputErrorFlag(theS6WriteIterator);
            return NOTOK;
        }

        _s6_ClearChangeBatch(theS6WriteIterator);
    }

    theS6WriteIterator->numGraphsWritten++;
    theS6WriteIterator->countAtLastWrite = theGraphModificationCount(theGraph);

    return OK;
}

// If the writer is initialized with string, then when we free the writer this
// method will give the allocated string back to the user.
// NOTE: This setting will occur if any writer operations returned NOTOK, so the
// caller is responsible for checking if the string is NULL and freeing it in
// all cases.
void s6_FreeWriter(S6WriteIteratorP *pS6WriteIterator)
{
    if (pS6WriteIterator != NULL && (*pS6WriteIterator) != NULL)
    {
        if ((*pS6WriteIterator)->outputContainer != NULL)
            sf_Free((&((*pS6WriteIterator)->outputContainer)));

        (*pS6WriteIterator)->order = 0;
        (*pS6WriteIterator)->numBitsForVertex = 0;
        (*pS6WriteIterator)->numGraphsWritten = 0;

        if ((*pS6WriteIterator)->changes != NULL)
        {
            free((*pS6WriteIterator)->changes);
            (*pS6WriteIterator)->changes = NULL;
        }

        if ((*pS6WriteIterator)->changeKeys != NULL)
        {
            free((*pS6WriteIterator)->changeKeys);
            (*pS6WriteIterator)->changeKeys = NULL;
        }

        if ((*pS6WriteIterator)->lineBuff != NULL)
        {
            free((*pS6WriteIterator)->lineBuff);
            (*pS6WriteIterator)->lineBuff = NULL;
        }

        // N.B. The S6WriteIterator doesn't "own" the graph, so we don't free it.
        (*pS6WriteIterator)->currGraph = NULL;

        free((*pS6WriteIterator));
        (*pS6WriteIterator) = NULL;
    }
}

int _s6_WriteGraphToFile(graphP theGraph, char *s6OutputFileName)
{
    strOrFileP outputContainer = NULL;

    if (s6OutputFileName == NULL || strlen(s6OutputFileName) == 0)
    {
        gp_ErrorMessage("Unable to write graph to file, as output file name "
                        "supplied is NULL or empty.");
        return NOTOK;
    }

    if ((outputContainer = sf_NewOutputContainer(NULL, s6OutputFileName)) == NULL)
    {
        gp_ErrorMessage("Unable to allocate outputContainer to which to write.");
        return NOTOK;
    }

    return _s6_WriteGraphToStrOrFile(theGraph, (&outputContainer));
}

int _s6_WriteGraphToString(graphP theGraph, char **pOutputStr)
{
    strOrFileP outputContainer = NULL;

    if (pOutputStr == NULL)
    {
        gp_ErrorMessage("If writing sparse6 to string, must provide "
                        "pointer-pointer to allow _s6_WriteGraphToString() to "
                        "assign the address of the output string.");
        return NOTOK;
    }

    if ((*pOutputStr) != NULL)
    {
        gp_ErrorMessage("(*pOutputStr) should not point to allocated memory.");
        return NOTOK;
    }

    if ((outputContainer = sf_NewOutputContainer(pOutputStr, NULL)) == NULL)
    {
        gp_ErrorMessage("Unable to allocate outputContainer to which to write.");
        return NOTOK;
    }

    return _s6_WriteGraphToStrOrFile(theGraph, (&outputContainer));
}

// Writes the graph as one sparse6 line, with the header, taking ownership of
// the output container as the graph6 writer does, so (*pOutputContainer) is
// NULL after this call.
int _s6_WriteGraphToStrOrFile(graphP theGraph, strOrFileP *pOutputContainer)
{
    S6WriteIteratorP theS6WriteIterator = NULL;

    if (pOutputContainer == NULL || !sf_IsValidStrOrFile((*pOutputContainer)))
    {
        gp_ErrorMessage("Invalid sparse6 output container.");
        return NOTOK;
    }

    if (s6_NewWriter((&theS6WriteIterator), theGraph) != OK)
    {
        gp_ErrorMessage("Unable to allocate S6WriteIterator.");
        sf_SetOutputErrorFlag((*pOutputContainer));
        sf_Free(pOutputContainer);
        return NOTOK;
    }

    if (_s6_InitWriterWithStrOrFile(theS6WriteIterator, pOutputContainer) != OK)
    {
        gp_ErrorMessage("Unable to initialize S6WriteIterator.");
        s6_FreeWriter((&theS6WriteIterator));
        return NOTOK;
    }

    if (s6_WriteGraph(theS6WriteIterator) != OK)
    {
        // A refusal leaves a writer usable, but this writer is abandoned
        // here, so its header-only output is marked as failed rather than
        // handed back as a result
        gp_ErrorMessage("Unable to write graph using S6WriteIterator.");
        s6_SetOutputErrorFlag(theS6WriteIterator);
        s6_FreeWriter((&theS6WriteIterator));
        return NOTOK;
    }

    s6_FreeWriter((&theS6WriteIterator));

    return OK;
}
