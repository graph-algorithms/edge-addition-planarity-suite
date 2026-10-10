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
void _s6_ChangeKeyPut(S6WriteIteratorP theS6WriteIterator, long long key, int value);
long long _s6_EdgeKey(S6WriteIteratorP theS6WriteIterator, int u, int v);
long long _s6_RecordKey(graphP theGraph, int e);
void _s6_ClearChangeBatch(S6WriteIteratorP theS6WriteIterator);
int _s6_CountInstances(graphP theGraph, int u, int v);
int _s6_IsInAdjacencyList(graphP theGraph, int u, int e);
int _s6_BuildToggles(S6WriteIteratorP theS6WriteIterator, int **pToggles, int *pNumToggles);
int _s6_NoEdgeIsHidden(graphP theGraph);
int _s6_CollectEdges(graphP theGraph, int **pPairs, int *pNumPairs);
int _s6_ComparePairs(void const *a, void const *b);
int _s6_EncodeLine(S6WriteIteratorP theS6WriteIterator, char lineChar, int const *pairs, int numPairs);
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
 n-1. A ';' line has the same bit stream and no order, and lists
 toggles of the graph on the previous line, which the reader applies
 in sequence: each pair deletes one instance of its edge if the graph
 has one and adds the edge otherwise. For a graph without parallel
 edges that is the symmetric difference of the specification. After
 a graph with parallel edges the specification does not define the
 line, and nauty's copyg takes that graph modulo 2 first, so such a
 file reads differently there.

 The writer produces the bytes nauty produces for the same graph: the
 edges are emitted in the order of their larger endpoint, then their
 smaller one, and the padding of the last byte follows the rule of
 the specification, so that a file written here and one written by
 copyg differ only in the ">>sparse6<<" header, which this writer
 always writes, as the graph6 writer writes its own.

 A change batch holds the changes the caller has stored since the
 last write: deletions of edge records and additions of edges, with
 the number of instances each edge they touch has, and how many of
 them the batch deletes and adds. The next ';' line toggles each such
 edge as many times as the deletions and the additions differ, which
 the reader turns into the same graph, and the batch is applied to
 the graph only when that line has been written, so until then the
 graph is the one on the previous line. The batch is relative to that
 graph: an edge added or deleted directly between writes is not in
 the batch, and the modification counter of the graph, sampled at each
 write, is how s6_WriteGraph() finds out that this has happened and
 refuses to write a ';' line that the reader could not apply.
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

    // The change batch: S6_CHANGE_INTS ints per change, the endpoints
    // u < v in the 0-based numbering of the file, the kind of change, and
    // for a deletion the edge record to delete
    int *changes;
    int numChanges;
    int changeCapacity;

    // The edges the batch touches: S6_EDGE_INTS ints per edge, the
    // endpoints u < v in the 0-based numbering of the file, the number
    // of instances of the edge in the graph, and the numbers of its
    // deletions and additions in the batch. There are no more of them
    // than changes, so they share the capacity of the batch.
    int *changedEdges;
    int numChangedEdges;

    // Open addressing table over the batch. The key of the edge {u, v}
    // is u * order + v + 1, and its value is the edge's entry in
    // changedEdges; the key of a deleted edge record is minus one more
    // than the lower of its two arc records, so that the same record
    // is not deleted twice. Zero means an empty slot.
    long long *changeKeys;
    int *changeValues;
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

#define S6_CHANGE_INTS 4
#define S6_EDGE_INTS 5
#define S6_EDGE_INSTANCES 2
#define S6_EDGE_DELETIONS 3
#define S6_EDGE_ADDITIONS 4

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
    (*pS6WriteIterator)->changedEdges = NULL;
    (*pS6WriteIterator)->changeKeys = NULL;
    (*pS6WriteIterator)->changeValues = NULL;
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

// Puts the key with its value in the table, which has room for it, as it
// holds at most two keys per change and has four slots per change
void _s6_ChangeKeyPut(S6WriteIteratorP theS6WriteIterator, long long key, int value)
{
    size_t slot = _s6_ChangeKeySlot(theS6WriteIterator, key);

    theS6WriteIterator->changeKeys[slot] = key;
    theS6WriteIterator->changeValues[slot] = value;
}

// The key of the edge {u, v}, u < v, in the 0-based numbering of the file
long long _s6_EdgeKey(S6WriteIteratorP theS6WriteIterator, int u, int v)
{
    return (long long)u * theS6WriteIterator->order + v + 1;
}

// The key of the edge record e, the same for both of its arc records
long long _s6_RecordKey(graphP theGraph, int e)
{
    int twin = gp_GetTwin(theGraph, e);

    (void)theGraph;

    return -((long long)(e < twin ? e : twin) + 1);
}

// Doubles the batch and rebuilds its table at four times the new
// capacity, so that probing stays short. All allocations are made before
// any is published, except that a grown buffer replaces its old self as
// soon as realloc() returns it, so that a failure leaves the batch as it
// was, only with more room. The capacity stays below INT_MAX /
// S6_EDGE_INTS so that the ints of every change and edge are addressable.
int _s6_GrowChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    long long newCapacity = (theS6WriteIterator->changeCapacity == 0)
                                ? S6_INITIAL_CHANGE_CAPACITY
                                : (long long)theS6WriteIterator->changeCapacity * 2;
    size_t newTableSize = 1;
    int *newChanges = NULL, *newEdges = NULL, *newValues = NULL;
    long long *newKeys = NULL;

    if (newCapacity > INT_MAX / S6_EDGE_INTS ||
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
    newValues = (int *)malloc(newTableSize * sizeof(int));
    if (newKeys == NULL || newValues == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the change table of the "
                        "sparse6 writer.");
        free(newKeys);
        free(newValues);
        return NOTOK;
    }

    newChanges = (int *)realloc(theS6WriteIterator->changes, (size_t)newCapacity * S6_CHANGE_INTS * sizeof(int));
    if (newChanges != NULL)
    {
        theS6WriteIterator->changes = newChanges;
        newEdges = (int *)realloc(theS6WriteIterator->changedEdges, (size_t)newCapacity * S6_EDGE_INTS * sizeof(int));
    }

    if (newChanges == NULL || newEdges == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the change batch of the "
                        "sparse6 writer.");
        free(newKeys);
        free(newValues);
        return NOTOK;
    }

    theS6WriteIterator->changedEdges = newEdges;
    theS6WriteIterator->changeCapacity = (int)newCapacity;

    free(theS6WriteIterator->changeKeys);
    free(theS6WriteIterator->changeValues);
    theS6WriteIterator->changeKeys = newKeys;
    theS6WriteIterator->changeValues = newValues;
    theS6WriteIterator->changeTableSize = newTableSize;

    // Re-enter the edges the batch touches, with their entries, and the
    // records it deletes
    for (int i = 0; i < theS6WriteIterator->numChangedEdges; i++)
    {
        int const *changedEdge = theS6WriteIterator->changedEdges + S6_EDGE_INTS * i;

        _s6_ChangeKeyPut(theS6WriteIterator, _s6_EdgeKey(theS6WriteIterator, changedEdge[0], changedEdge[1]), i);
    }

    for (int c = 0; c < theS6WriteIterator->numChanges; c++)
    {
        int const *change = theS6WriteIterator->changes + S6_CHANGE_INTS * c;

        if (change[2] == S6_CHANGE_DELETE)
            _s6_ChangeKeyPut(theS6WriteIterator, _s6_RecordKey(theS6WriteIterator->currGraph, change[3]), -1);
    }

    return OK;
}

void _s6_ClearChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    theS6WriteIterator->numChanges = 0;
    theS6WriteIterator->numChangedEdges = 0;

    if (theS6WriteIterator->changeKeys != NULL)
        memset(theS6WriteIterator->changeKeys, 0, theS6WriteIterator->changeTableSize * sizeof(long long));
}

// Counts the instances of the edge {u, v} in the adjacency list of u
int _s6_CountInstances(graphP theGraph, int u, int v)
{
    int numInstances = 0;

    for (int e = gp_GetFirstEdge(theGraph, u); gp_IsEdge(theGraph, e); e = gp_GetNextEdge(theGraph, e))
        if (gp_GetNeighbor(theGraph, e) == v)
            numInstances++;

    return numInstances;
}

// Returns TRUE if the arc record e is in the adjacency list of u, which is
// where it is unless its edge is hidden
int _s6_IsInAdjacencyList(graphP theGraph, int u, int e)
{
    for (int f = gp_GetFirstEdge(theGraph, u); gp_IsEdge(theGraph, f); f = gp_GetNextEdge(theGraph, f))
        if (f == e)
            return TRUE;

    return FALSE;
}

/********************************************************************
 s6_StoreGraphChange()

 Stores one change to be written by the next s6_WriteGraph() as part
 of an incremental (';') line: a deletion when e is an edge record
 and u and v are NIL, or an addition when e is NIL and u and v are
 vertices. Any other combination is refused.

 The change is validated against the graph as it is now, which is the
 graph on the previous line as long as nothing has modified it
 directly. A deletion must name an edge record in use and in an
 adjacency list, since a hidden edge is not in the graph the reader
 sees, and the batch may delete a record only once; any instance of a
 parallel edge can be deleted. An addition must join two distinct real
 vertices, since loops are not supported. The reader applies a ';'
 line as toggles in sequence, so from an edge with m instances a line
 can leave any number from 0 to m, and from an edge with none it can
 add one; a batch that would leave more is refused when the change
 that would make it so is stored. So an addition is accepted for an
 edge the graph lacks, once, or in place of an instance the batch has
 already deleted.
 Nothing can be stored before the first full (':') line has been
 written, since a ';' line is relative to the line before it.

 Returns OK on success, NOTOK otherwise, leaving the batch unchanged.
 ********************************************************************/

int s6_StoreGraphChange(S6WriteIteratorP theS6WriteIterator, int e, int u, int v)
{
    graphP theGraph = NULL;
    int kind = S6_CHANGE_ADD;
    int uFile = 0, vFile = 0;
    int edgeIndex = -1, numInstances = 0;
    long long edgeKey = 0;
    size_t edgeSlot = 0;
    int *change = NULL, *changedEdge = NULL;

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
        if (!_s6_IsInAdjacencyList(theGraph, u, e))
        {
            gp_ErrorMessage("Unable to store the deletion of edge %d, which "
                            "is hidden.",
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

    // The batch grows before anything else changes it, so that a failure
    // to grow leaves it as it was
    if (theS6WriteIterator->numChanges == theS6WriteIterator->changeCapacity &&
        _s6_GrowChangeBatch(theS6WriteIterator) != OK)
        return NOTOK;

    edgeKey = _s6_EdgeKey(theS6WriteIterator, uFile, vFile);
    edgeSlot = _s6_ChangeKeySlot(theS6WriteIterator, edgeKey);

    if (theS6WriteIterator->changeKeys[edgeSlot] == edgeKey)
    {
        edgeIndex = theS6WriteIterator->changeValues[edgeSlot];
        changedEdge = theS6WriteIterator->changedEdges + S6_EDGE_INTS * edgeIndex;
        numInstances = changedEdge[S6_EDGE_INSTANCES];
    }
    else
        numInstances = _s6_CountInstances(theGraph, u, v);

    if (kind == S6_CHANGE_DELETE)
    {
        long long recordKey = _s6_RecordKey(theGraph, e);

        if (theS6WriteIterator->changeKeys[_s6_ChangeKeySlot(theS6WriteIterator, recordKey)] == recordKey)
        {
            gp_ErrorMessage("Unable to store a second deletion of edge %d in "
                            "one batch.",
                            e);
            return NOTOK;
        }
    }
    else
    {
        int numDeleted = (changedEdge != NULL) ? changedEdge[S6_EDGE_DELETIONS] : 0;
        int numAdded = (changedEdge != NULL) ? changedEdge[S6_EDGE_ADDITIONS] : 0;

        if (numAdded >= numDeleted && (numInstances > 0 || numAdded > 0))
        {
            gp_ErrorMessage("Unable to store the addition of edge {%d, %d}, "
                            "which would leave it with more instances than a "
                            "';' line can: an instance can be added only to "
                            "an edge the graph lacks, once, or in place of one "
                            "the batch deletes.",
                            u, v);
            return NOTOK;
        }
    }

    // Nothing below can fail, so the batch changes only on success
    if (changedEdge == NULL)
    {
        edgeIndex = theS6WriteIterator->numChangedEdges++;
        changedEdge = theS6WriteIterator->changedEdges + S6_EDGE_INTS * edgeIndex;
        changedEdge[0] = uFile;
        changedEdge[1] = vFile;
        changedEdge[S6_EDGE_INSTANCES] = numInstances;
        changedEdge[S6_EDGE_DELETIONS] = 0;
        changedEdge[S6_EDGE_ADDITIONS] = 0;
        _s6_ChangeKeyPut(theS6WriteIterator, edgeKey, edgeIndex);
    }

    change = theS6WriteIterator->changes + S6_CHANGE_INTS * theS6WriteIterator->numChanges;
    change[0] = uFile;
    change[1] = vFile;
    change[2] = kind;
    change[3] = (kind == S6_CHANGE_DELETE) ? e : NIL;
    theS6WriteIterator->numChanges++;

    if (kind == S6_CHANGE_DELETE)
    {
        changedEdge[S6_EDGE_DELETIONS]++;
        _s6_ChangeKeyPut(theS6WriteIterator, _s6_RecordKey(theGraph, e), -1);
    }
    else
        changedEdge[S6_EDGE_ADDITIONS]++;

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
// yields a NULL array. Parallel edges become repeated pairs, which the
// sort makes adjacent, as nauty writes them. A loop or an edge on a
// virtual vertex cannot be written, since the reader would refuse or
// lose it, so each is reported and refused here.
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
    // null array
    if (numPairs > 1)
        qsort(pairs, (size_t)numPairs, 2 * sizeof(int), _s6_ComparePairs);

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

 Encodes the given pairs, two ints each and sorted by
 _s6_ComparePairs(), as one line of the file beginning with lineChar, which is ':' for a whole graph,
 in which case the order is written before the edges, or ';' for
 toggles of the graph on the previous line, which has no order.

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

 The line, including its terminator, is written to the output
 container in pieces of at most S6_LINE_CHUNK_SIZE bytes, so its length
 is limited only by the order and the number of edges. Returns OK on
 success, NOTOK otherwise.
 ********************************************************************/

int _s6_EncodeLine(S6WriteIteratorP theS6WriteIterator, char lineChar, int const *pairs, int numPairs)
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
        const int i = pairs[2 * p];
        const int j = pairs[2 * p + 1];

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
 _s6_BuildToggles()

 Lists the toggles of the ';' line for the batch: each edge the batch
 touches, as many times as its deletions and additions differ, which
 the reader turns into the graph the batch leaves, since the batch
 deletes no more instances than the edge has and adds one only to an
 edge the graph lacks or in place of an instance it deletes. Edges the
 batch leaves as they were are left out. The pairs are sorted as the
 format orders them, repeats adjacent, in an array the caller frees,
 which is NULL when there is no toggle.
 ********************************************************************/

int _s6_BuildToggles(S6WriteIteratorP theS6WriteIterator, int **pToggles, int *pNumToggles)
{
    long long numToggles = 0;
    int *toggles = NULL;
    int t = 0;

    (*pToggles) = NULL;
    (*pNumToggles) = 0;

    for (int i = 0; i < theS6WriteIterator->numChangedEdges; i++)
    {
        int const *changedEdge = theS6WriteIterator->changedEdges + S6_EDGE_INTS * i;

        numToggles += abs(changedEdge[S6_EDGE_DELETIONS] - changedEdge[S6_EDGE_ADDITIONS]);
    }

    if (numToggles == 0)
        return OK;

    // There are no more toggles than changes, which fit in an int
    if ((toggles = (int *)malloc((size_t)numToggles * 2 * sizeof(int))) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the toggles of the "
                        "sparse6 line.");
        return NOTOK;
    }

    for (int i = 0; i < theS6WriteIterator->numChangedEdges; i++)
    {
        int const *changedEdge = theS6WriteIterator->changedEdges + S6_EDGE_INTS * i;
        int count = abs(changedEdge[S6_EDGE_DELETIONS] - changedEdge[S6_EDGE_ADDITIONS]);

        for (int c = 0; c < count; c++, t++)
        {
            toggles[2 * t] = changedEdge[0];
            toggles[2 * t + 1] = changedEdge[1];
        }
    }

    if (numToggles > 1)
        qsort(toggles, (size_t)numToggles, 2 * sizeof(int), _s6_ComparePairs);

    (*pToggles) = toggles;
    (*pNumToggles) = (int)numToggles;

    return OK;
}

/********************************************************************
 _s6_ApplyChangeBatch()

 Applies the batch to the graph once its ';' line has been written,
 so that the graph is the one on that line, and compacts the edge
 storage, which the deletions leave with holes that some algorithms
 refuse. The deletions come first, each by its edge record, which is
 still in use, since nothing moves an edge record until the
 compaction and the batch deletes a record only once; the additions
 then fill the holes they leave. A change that no longer fits the
 graph means it was modified directly, in which case the write should
 not have happened.
 ********************************************************************/

int _s6_ApplyChangeBatch(S6WriteIteratorP theS6WriteIterator)
{
    graphP theGraph = theS6WriteIterator->currGraph;
    const int lowerBound = gp_LowerBoundVertexStorage(theGraph);

    for (int c = 0; c < theS6WriteIterator->numChanges; c++)
    {
        int const *change = theS6WriteIterator->changes + S6_CHANGE_INTS * c;

        if (change[2] == S6_CHANGE_DELETE &&
            (gp_EdgeNotInUse(theGraph, change[3]) || gp_DeleteEdge(theGraph, change[3]) != OK))
        {
            gp_ErrorMessage("Unable to delete edge {%d, %d} while applying "
                            "the written changes to the graph.",
                            change[0] + lowerBound, change[1] + lowerBound);
            return NOTOK;
        }
    }

    for (int c = 0; c < theS6WriteIterator->numChanges; c++)
    {
        int const *change = theS6WriteIterator->changes + S6_CHANGE_INTS * c;

        if (change[2] == S6_CHANGE_ADD &&
            gp_DynamicAddEdge(theGraph, change[0] + lowerBound, 0, change[1] + lowerBound, 0) != OK)
        {
            gp_ErrorMessage("Unable to add edge {%d, %d} while applying "
                            "the written changes to the graph.",
                            change[0] + lowerBound, change[1] + lowerBound);
            return NOTOK;
        }
    }

    return _CompactEdgeStorage(theGraph);
}

/********************************************************************
 s6_WriteGraph()

 Writes the graph as the next line of the output. With no changes
 stored since the last write, the whole graph is written as a ':'
 line, which is also how a graph that was modified directly is
 written; parallel edges are written as repeated pairs. With changes
 stored, they are written as a ';' line of toggles and then applied
 to the graph, provided the graph is still the one written last: if
 it was modified directly since, the batch is not relative to the
 graph the reader holds, so the write is refused and the batch is
 discarded, and a full write is the way to continue. A batch whose
 changes cancel out gives an empty ';' line.

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

        Result = _s6_EncodeLine(theS6WriteIterator, ':', pairs, numPairs);

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
        int numDeletions = 0, numAdditions = 0;

        if (theGraphModificationCount(theGraph) != theS6WriteIterator->countAtLastWrite)
        {
            gp_ErrorMessage("Unable to write the stored changes, as the graph "
                            "was modified directly since the last write; the "
                            "changes are discarded, so write the whole graph "
                            "first.");
            _s6_ClearChangeBatch(theS6WriteIterator);
            return NOTOK;
        }

        // Nothing is touched yet, so a failure here is a refusal
        if (_s6_BuildToggles(theS6WriteIterator, &pairs, &numPairs) != OK)
            return NOTOK;

        for (int c = 0; c < theS6WriteIterator->numChanges; c++)
        {
            if (theS6WriteIterator->changes[S6_CHANGE_INTS * c + 2] == S6_CHANGE_ADD)
                numAdditions++;
            else
                numDeletions++;
        }

        // Room for the additions is made before the line is written, so
        // that applying the batch afterwards cannot fail for lack of it.
        // The deletions are applied first and the additions reuse their
        // holes, so the graph never has more edges than before or after
        // the batch. A failure here has already touched the edge storage,
        // so it is not a refusal the caller can retry.
        if (numAdditions > numDeletions &&
            gp_EnsureEdgeCapacity(theGraph, gp_GetM(theGraph) + numAdditions - numDeletions) != OK)
        {
            gp_ErrorMessage("Unable to ensure the edge capacity needed to "
                            "apply the stored changes.");
            if (pairs != NULL)
                free(pairs);
            theS6WriteIterator->writerFailed = TRUE;
            s6_SetOutputErrorFlag(theS6WriteIterator);
            return NOTOK;
        }

        Result = _s6_EncodeLine(theS6WriteIterator, ';', pairs, numPairs);

        if (pairs != NULL)
            free(pairs);

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

        if ((*pS6WriteIterator)->changedEdges != NULL)
        {
            free((*pS6WriteIterator)->changedEdges);
            (*pS6WriteIterator)->changedEdges = NULL;
        }

        if ((*pS6WriteIterator)->changeKeys != NULL)
        {
            free((*pS6WriteIterator)->changeKeys);
            (*pS6WriteIterator)->changeKeys = NULL;
        }

        if ((*pS6WriteIterator)->changeValues != NULL)
        {
            free((*pS6WriteIterator)->changeValues);
            (*pS6WriteIterator)->changeValues = NULL;
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
