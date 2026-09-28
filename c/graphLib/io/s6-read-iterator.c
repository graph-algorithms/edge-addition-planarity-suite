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

#include "s6-read-iterator.h"

// For definition of zero-based IO flag
#include "graphIO.h"

// For the package private modification counter of the graph
#include "../graph.private.h"

/* Private function declarations (exported within system) */
extern int _CompactEdgeStorage(graphP theGraph);
int _s6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *pInputContainer);

/* Private functions */
int _s6_InitReaderWithStrOrFile(S6ReadIteratorP theS6ReadIterator, strOrFileP *pInputContainer);
int _s6_InitReader(S6ReadIteratorP theS6ReadIterator);
int _s6_IsReaderInitialized(S6ReadIteratorP theS6ReadIterator, int reportUninitializedParts);
int _s6_ValidateHeader(strOrFileP inputContainer);
int _s6_ReadOrder(strOrFileP inputContainer, int *order, const int lineNum);
int _s6_GetNumBitsForVertex(int order);
int _s6_ReadNextByte(strOrFileP inputContainer, int *byteBits, int *endOfLine, unsigned long long *bytePos, const int lineNum);
void _s6_ReportLineStartError(int firstChar, const int lineNum);
void _s6_StartLine(S6ReadIteratorP theS6ReadIterator);
int _s6_DecodeEdges(S6ReadIteratorP theS6ReadIterator, const int stopAtEdge, const int incremental,
                    int *pU, int *pV, int *pEndOfLine, const int lineNum);
int _s6_CheckEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum);
int _s6_ApplyEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum);
int _s6_GrowRetrievedChanges(S6ReadIteratorP theS6ReadIterator);
int _s6_IsGraphAsLastRead(S6ReadIteratorP theS6ReadIterator, const int lineNum);
int _s6_ApplyRetrievedChanges(S6ReadIteratorP theS6ReadIterator);

int _s6_ReadGraphFromFile(graphP theGraph, char *pathToS6File);
int _s6_ReadGraphFromString(graphP theGraph, char *s6EncodedString);

/********************************************************************
 Package private structure declaration for read iterator

 The sparse6 and incremental sparse6 formats are specified in
 https://users.cecs.anu.edu.au/~bdm/data/formats.txt

 A line beginning with ':' encodes a whole graph as N(n), the same
 order encoding as graph6, followed by a bit stream of (b, x) pairs
 of 1 + k bits each, where k is the number of bits needed to
 represent n-1. The pairs are decoded with a running vertex v that
 starts at 0: b set means v is incremented, then x greater than v
 moves v to x, otherwise the pair is the edge {x, v}. Pairs that
 push v to n or beyond are padding, as is an incomplete pair at the
 end of the line.

 A line beginning with ';' encodes the symmetric difference between
 the graph on the previous line and this one, with the same bit
 stream and no order, so it cannot be the first line of the input.

 Unlike the graph6 reader, this reader does not buffer a line before
 decoding it. A graph6 line has a fixed length for a given order,
 but a sparse6 line grows with the number of edges, so the bit
 stream is decoded straight from the input container one byte at a
 time until the end of the line.

 The decoding state of the line is kept in the iterator, so that
 s6_RetrieveGraphChange() can look ahead into a ';' line one edge at
 a time, and s6_ReadGraph() can apply what was retrieved and decode
 the rest of the line from where the lookahead stopped.
 ********************************************************************/
#define S6_LINE_NOT_STARTED 0
#define S6_LINE_WHOLE_NEXT 1
#define S6_LINE_END_NEXT 2
#define S6_LINE_INCREMENTAL_OPEN 3
#define S6_LINE_INCREMENTAL_DONE 4

#define S6_CHANGE_DELETE 0
#define S6_CHANGE_ADD 1

#define S6_INITIAL_CHANGE_CAPACITY 16

struct S6ReadIteratorStruct
{
    strOrFileP inputContainer;
    int numGraphsRead;

    int order;
    // Number of bits used to encode a vertex index, i.e. the number of
    // bits needed to represent order-1 (zero for a graph of order 1)
    int numBitsForVertex;

    // Set by initialization, which consumes the ':' and the order from
    // the first line, and cleared by the first s6_ReadGraph(), which
    // then has only the edge list of that line left to decode
    int firstLinePrefixConsumed;

    // One entry per vertex, used to detect a repeated edge on a ':' line
    // in constant time. Each edge {x, v} with x <= v is decoded from a
    // pair whose x and v are fixed by the edge, and v never decreases
    // along a line, so a repeat of the edge can only occur while v is
    // unchanged. The entry for x records the (line, v) of the last edge
    // decoded on x as lineNum * (order + 1) + v + 1, so the check is a
    // comparison with no per-line reset. Needs long long because
    // lineNum and order can each exceed 2^16.
    long long *edgeStamps;

    graphP currGraph;

    int endReached;

    // Set when a read or a retrieval fails after it has begun to consume
    // a line, since the input is then no longer at the start of a line
    // and the graph may hold part of the line
    int readerFailed;

    // How far the lookahead has got into the next line: not started, a
    // ':' line found (the ':' is consumed and the rest of the line is
    // left to s6_ReadGraph()), the end of the input found, or a ';' line
    // whose edges are being retrieved or have all been retrieved
    int lineState;

    // The decoding state of the current line: the data bits of the
    // current byte and how many of them are unread, the running vertex
    // v of the decoding procedure, and the position in the line for
    // error messages
    int byteBits;
    int numBitsLeft;
    int currV;
    unsigned long long bytePos;

    // The changes retrieved from the current ';' line, which the next
    // s6_ReadGraph() applies: three ints per change, the kind, then the
    // edge record for a deletion or the two vertices for an addition
    int *changes;
    int numChanges;
    int changeCapacity;

    // The modification counter of the graph after the last graph was
    // read. A ';' line is the difference from that graph, so the edge
    // records the lookahead reports, and the toggles s6_ReadGraph()
    // applies, are only right while the counter has not moved.
    unsigned long long countAtLastRead;
};

/********************************************************************
 Public and package private method implementations for read iterator
 ********************************************************************/

/********************************************************************
 s6_IsSparse6Input()

 Returns TRUE if the given first line of an input is sparse6 or
 incremental sparse6 content, i.e. it starts with the optional
 ">>sparse6<<" header, with the ':' of a whole graph, or with the ';'
 of an incremental graph. A leading ';' is an error, since the first
 graph of an input cannot be incremental, but it is the reader that
 reports it, so this method accepts it as sparse6 input.

 Returns FALSE for a NULL first line and for the other formats.
 ********************************************************************/

int s6_IsSparse6Input(char const *const firstLine)
{
    if (firstLine == NULL)
        return FALSE;

    return (firstLine[0] == ':' || firstLine[0] == ';' ||
            strncmp(firstLine, ">>sparse6<<", strlen(">>sparse6<<")) == 0)
               ? TRUE
               : FALSE;
}

int s6_NewReader(S6ReadIteratorP *pS6ReadIterator, graphP theGraph)
{
    if (pS6ReadIterator == NULL)
    {
        gp_ErrorMessage("Unable to allocate S6ReadIterator, as pointer to "
                        "which to assign address of memory allocated for "
                        "S6ReadIterator is NULL.");
        return NOTOK;
    }

    if ((*pS6ReadIterator) != NULL)
    {
        gp_ErrorMessage("S6ReadIterator is not NULL and therefore can't be "
                        "allocated.");
        return NOTOK;
    }

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Must allocate graph to be used by S6ReadIterator.");
        return NOTOK;
    }

    // numGraphsRead, order, numBitsForVertex, firstLinePrefixConsumed,
    // endReached, readerFailed, the line state (S6_LINE_NOT_STARTED) and
    // the decoding state all set to 0, and the change list to empty
    (*pS6ReadIterator) = (S6ReadIteratorP)calloc(1, sizeof(S6ReadIteratorStruct));

    if ((*pS6ReadIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for S6ReadIterator.");
        return NOTOK;
    }

    (*pS6ReadIterator)->inputContainer = NULL;
    (*pS6ReadIterator)->currGraph = theGraph;

    return OK;
}

int _s6_IsReaderInitialized(S6ReadIteratorP theS6ReadIterator, int reportUninitializedParts)
{
    int readerInitialized = TRUE;

    if (theS6ReadIterator == NULL)
    {
        if (reportUninitializedParts)
            gp_ErrorMessage("S6ReadIterator is NULL.");
        readerInitialized = FALSE;
    }
    else
    {
        if (!sf_IsValidStrOrFile(theS6ReadIterator->inputContainer))
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6ReadIterator's inputContainer string-or-file "
                                "container is not valid.");
            readerInitialized = FALSE;
        }
        if (theS6ReadIterator->order <= 0)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6ReadIterator's graph order has not been "
                                "determined.");
            readerInitialized = FALSE;
        }
        if (theS6ReadIterator->currGraph == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("S6ReadIterator's currGraph is NULL.");
            readerInitialized = FALSE;
        }
    }

    return readerInitialized;
}

int s6_EndReached(S6ReadIteratorP theS6ReadIterator)
{
    if (theS6ReadIterator == NULL)
        return TRUE;

    return theS6ReadIterator->endReached;
}

int s6_InitReaderWithString(S6ReadIteratorP theS6ReadIterator, char *inputString)
{
    strOrFileP inputContainer = NULL;

    if (theS6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_s6_IsReaderInitialized(theS6ReadIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize reader, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (inputString == NULL || strlen(inputString) == 0)
    {
        gp_ErrorMessage("Unable to initialize reader with empty input string.");
        return NOTOK;
    }

    if ((inputContainer = sf_NewInputContainer(inputString, NULL)) == NULL)
    {
        gp_ErrorMessage("Unable to initialize reader with string, as we failed "
                        "to allocate the inputContainer.");
        return NOTOK;
    }

    return _s6_InitReaderWithStrOrFile(
        theS6ReadIterator,
        (&inputContainer));
}

int s6_InitReaderWithFileName(S6ReadIteratorP theS6ReadIterator, char const *const infileName)
{
    strOrFileP inputContainer = NULL;

    if (theS6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_s6_IsReaderInitialized(theS6ReadIterator, FALSE))
    {
        gp_ErrorMessage("Unable to initialize reader, as it was already "
                        "previously initialized.");
        return NOTOK;
    }

    if (infileName == NULL || strlen(infileName) == 0)
    {
        gp_ErrorMessage("Unable to initialize reader with empty infile name.");
        return NOTOK;
    }

    if ((inputContainer = sf_NewInputContainer(NULL, infileName)) == NULL)
    {
        gp_ErrorMessage("Unable to initialize reader with file name, as we "
                        "failed to allocate the inputContainer.");
        return NOTOK;
    }

    return _s6_InitReaderWithStrOrFile(
        theS6ReadIterator,
        (&inputContainer));
}

int _s6_InitReaderWithStrOrFile(S6ReadIteratorP theS6ReadIterator, strOrFileP *pInputContainer)
{
    if (theS6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theS6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (pInputContainer == NULL || !sf_IsValidStrOrFile((*pInputContainer)))
    {
        gp_ErrorMessage("Unable to initialize reader with invalid strOrFile "
                        "input container.");
        return NOTOK;
    }

    theS6ReadIterator->inputContainer = (*pInputContainer);
    // We have taken ownership of the inputContainer, and so we have set the
    // caller's pointer to NULL. The reader is responsible for freeing this
    // input container.
    (*pInputContainer) = NULL;

    if (_s6_InitReader(theS6ReadIterator) != OK)
    {
        // Return the reader to its state before this call, so that a later
        // initialization neither leaks this input container nor reads on
        // from the failed input
        sf_Free(&(theS6ReadIterator->inputContainer));

        if (theS6ReadIterator->edgeStamps != NULL)
        {
            free(theS6ReadIterator->edgeStamps);
            theS6ReadIterator->edgeStamps = NULL;
        }

        theS6ReadIterator->order = 0;
        theS6ReadIterator->numBitsForVertex = 0;
        theS6ReadIterator->firstLinePrefixConsumed = FALSE;

        return NOTOK;
    }

    return OK;
}

int _s6_InitReader(S6ReadIteratorP theS6ReadIterator)
{
    int firstChar = EOF;
    int charConfirmation = EOF;
    int order = 0;
    const int lineNum = 1;
    strOrFileP inputContainer = theS6ReadIterator->inputContainer;

    if ((firstChar = sf_getc(inputContainer)) == EOF)
    {
        gp_ErrorMessage("Unable to initialize reader: sparse6 input is empty.");
        return NOTOK;
    }

    if (firstChar == '>')
    {
        charConfirmation = sf_ungetc(firstChar, inputContainer);

        if (charConfirmation != firstChar)
        {
            gp_ErrorMessage("Unable to initialize reader due to failure to "
                            "ungetc first character.");
            return NOTOK;
        }

        if (_s6_ValidateHeader(inputContainer) != OK)
        {
            gp_ErrorMessage("Unable to initialize reader due to inability "
                            "to process and check sparse6 input header.");
            return NOTOK;
        }

        firstChar = sf_getc(inputContainer);
    }

    if (firstChar == ';')
    {
        gp_ErrorMessage("Line %d is an incremental sparse6 line (';'), which "
                        "cannot be the first graph in the input because there "
                        "is no previous graph for it to modify.",
                        lineNum);
        return NOTOK;
    }
    else if (firstChar == '&')
    {
        gp_ErrorMessage("Line %d is digraph6 format, which is not supported.",
                        lineNum);
        return NOTOK;
    }
    else if (firstChar != ':')
    {
        gp_ErrorMessage("Line %d does not begin with ':', so it is not a "
                        "sparse6 graph.",
                        lineNum);
        return NOTOK;
    }

    // Despite the general specification indicating that n \in [0, 68,719,476,735],
    // in practice n will be limited such that an integer will suffice in storing it.
    if (_s6_ReadOrder(inputContainer, &order, lineNum) != OK)
    {
        gp_ErrorMessage("Unable to initialize reader due to invalid graph "
                        "order on line %d of the sparse6 input.",
                        lineNum);
        return NOTOK;
    }

    if (gp_GetN(theS6ReadIterator->currGraph) == 0)
    {
        if (gp_EnsureVertexCapacity(theS6ReadIterator->currGraph, order) != OK)
        {
            gp_ErrorMessage("Unable to initialize reader due to failure "
                            "initializing graph datastructure with order %d "
                            "for graph on line %d of the sparse6 input.",
                            order, lineNum);
            return NOTOK;
        }
    }
    else
    {
        if (gp_GetN(theS6ReadIterator->currGraph) != order)
        {
            gp_ErrorMessage("Unable to initialize reader, as graph structure "
                            "passed in was already initialized with order "
                            "%d, which doesn't match the graph order %d "
                            "specified in the input.",
                            gp_GetN(theS6ReadIterator->currGraph), order);
            return NOTOK;
        }
        else
        {
            gp_ResetGraphStorage(theS6ReadIterator->currGraph);
        }
    }

    // Ensures zero-based flag is set regardless of whether the graph was initialized or reinitialized.
    theS6ReadIterator->currGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;

    theS6ReadIterator->edgeStamps = (long long *)calloc((size_t)order, sizeof(long long));

    if (theS6ReadIterator->edgeStamps == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for edgeStamps.");
        return NOTOK;
    }

    theS6ReadIterator->order = order;
    theS6ReadIterator->numBitsForVertex = _s6_GetNumBitsForVertex(order);
    theS6ReadIterator->firstLinePrefixConsumed = TRUE;

    return OK;
}

int _s6_ValidateHeader(strOrFileP inputContainer)
{
    char const *sparse6Header = ">>sparse6<<";
    char const *g6Header = ">>graph6<<";
    char const *digraph6Header = ">>digraph6<<";
    const size_t sparse6HeaderLen = strlen(sparse6Header);

    char headerCandidateChars[12];
    int theChar = EOF;

    if (inputContainer == NULL)
    {
        gp_ErrorMessage("Invalid sparse6 string-or-file container.");
        return NOTOK;
    }

    memset(headerCandidateChars, '\0', sizeof(headerCandidateChars));

    for (size_t i = 0; i < sparse6HeaderLen; i++)
    {
        if ((theChar = sf_getc(inputContainer)) == EOF)
            break;

        headerCandidateChars[i] = (char)theChar;
    }

    if (strcmp(sparse6Header, headerCandidateChars) != 0)
    {
        if (strncmp(g6Header, headerCandidateChars, strlen(g6Header)) == 0)
            gp_ErrorMessage("Input has a graph6 header, so it must be read "
                            "with the graph6 reader rather than the sparse6 "
                            "reader.");
        else if (strncmp(digraph6Header, headerCandidateChars, sparse6HeaderLen) == 0)
            gp_ErrorMessage("Input is digraph6 format, which is not "
                            "supported.");
        else
            gp_ErrorMessage("Invalid header for sparse6 input.");

        return NOTOK;
    }

    return OK;
}

int _s6_ReadOrder(strOrFileP inputContainer, int *order, const int lineNum)
{
    long long n = 0;
    int numGroups = 0;
    int orderChar = EOF;

    if (inputContainer == NULL || order == NULL)
    {
        gp_ErrorMessage("Invalid string-or-file container for sparse6 input.");
        return NOTOK;
    }

    // The order is encoded exactly as in graph6: one byte for n <= 62, 126
    // and three bytes carrying 18 bits for n <= 258047, and 126 126 and six
    // bytes carrying 36 bits beyond that. The order must fit an int; whether
    // a graph can have that many vertices is for the graph to decide.
    if ((orderChar = sf_getc(inputContainer)) == 126)
    {
        numGroups = 3;
        if ((orderChar = sf_getc(inputContainer)) == 126)
        {
            numGroups = 6;
            orderChar = sf_getc(inputContainer);
        }

        for (int i = 0; i < numGroups; i++)
        {
            if (i > 0)
                orderChar = sf_getc(inputContainer);

            if (orderChar < 63 || orderChar > 126)
            {
                gp_ErrorMessage("Invalid byte in the graph order on line %d; "
                                "expected a printable ASCII character in the "
                                "range 63 to 126.",
                                lineNum);
                return NOTOK;
            }

            n = (n << 6) | (orderChar - 63);
        }

        if (n > INT_MAX)
        {
            gp_ErrorMessage("Graph order %lld on line %d is larger than a "
                            "graph can have.",
                            n, lineNum);
            return NOTOK;
        }
    }
    else if (orderChar > 62 && orderChar < 126)
        n = orderChar - 63;
    else
    {
        gp_ErrorMessage("Invalid graph order on line %d; expected a printable "
                        "ASCII character in the range 63 to 126.",
                        lineNum);
        return NOTOK;
    }

    if (n == 0)
    {
        gp_ErrorMessage("Graph of order 0 on line %d is not supported.",
                        lineNum);
        return NOTOK;
    }

    (*order) = (int)n;

    return OK;
}

// The number of bits needed to represent order-1, which is the width of
// the x field of each (b, x) pair. It is zero for a graph of order 1,
// whose pairs then consist of the b bit alone.
int _s6_GetNumBitsForVertex(int order)
{
    int numBits = 0;

    for (int i = order - 1; i > 0; i >>= 1)
        numBits++;

    return numBits;
}

int s6_ReadGraph(S6ReadIteratorP theS6ReadIterator)
{
    strOrFileP inputContainer = NULL;
    graphP currGraph = NULL;
    int lineNum = 0;
    int firstChar = EOF;
    int order = 0;
    int incremental = FALSE;
    int endOfLine = FALSE;

    if (!_s6_IsReaderInitialized(theS6ReadIterator, TRUE))
    {
        gp_ErrorMessage("S6ReadIterator is not initialized.");
        return NOTOK;
    }

    if (theS6ReadIterator->readerFailed)
    {
        gp_ErrorMessage("Unable to read a graph, as an earlier read or "
                        "retrieval failed part way through a line.");
        return NOTOK;
    }

    if (theS6ReadIterator->endReached)
        return OK;

    if (theS6ReadIterator->numGraphsRead == INT_MAX)
    {
        gp_ErrorMessage("Unable to read more than %d graphs from one sparse6 "
                        "input.",
                        INT_MAX);
        return NOTOK;
    }

    inputContainer = theS6ReadIterator->inputContainer;
    currGraph = theS6ReadIterator->currGraph;
    lineNum = theS6ReadIterator->numGraphsRead + 1;

    if (theS6ReadIterator->firstLinePrefixConsumed)
    {
        // Initialization consumed the ':' and the order from line 1 and
        // left the graph empty, so only the edge list remains to be decoded
        theS6ReadIterator->firstLinePrefixConsumed = FALSE;
        _s6_StartLine(theS6ReadIterator);
    }
    else if (theS6ReadIterator->lineState == S6_LINE_INCREMENTAL_OPEN ||
             theS6ReadIterator->lineState == S6_LINE_INCREMENTAL_DONE)
    {
        // The lookahead has begun this ';' line, so the changes it has
        // retrieved are applied, and the edges it has not reached yet are
        // decoded below from where it stopped
        incremental = TRUE;

        if (_s6_IsGraphAsLastRead(theS6ReadIterator, lineNum) != OK ||
            _s6_ApplyRetrievedChanges(theS6ReadIterator) != OK)
        {
            theS6ReadIterator->readerFailed = TRUE;
            return NOTOK;
        }
    }
    else
    {
        // The lookahead may have consumed the first character of the line
        if (theS6ReadIterator->lineState == S6_LINE_END_NEXT)
            firstChar = EOF;
        else if (theS6ReadIterator->lineState == S6_LINE_WHOLE_NEXT)
            firstChar = ':';
        else
            firstChar = sf_getc(inputContainer);

        theS6ReadIterator->lineState = S6_LINE_NOT_STARTED;

        if (firstChar == EOF)
        {
            theS6ReadIterator->endReached = TRUE;
            return OK;
        }
        else if (firstChar == ':')
        {
            if (_s6_ReadOrder(inputContainer, &order, lineNum) != OK)
            {
                theS6ReadIterator->readerFailed = TRUE;
                return NOTOK;
            }

            // See the NOTE on _g6_ValidateOrderOfEncodedGraph() in
            // g6-api-utilities.c: all graphs in the input must have the
            // order of the first one
            if (order != theS6ReadIterator->order)
            {
                gp_ErrorMessage("Graph order %d on line %d doesn't match "
                                "expected graph order %d",
                                order, lineNum, theS6ReadIterator->order);
                theS6ReadIterator->readerFailed = TRUE;
                return NOTOK;
            }

            gp_ResetGraphStorage(currGraph);
            // Ensures zero-based flag is set after reinitializing graph.
            currGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;
        }
        else if (firstChar == ';')
        {
            // The line encodes the symmetric difference from the graph on
            // the previous line, so that graph is modified rather than reset
            incremental = TRUE;

            if (_s6_IsGraphAsLastRead(theS6ReadIterator, lineNum) != OK)
            {
                theS6ReadIterator->readerFailed = TRUE;
                return NOTOK;
            }
        }
        else
        {
            _s6_ReportLineStartError(firstChar, lineNum);
            theS6ReadIterator->readerFailed = TRUE;
            return NOTOK;
        }

        _s6_StartLine(theS6ReadIterator);
    }

    if (theS6ReadIterator->lineState != S6_LINE_INCREMENTAL_DONE &&
        _s6_DecodeEdges(theS6ReadIterator, FALSE, incremental, NULL, NULL, &endOfLine, lineNum) != OK)
    {
        gp_ErrorMessage("Unable to interpret bits on line %d to populate "
                        "the graph.",
                        lineNum);
        theS6ReadIterator->readerFailed = TRUE;
        return NOTOK;
    }

    // The deletions of an incremental line leave holes in the edge storage,
    // which are filled once here rather than after each deletion
    if (incremental && _CompactEdgeStorage(currGraph) != OK)
    {
        gp_ErrorMessage("Unable to keep the edge storage dense after applying "
                        "line %d.",
                        lineNum);
        theS6ReadIterator->readerFailed = TRUE;
        return NOTOK;
    }

    theS6ReadIterator->lineState = S6_LINE_NOT_STARTED;
    theS6ReadIterator->numChanges = 0;
    theS6ReadIterator->numGraphsRead = lineNum;
    theS6ReadIterator->countAtLastRead = theGraphModificationCount(currGraph);

    return OK;
}

/********************************************************************
 s6_RetrieveGraphChange()

 Tells the caller how the next graph differs from the one in the
 iterator's graph, one edge per call, without changing the graph. On
 a ';' line, each call decodes the next edge of the line: if the graph
 has the edge, e is set to an edge record of it, which the line
 deletes; otherwise u and v are set to its endpoints, u < v, which the
 line adds. The next s6_ReadGraph() applies the retrieved changes,
 then the rest of the line, so the caller may stop retrieving at any
 point. e, u and v are all set to NIL when no incremental change
 comes next: the next line is a ':' line or the first line, the input
 is at its end, or every edge of the ';' line has been retrieved.
 None of these consume a graph; that is still s6_ReadGraph()'s to do,
 including for a ';' line with no edges.

 A ';' line is the difference from the graph as last read, so from
 that read until the line has been read, the graph must not be changed
 other than by the reader; a change is detected, through the graph's
 modification counter, and refused, by this function and by
 s6_ReadGraph(), and so are the errors that s6_ReadGraph() would find
 in the part of the line retrieved. After a refusal the reader cannot
 go on.

 Returns OK on success, NOTOK otherwise.
 ********************************************************************/

int s6_RetrieveGraphChange(S6ReadIteratorP theS6ReadIterator, int *e, int *u, int *v)
{
    graphP theGraph = NULL;
    int lineNum = 0;
    int firstChar = EOF;
    int uFile = 0, vFile = 0;
    int endOfLine = FALSE;
    int eFound = NIL;
    int *change = NULL;

    if (e == NULL || u == NULL || v == NULL)
    {
        gp_ErrorMessage("Invalid parameter: e, u and v must be non-NULL.");
        return NOTOK;
    }

    (*e) = (*u) = (*v) = NIL;

    if (!_s6_IsReaderInitialized(theS6ReadIterator, TRUE))
    {
        gp_ErrorMessage("S6ReadIterator is not initialized.");
        return NOTOK;
    }

    if (theS6ReadIterator->readerFailed)
    {
        gp_ErrorMessage("Unable to retrieve a change, as an earlier read or "
                        "retrieval failed part way through a line.");
        return NOTOK;
    }

    // The first line is a ':' line, whose prefix initialization has read
    if (theS6ReadIterator->endReached || theS6ReadIterator->firstLinePrefixConsumed)
        return OK;

    theGraph = theS6ReadIterator->currGraph;

    // A line is only begun below if s6_ReadGraph() could read it, so a
    // line that is under way always has a line number
    if (theS6ReadIterator->numGraphsRead == INT_MAX)
    {
        gp_ErrorMessage("Unable to read more than %d graphs from one "
                        "sparse6 input.",
                        INT_MAX);
        return NOTOK;
    }

    lineNum = theS6ReadIterator->numGraphsRead + 1;

    if (theS6ReadIterator->lineState == S6_LINE_NOT_STARTED)
    {
        firstChar = sf_getc(theS6ReadIterator->inputContainer);

        if (firstChar == EOF)
        {
            theS6ReadIterator->lineState = S6_LINE_END_NEXT;
            return OK;
        }
        else if (firstChar == ':')
        {
            theS6ReadIterator->lineState = S6_LINE_WHOLE_NEXT;
            return OK;
        }
        else if (firstChar != ';')
        {
            _s6_ReportLineStartError(firstChar, lineNum);
            theS6ReadIterator->readerFailed = TRUE;
            return NOTOK;
        }

        _s6_StartLine(theS6ReadIterator);
        theS6ReadIterator->lineState = S6_LINE_INCREMENTAL_OPEN;
    }

    // Nothing depends on the graph before a ':' line or the end of the input
    if (theS6ReadIterator->lineState != S6_LINE_INCREMENTAL_OPEN &&
        theS6ReadIterator->lineState != S6_LINE_INCREMENTAL_DONE)
        return OK;

    if (_s6_IsGraphAsLastRead(theS6ReadIterator, lineNum) != OK)
    {
        theS6ReadIterator->readerFailed = TRUE;
        return NOTOK;
    }

    if (theS6ReadIterator->lineState == S6_LINE_INCREMENTAL_DONE)
        return OK;

    if (_s6_DecodeEdges(theS6ReadIterator, TRUE, TRUE, &uFile, &vFile, &endOfLine, lineNum) != OK)
    {
        theS6ReadIterator->readerFailed = TRUE;
        return NOTOK;
    }

    if (endOfLine)
    {
        theS6ReadIterator->lineState = S6_LINE_INCREMENTAL_DONE;
        return OK;
    }

    if (_s6_CheckEdge(theS6ReadIterator, uFile, vFile, TRUE, lineNum) != OK ||
        (theS6ReadIterator->numChanges == theS6ReadIterator->changeCapacity &&
         _s6_GrowRetrievedChanges(theS6ReadIterator) != OK))
    {
        theS6ReadIterator->readerFailed = TRUE;
        return NOTOK;
    }

    // The sparse6 file is 0-based, but in-memory storage may not be
    uFile += gp_LowerBoundVertexStorage(theGraph);
    vFile += gp_LowerBoundVertexStorage(theGraph);

    // Nothing is applied until s6_ReadGraph(), and an edge appears at most
    // once on a line, so the graph searched is the graph on the previous line
    eFound = gp_FindEdge(theGraph, uFile, vFile);
    change = theS6ReadIterator->changes + 3 * theS6ReadIterator->numChanges;

    if (gp_IsEdge(theGraph, eFound))
    {
        change[0] = S6_CHANGE_DELETE;
        change[1] = eFound;
        change[2] = NIL;
        (*e) = eFound;
    }
    else
    {
        change[0] = S6_CHANGE_ADD;
        change[1] = uFile;
        change[2] = vFile;
        (*u) = uFile;
        (*v) = vFile;
    }

    theS6ReadIterator->numChanges++;

    return OK;
}

// Reads the next byte of the edge list on the current line. On return,
// either endOfLine is set because the line ended (at a line terminator or
// at the end of the input), or byteBits holds the six data bits of the byte,
// i.e. its value less 63. Any other byte is an encoding error. A line
// terminator may be LF, CR or CRLF, as with the graph6 reader.
int _s6_ReadNextByte(strOrFileP inputContainer, int *byteBits, int *endOfLine, unsigned long long *bytePos, const int lineNum)
{
    int theChar = sf_getc(inputContainer);

    (*bytePos)++;

    if (theChar == EOF || theChar == '\n')
    {
        (*endOfLine) = TRUE;
        return OK;
    }

    if (theChar == '\r')
    {
        int nextChar = sf_getc(inputContainer);

        if (nextChar != '\n' && nextChar != EOF)
        {
            if (sf_ungetc(nextChar, inputContainer) != nextChar)
            {
                gp_ErrorMessage("Failure to ungetc the character after a "
                                "carriage return on line %d.",
                                lineNum);
                return NOTOK;
            }
        }

        (*endOfLine) = TRUE;
        return OK;
    }

    if (theChar < 63 || theChar > 126)
    {
        gp_ErrorMessage("Invalid character (byte value %d) at position %llu "
                        "of the edge list on line %d; sparse6 bytes must be "
                        "printable ASCII characters in the range 63 to 126.",
                        theChar, (*bytePos), lineNum);
        return NOTOK;
    }

    (*byteBits) = theChar - 63;

    return OK;
}

// Reports why a line that should begin a graph does not.
void _s6_ReportLineStartError(int firstChar, const int lineNum)
{
    if (firstChar == '\n' || firstChar == '\r')
        gp_ErrorMessage("Line %d is empty; expected a sparse6 graph "
                        "beginning with ':' or ';'.",
                        lineNum);
    else
        gp_ErrorMessage("Line %d does not begin with ':' or ';', so it "
                        "is not a sparse6 graph.",
                        lineNum);
}

// Sets the decoding state for the edge list of a new line, whose first
// character, and order for a ':' line, have been read, and empties the
// list of retrieved changes.
void _s6_StartLine(S6ReadIteratorP theS6ReadIterator)
{
    theS6ReadIterator->byteBits = 0;
    theS6ReadIterator->numBitsLeft = 0;
    theS6ReadIterator->currV = 0;
    theS6ReadIterator->bytePos = 0;
    theS6ReadIterator->numChanges = 0;
}

// Decodes (b, x) pairs of the edge list on the current line, following
// the decoding procedure of the format specification, from wherever the
// decoding state of the line stands. With stopAtEdge, it stops at the
// first pair that encodes an edge, which is returned as {u, v} with
// u <= v, for the lookahead; otherwise it applies each edge to the graph
// until the line ends, which is how s6_ReadGraph() reads a line. The bits
// of a pair may span byte boundaries, and an incomplete pair at the end
// of the line is padding. The decoding state is kept in locals while the
// pairs are decoded, and in the iterator between calls.
int _s6_DecodeEdges(S6ReadIteratorP theS6ReadIterator, const int stopAtEdge, const int incremental,
                    int *pU, int *pV, int *pEndOfLine, const int lineNum)
{
    strOrFileP inputContainer = theS6ReadIterator->inputContainer;
    const int order = theS6ReadIterator->order;
    const int numBitsForVertex = theS6ReadIterator->numBitsForVertex;

    int byteBits = theS6ReadIterator->byteBits;
    int numBitsLeft = theS6ReadIterator->numBitsLeft;
    int v = theS6ReadIterator->currV;
    unsigned long long bytePos = theS6ReadIterator->bytePos;
    int endOfLine = FALSE;
    int Result = OK;

    int x = 0;
    int numBitsNeeded = 0;

    while (TRUE)
    {
        // The b bit of the next pair
        if (numBitsLeft == 0)
        {
            if (_s6_ReadNextByte(inputContainer, &byteBits, &endOfLine, &bytePos, lineNum) != OK)
            {
                Result = NOTOK;
                break;
            }

            if (endOfLine)
                break;

            numBitsLeft = 6;
        }

        numBitsLeft--;
        // Once v has reached the order, every remaining pair is padding, so
        // v is left where it is rather than counted up without bound
        if (((byteBits >> numBitsLeft) & 1) && v < order)
            v++;

        // The x field of the pair, which is numBitsForVertex bits wide
        x = 0;
        numBitsNeeded = numBitsForVertex;

        while (numBitsNeeded > 0)
        {
            if (numBitsLeft == 0)
            {
                if (_s6_ReadNextByte(inputContainer, &byteBits, &endOfLine, &bytePos, lineNum) != OK)
                {
                    Result = NOTOK;
                    break;
                }

                if (endOfLine)
                    break;

                numBitsLeft = 6;
            }

            if (numBitsNeeded >= numBitsLeft)
            {
                x = (x << numBitsLeft) | (byteBits & ((1 << numBitsLeft) - 1));
                numBitsNeeded -= numBitsLeft;
                numBitsLeft = 0;
            }
            else
            {
                numBitsLeft -= numBitsNeeded;
                x = (x << numBitsNeeded) | ((byteBits >> numBitsLeft) & ((1 << numBitsNeeded) - 1));
                numBitsNeeded = 0;
            }
        }

        // An incomplete pair at the end of the line is padding
        if (Result != OK || endOfLine)
            break;

        if (x > v)
            v = x;
        else if (v < order)
        {
            if (stopAtEdge)
            {
                (*pU) = x;
                (*pV) = v;
                break;
            }

            if (_s6_ApplyEdge(theS6ReadIterator, x, v, incremental, lineNum) != OK)
            {
                Result = NOTOK;
                break;
            }
        }
        // else the pair is padding that pushed v to the order or beyond
    }

    theS6ReadIterator->byteBits = byteBits;
    theS6ReadIterator->numBitsLeft = numBitsLeft;
    theS6ReadIterator->currV = v;
    theS6ReadIterator->bytePos = bytePos;
    (*pEndOfLine) = endOfLine;

    return Result;
}

// Refuses a decoded pair {u, v}, u <= v, that the graph cannot take. The
// graph library does not support loop edges, and parallel edges are not
// supported by the algorithms, so both are reported as errors rather than
// silently dropped or doubled. A pair that occurs twice on one line is a
// parallel edge on a ':' line; on a ';' line, which lists the symmetric
// difference from the previous graph, it would toggle the edge twice,
// which the writer never produces and the lookahead could not report,
// since the second toggle would undo a change already retrieved. Either
// way the repeat is found through edgeStamps in constant time.
int _s6_CheckEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum)
{
    long long edgeStamp = (long long)lineNum * (theS6ReadIterator->order + 1) + v + 1;

    if (u == v)
    {
        gp_ErrorMessage("Loop edge on vertex %d on line %d is not supported.",
                        u, lineNum);
        return NOTOK;
    }

    if (theS6ReadIterator->edgeStamps[u] == edgeStamp)
    {
        if (incremental)
            gp_ErrorMessage("Edge between vertices %d and %d appears twice on "
                            "incremental line %d, which lists each changed "
                            "edge once.",
                            u, v, lineNum);
        else
            gp_ErrorMessage("Parallel edge between vertices %d and %d on line "
                            "%d is not supported.",
                            u, v, lineNum);
        return NOTOK;
    }

    theS6ReadIterator->edgeStamps[u] = edgeStamp;

    return OK;
}

// Applies the decoded pair {u, v}, with u <= v, to the graph. On a ':' line
// the pair adds an edge; on a ';' line it toggles the edge, since the line
// is the symmetric difference from the previous graph, so the edge is
// looked up in the adjacency list of u to know whether the toggle adds or
// deletes.
int _s6_ApplyEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum)
{
    graphP theGraph = theS6ReadIterator->currGraph;
    // The sparse6 file is 0-based, but in-memory storage may not be
    int uStorage = u + gp_LowerBoundVertexStorage(theGraph);
    int vStorage = v + gp_LowerBoundVertexStorage(theGraph);

    if (_s6_CheckEdge(theS6ReadIterator, u, v, incremental, lineNum) != OK)
        return NOTOK;

    if (incremental)
    {
        int e = gp_FindEdge(theGraph, uStorage, vStorage);

        if (gp_IsEdge(theGraph, e))
        {
            // The deletion leaves a hole in the edge storage unless the pair
            // it removed was the last one; s6_ReadGraph() compacts the
            // storage once the whole line has been applied, since a freshly
            // read graph must have no holes for algorithms such as DrawPlanar
            return gp_DeleteEdge(theGraph, e);
        }
    }

    return gp_DynamicAddEdge(theGraph, uStorage, 0, vStorage, 0);
}

// Doubles the capacity of the list of retrieved changes.
int _s6_GrowRetrievedChanges(S6ReadIteratorP theS6ReadIterator)
{
    long long newCapacity = (theS6ReadIterator->changeCapacity == 0)
                                ? S6_INITIAL_CHANGE_CAPACITY
                                : (long long)theS6ReadIterator->changeCapacity * 2;
    int *newChanges = NULL;

    if (newCapacity > INT_MAX / 3 ||
        (size_t)newCapacity > SIZE_MAX / (3 * sizeof(int)))
    {
        gp_ErrorMessage("Unable to retrieve more than %d changes from one "
                        "incremental sparse6 line.",
                        theS6ReadIterator->changeCapacity);
        return NOTOK;
    }

    newChanges = (int *)realloc(theS6ReadIterator->changes, (size_t)newCapacity * 3 * sizeof(int));
    if (newChanges == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for the changes retrieved "
                        "by the sparse6 reader.");
        return NOTOK;
    }

    theS6ReadIterator->changes = newChanges;
    theS6ReadIterator->changeCapacity = (int)newCapacity;

    return OK;
}

// Refuses a ';' line if the graph has been modified since the last graph
// was read, since the line is the difference from that graph. Every
// function that modifies a graph advances its modification counter.
int _s6_IsGraphAsLastRead(S6ReadIteratorP theS6ReadIterator, const int lineNum)
{
    if (theGraphModificationCount(theS6ReadIterator->currGraph) != theS6ReadIterator->countAtLastRead)
    {
        gp_ErrorMessage("Unable to apply incremental line %d, as the graph "
                        "was modified after the previous graph was read.",
                        lineNum);
        return NOTOK;
    }

    return OK;
}

// Applies the changes retrieved from the current ';' line to the graph, in
// the order they were retrieved. Holes left by the deletions are compacted
// by s6_ReadGraph() once the whole line is applied; until then no edge in
// use moves, so each retrieved edge record still names its edge.
int _s6_ApplyRetrievedChanges(S6ReadIteratorP theS6ReadIterator)
{
    graphP theGraph = theS6ReadIterator->currGraph;

    for (int i = 0; i < theS6ReadIterator->numChanges; i++)
    {
        int const *change = theS6ReadIterator->changes + 3 * i;

        if (change[0] == S6_CHANGE_DELETE)
        {
            if (gp_DeleteEdge(theGraph, change[1]) != OK)
                return NOTOK;
        }
        else if (gp_DynamicAddEdge(theGraph, change[1], 0, change[2], 0) != OK)
            return NOTOK;
    }

    theS6ReadIterator->numChanges = 0;

    return OK;
}

void s6_FreeReader(S6ReadIteratorP *pS6ReadIterator)
{
    if (pS6ReadIterator != NULL && (*pS6ReadIterator) != NULL)
    {
        if ((*pS6ReadIterator)->inputContainer != NULL)
            sf_Free(&((*pS6ReadIterator)->inputContainer));

        (*pS6ReadIterator)->numGraphsRead = 0;
        (*pS6ReadIterator)->order = 0;
        (*pS6ReadIterator)->numBitsForVertex = 0;

        if ((*pS6ReadIterator)->edgeStamps != NULL)
        {
            free((*pS6ReadIterator)->edgeStamps);
            (*pS6ReadIterator)->edgeStamps = NULL;
        }

        if ((*pS6ReadIterator)->changes != NULL)
        {
            free((*pS6ReadIterator)->changes);
            (*pS6ReadIterator)->changes = NULL;
        }

        // N.B. The S6ReadIterator doesn't "own" the graph, so we don't free it.
        (*pS6ReadIterator)->currGraph = NULL;

        free((*pS6ReadIterator));
        (*pS6ReadIterator) = NULL;
    }
}

int _s6_ReadGraphFromFile(graphP theGraph, char *pathToS6File)
{
    strOrFileP inputContainer = NULL;

    if (pathToS6File == NULL || strlen(pathToS6File) == 0)
    {
        gp_ErrorMessage("Unable to read graph from file, as pathToS6File is "
                        "NULL or empty string.");
        return NOTOK;
    }

    if ((inputContainer = sf_NewInputContainer(NULL, pathToS6File)) == NULL)
    {
        gp_ErrorMessage("Unable to allocate strOrFile container for infile "
                        "\"%.*s\".",
                        FILENAME_MAX, pathToS6File);
        return NOTOK;
    }

    return _s6_ReadGraphFromStrOrFile(theGraph, (&inputContainer));
}

int _s6_ReadGraphFromString(graphP theGraph, char *s6EncodedString)
{
    strOrFileP inputContainer = NULL;

    if (s6EncodedString == NULL || strlen(s6EncodedString) == 0)
    {
        gp_ErrorMessage("Unable to proceed with empty sparse6 input string.");
        return NOTOK;
    }

    if ((inputContainer = sf_NewInputContainer(s6EncodedString, NULL)) == NULL)
    {
        gp_ErrorMessage("Unable to allocate strOrFile container for sparse6 "
                        "input string.");
        return NOTOK;
    }

    return _s6_ReadGraphFromStrOrFile(theGraph, (&inputContainer));
}

// Reads the first graph of the sparse6 input into theGraph. As with the
// graph6 reader, the read iterator takes ownership of the input container,
// so (*pInputContainer) is NULL after this call, whether or not it succeeds.
int _s6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *pInputContainer)
{
    S6ReadIteratorP theS6ReadIterator = NULL;
    int Result = OK;

    if (pInputContainer == NULL || !sf_IsValidStrOrFile((*pInputContainer)))
    {
        gp_ErrorMessage("Invalid sparse6 input container.");
        return NOTOK;
    }

    if (s6_NewReader((&theS6ReadIterator), theGraph) != OK)
    {
        gp_ErrorMessage("Unable to allocate S6ReadIterator.");
        sf_Free(pInputContainer);
        return NOTOK;
    }

    if (_s6_InitReaderWithStrOrFile(theS6ReadIterator, pInputContainer) != OK)
    {
        gp_ErrorMessage("Unable to initialize S6ReadIterator.");
        s6_FreeReader((&theS6ReadIterator));
        return NOTOK;
    }

    if (s6_ReadGraph(theS6ReadIterator) != OK)
    {
        gp_ErrorMessage("Unable to read graph from sparse6 read iterator.");
        Result = NOTOK;
    }

    s6_FreeReader((&theS6ReadIterator));

    return Result;
}
