/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "strOrFile.h"

#include "s6-read-iterator.h"

// For definition of zero-based IO flag
#include "graphIO.h"

/* Private function declarations (exported within system) */
int _s6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *pInputContainer);

/* Private functions */
int _s6_InitReaderWithStrOrFile(S6ReadIteratorP theS6ReadIterator, strOrFileP *pInputContainer);
int _s6_InitReader(S6ReadIteratorP theS6ReadIterator);
int _s6_IsReaderInitialized(S6ReadIteratorP theS6ReadIterator, int reportUninitializedParts);
int _s6_ValidateHeader(strOrFileP inputContainer);
int _s6_ReadOrder(strOrFileP inputContainer, int *order, const int lineNum);
int _s6_GetNumBitsForVertex(int order);
int _s6_ReadNextByte(strOrFileP inputContainer, int *byteBits, int *endOfLine, unsigned long long *bytePos, const int lineNum);
int _s6_DecodeEdgeList(S6ReadIteratorP theS6ReadIterator, const int incremental, const int lineNum);
int _s6_ApplyEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum);

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
 ********************************************************************/
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
};

/********************************************************************
 Public and package private method implementations for read iterator
 ********************************************************************/

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

    // numGraphsRead, order, numBitsForVertex, firstLinePrefixConsumed and
    // endReached all set to 0
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
    int n = 0;
    int orderChar = EOF;

    if (inputContainer == NULL || order == NULL)
    {
        gp_ErrorMessage("Invalid string-or-file container for sparse6 input.");
        return NOTOK;
    }

    // The order is encoded exactly as in graph6: one byte for n <= 62, four
    // bytes beginning with 126 for n <= 258047, and eight bytes beginning
    // with 126 126 beyond that. Since edge-addition-planarity-suite processing
    // may only handle up to n = 100,000, we will only check if 1 or 4 bytes
    // are necessary
    if ((orderChar = sf_getc(inputContainer)) == 126)
    {
        int orderGroups[3];

        if ((orderChar = sf_getc(inputContainer)) == 126)
        {
            gp_ErrorMessage("Graphs of order n > 100000 are not supported at "
                            "this time (line %d).",
                            lineNum);
            return NOTOK;
        }

        orderGroups[0] = orderChar;
        orderGroups[1] = sf_getc(inputContainer);
        orderGroups[2] = sf_getc(inputContainer);

        for (int i = 0; i < 3; i++)
        {
            if (orderGroups[i] < 63 || orderGroups[i] > 126)
            {
                gp_ErrorMessage("Invalid byte in the graph order on line %d; "
                                "expected a printable ASCII character in the "
                                "range 63 to 126.",
                                lineNum);
                return NOTOK;
            }

            n = (n << 6) | (orderGroups[i] - 63);
        }

        if (n > 100000)
        {
            gp_ErrorMessage("Graph order %d on line %d is greater than 100000, "
                            "which is not supported at this time.",
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

    (*order) = n;

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

    if (!_s6_IsReaderInitialized(theS6ReadIterator, TRUE))
    {
        gp_ErrorMessage("S6ReadIterator is not initialized.");
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
    }
    else
    {
        firstChar = sf_getc(inputContainer);

        if (firstChar == EOF)
        {
            theS6ReadIterator->endReached = TRUE;
            return OK;
        }
        else if (firstChar == ':')
        {
            if (_s6_ReadOrder(inputContainer, &order, lineNum) != OK)
                return NOTOK;

            // See the NOTE on _g6_ValidateOrderOfEncodedGraph() in
            // g6-api-utilities.c: all graphs in the input must have the
            // order of the first one
            if (order != theS6ReadIterator->order)
            {
                gp_ErrorMessage("Graph order %d on line %d doesn't match "
                                "expected graph order %d",
                                order, lineNum, theS6ReadIterator->order);
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
        }
        else if (firstChar == '\n' || firstChar == '\r')
        {
            gp_ErrorMessage("Line %d is empty; expected a sparse6 graph "
                            "beginning with ':' or ';'.",
                            lineNum);
            return NOTOK;
        }
        else
        {
            gp_ErrorMessage("Line %d does not begin with ':' or ';', so it "
                            "is not a sparse6 graph.",
                            lineNum);
            return NOTOK;
        }
    }

    if (_s6_DecodeEdgeList(theS6ReadIterator, incremental, lineNum) != OK)
    {
        gp_ErrorMessage("Unable to interpret bits on line %d to populate "
                        "the graph.",
                        lineNum);
        return NOTOK;
    }

    theS6ReadIterator->numGraphsRead = lineNum;

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

// Decodes the (b, x) pairs of the edge list on the current line into the
// graph, following the decoding procedure of the format specification. The
// bits of a pair may span byte boundaries, and an incomplete pair at the
// end of the line is padding.
int _s6_DecodeEdgeList(S6ReadIteratorP theS6ReadIterator, const int incremental, const int lineNum)
{
    strOrFileP inputContainer = theS6ReadIterator->inputContainer;
    const int order = theS6ReadIterator->order;
    const int numBitsForVertex = theS6ReadIterator->numBitsForVertex;

    int byteBits = 0;
    int numBitsLeft = 0;
    int endOfLine = FALSE;
    unsigned long long bytePos = 0;

    int v = 0;
    int x = 0;
    int numBitsNeeded = 0;

    while (TRUE)
    {
        // The b bit of the next pair
        if (numBitsLeft == 0)
        {
            if (_s6_ReadNextByte(inputContainer, &byteBits, &endOfLine, &bytePos, lineNum) != OK)
                return NOTOK;

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
                    return NOTOK;

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
        if (endOfLine)
            break;

        if (x > v)
            v = x;
        else if (v < order)
        {
            if (_s6_ApplyEdge(theS6ReadIterator, x, v, incremental, lineNum) != OK)
                return NOTOK;
        }
        // else the pair is padding that pushed v to the order or beyond
    }

    return OK;
}

// Applies the decoded pair {u, v}, with u <= v, to the graph. On a ':' line
// the pair adds an edge; on a ';' line it toggles the edge, since the line
// is the symmetric difference from the previous graph. The graph library
// does not support loop edges, and parallel edges are not supported by the
// algorithms, so both are reported as errors rather than silently dropped
// or doubled. A repeated edge on a ':' line is found through edgeStamps in
// constant time; on a ';' line a repeat is the second of two toggles, and
// the edge is looked up in the adjacency list of u, as it must be to know
// whether the toggle adds or deletes.
int _s6_ApplyEdge(S6ReadIteratorP theS6ReadIterator, int u, int v, const int incremental, const int lineNum)
{
    graphP theGraph = theS6ReadIterator->currGraph;
    // The sparse6 file is 0-based, but in-memory storage may not be
    int uStorage = u + gp_LowerBoundVertexStorage(theGraph);
    int vStorage = v + gp_LowerBoundVertexStorage(theGraph);

    if (u == v)
    {
        gp_ErrorMessage("Loop edge on vertex %d on line %d is not supported.",
                        u, lineNum);
        return NOTOK;
    }

    if (incremental)
    {
        int e = gp_FindEdge(theGraph, uStorage, vStorage);

        if (gp_IsEdge(theGraph, e))
            return gp_DeleteEdge(theGraph, e);
    }
    else
    {
        long long edgeStamp = (long long)lineNum * (theS6ReadIterator->order + 1) + v + 1;

        if (theS6ReadIterator->edgeStamps[u] == edgeStamp)
        {
            gp_ErrorMessage("Parallel edge between vertices %d and %d on line "
                            "%d is not supported.",
                            u, v, lineNum);
            return NOTOK;
        }

        theS6ReadIterator->edgeStamps[u] = edgeStamp;
    }

    return gp_DynamicAddEdge(theGraph, uStorage, 0, vStorage, 0);
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
