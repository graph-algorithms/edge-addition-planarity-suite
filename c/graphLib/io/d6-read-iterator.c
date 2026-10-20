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

#include "d6-read-iterator.h"

// For definition of zero-based IO flag
#include "graphIO.h"

/* Imported functions */
// digraph6 encodes the order of a graph as graph6 does, so the reader of
// the order and the check of a later graph's order are shared
extern int _g6_GetNumCharsForOrder(int order);
extern int _g6_DetermineOrderFromInput(strOrFileP inputContainer, int *order);

/* Private function declarations (exported within system) */
int _d6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *pInputContainer);
size_t _d6_GetNumCharsForEncoding(int order);
void _d6_EncodeOrder(char *orderChars, int order);

/* Private functions */
int _d6_GetArcBit(char const *graphBuff, int order, int u, int v);
int _d6_InitReaderWithStrOrFile(D6ReadIteratorP theD6ReadIterator, strOrFileP *pInputContainer);
int _d6_InitReader(D6ReadIteratorP theD6ReadIterator);
int _d6_IsReaderInitialized(D6ReadIteratorP theD6ReadIterator, int reportUninitializedParts);
int _d6_ValidateHeader(strOrFileP inputContainer);
int _d6_ValidateGraphEncoding(char *graphBuff, const int order, const size_t numChars, const int lineNum);
int _d6_DecodeGraph(char *graphBuff, const int order, graphP theGraph);

/********************************************************************
 Package private structure declaration for read iterator
 ********************************************************************/
typedef struct strOrFileStruct strOrFileStruct;
typedef strOrFileStruct *strOrFileP;

struct D6ReadIteratorStruct
{
    strOrFileP inputContainer;
    int numGraphsRead;

    int order;
    int numCharsForOrder;
    size_t numCharsForGraphEncoding;
    size_t currGraphBuffSize;
    char *currGraphBuff;

    graphP currGraph;

    int endReached;
};

/********************************************************************
 Public and package private method implementations for read iterator
 ********************************************************************/

/********************************************************************
 d6_IsDigraph6Input()

 Returns TRUE if the given first line of an input can be digraph6
 content, i.e. it starts with the optional ">>digraph6<<" header or
 with the '&' that begins each digraph6 line, followed by a byte in the
 range 63 to 126, which is how digraph6 encodes the order of the graph.
 Returns FALSE for a NULL or empty first line and for the other formats.
 ********************************************************************/
int d6_IsDigraph6Input(char const *const firstLine)
{
    int secondChar = '\0';

    if (firstLine == NULL)
        return FALSE;

    if (strncmp(firstLine, ">>digraph6<<", strlen(">>digraph6<<")) == 0)
        return TRUE;

    if (firstLine[0] != '&')
        return FALSE;

    secondChar = (unsigned char)firstLine[1];
    return (secondChar >= 63 && secondChar <= 126) ? TRUE : FALSE;
}

int d6_NewReader(D6ReadIteratorP *pD6ReadIterator, graphP theGraph)
{
    if (pD6ReadIterator == NULL)
    {
        gp_ErrorMessage("Unable to allocate D6ReadIterator, as pointer to "
                        "which to assign address of memory allocated for "
                        "D6ReadIterator is NULL.");
        return NOTOK;
    }

    if ((*pD6ReadIterator) != NULL)
    {
        gp_ErrorMessage("D6ReadIterator is not NULL and therefore can't be "
                        "allocated.");
        return NOTOK;
    }

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Must allocate graph to be used by D6ReadIterator.");
        return NOTOK;
    }

    // numGraphsRead, order, numCharsForOrder, numCharsForGraphEncoding,
    // currGraphBuffSize and endReached all set to 0
    (*pD6ReadIterator) = (D6ReadIteratorP)calloc(1, sizeof(D6ReadIteratorStruct));

    if ((*pD6ReadIterator) == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for D6ReadIterator.");
        return NOTOK;
    }

    (*pD6ReadIterator)->inputContainer = NULL;
    (*pD6ReadIterator)->currGraphBuff = NULL;
    (*pD6ReadIterator)->currGraph = theGraph;

    return OK;
}

int _d6_IsReaderInitialized(D6ReadIteratorP theD6ReadIterator, int reportUninitializedParts)
{
    int readerInitialized = TRUE;

    if (theD6ReadIterator == NULL)
    {
        if (reportUninitializedParts)
            gp_ErrorMessage("D6ReadIterator is NULL.");
        readerInitialized = FALSE;
    }
    else
    {
        // The reader is uninitialized if the input source is invalid,
        // except if it was closed due to its end having been reached.
        if (!theD6ReadIterator->endReached && !sf_IsValidStrOrFile(theD6ReadIterator->inputContainer))
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6ReadIterator's inputContainer string-or-file "
                                "container is not valid.");
            readerInitialized = FALSE;
        }
        if (theD6ReadIterator->currGraphBuff == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6ReadIterator's currGraphBuff is NULL.");
            readerInitialized = FALSE;
        }
        if (theD6ReadIterator->currGraph == NULL)
        {
            if (reportUninitializedParts)
                gp_ErrorMessage("D6ReadIterator's currGraph is NULL.");
            readerInitialized = FALSE;
        }
    }

    return readerInitialized;
}

int d6_EndReached(D6ReadIteratorP theD6ReadIterator)
{
    if (theD6ReadIterator == NULL)
        return TRUE;

    return theD6ReadIterator->endReached;
}

int d6_InitReaderWithString(D6ReadIteratorP theD6ReadIterator, char *inputString)
{
    strOrFileP inputContainer = NULL;

    if (theD6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_d6_IsReaderInitialized(theD6ReadIterator, FALSE))
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

    return _d6_InitReaderWithStrOrFile(theD6ReadIterator, (&inputContainer));
}

int d6_InitReaderWithFileName(D6ReadIteratorP theD6ReadIterator, char const *const infileName)
{
    strOrFileP inputContainer = NULL;

    if (theD6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (_d6_IsReaderInitialized(theD6ReadIterator, FALSE))
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

    return _d6_InitReaderWithStrOrFile(theD6ReadIterator, (&inputContainer));
}

int _d6_InitReaderWithStrOrFile(D6ReadIteratorP theD6ReadIterator, strOrFileP *pInputContainer)
{
    if (theD6ReadIterator == NULL)
    {
        gp_ErrorMessage("Invalid parameter: theD6ReadIterator must be non-NULL.");
        return NOTOK;
    }

    if (pInputContainer == NULL || !sf_IsValidStrOrFile((*pInputContainer)))
    {
        gp_ErrorMessage("Unable to initialize reader with invalid strOrFile "
                        "input container.");
        return NOTOK;
    }

    theD6ReadIterator->inputContainer = (*pInputContainer);
    // We have taken ownership of the inputContainer, and so we have set the
    // caller's pointer to NULL. The reader is responsible for freeing this
    // input container.
    (*pInputContainer) = NULL;

    // A reader that fails to initialize frees the input it took, so that it
    // can be initialized again without losing it
    if (_d6_InitReader(theD6ReadIterator) != OK)
    {
        sf_Free(&theD6ReadIterator->inputContainer);
        if (theD6ReadIterator->currGraphBuff != NULL)
        {
            free(theD6ReadIterator->currGraphBuff);
            theD6ReadIterator->currGraphBuff = NULL;
        }
        return NOTOK;
    }

    return OK;
}

// Reads the optional header, the '&' of the first line and the order of
// its graph, which every later line must repeat, and sizes the line
// buffer for that order. The rest of the first line is left for
// d6_ReadGraph().
int _d6_InitReader(D6ReadIteratorP theD6ReadIterator)
{
    int charConfirmation = EOF;
    int firstChar = '\0';
    int order = NIL;
    strOrFileP inputContainer = theD6ReadIterator->inputContainer;

    if ((firstChar = sf_getc(inputContainer)) == EOF)
    {
        // if inputContainer->inputErrorFlag, then sf_getc() also returns EOF
        gp_ErrorMessage("Unexpected end of file or read error.");
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

        if (_d6_ValidateHeader(inputContainer) != OK)
        {
            gp_ErrorMessage("Unable to initialize reader due to inability "
                            "to process and check .d6 infile header.");
            return NOTOK;
        }

        // The first graph follows the header on the same line
        if ((firstChar = sf_getc(inputContainer)) == EOF)
        {
            gp_ErrorMessage("Unexpected end of file or read error.");
            return NOTOK;
        }
    }

    if (firstChar != '&')
    {
        gp_ErrorMessage("Line 1 does not begin with '&', so it is not a "
                        "digraph6 line.");
        return NOTOK;
    }

    // Despite the general specification indicating that n \in [0, 68,719,476,735],
    // in practice n will be limited such that an integer will suffice in storing it.
    if (_g6_DetermineOrderFromInput(inputContainer, &order) != OK)
    {
        gp_ErrorMessage("Unable to initialize reader due to invalid graph "
                        "order on line 1 of .d6 input.");
        return NOTOK;
    }

    if (gp_GetN(theD6ReadIterator->currGraph) == 0)
    {
        if (gp_EnsureVertexCapacity(theD6ReadIterator->currGraph, order) != OK)
        {
            gp_ErrorMessage("Unable to initialize reader due to failure "
                            "initializing graph datastructure with order %d "
                            "for graph on line 1 of the .d6 input.",
                            order);
            return NOTOK;
        }
    }
    else if (gp_GetN(theD6ReadIterator->currGraph) != order)
    {
        gp_ErrorMessage("Unable to initialize reader, as graph structure "
                        "passed in was already initialized with order "
                        "%d, which doesn't match the graph order %d "
                        "specified in the input.",
                        gp_GetN(theD6ReadIterator->currGraph), order);
        return NOTOK;
    }
    else
        gp_ResetGraphStorage(theD6ReadIterator->currGraph);

    theD6ReadIterator->order = order;

    // Ensures zero-based flag is set regardless of whether the graph was initialized or reinitialized.
    theD6ReadIterator->currGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;

    theD6ReadIterator->numCharsForOrder = _g6_GetNumCharsForOrder(order);
    theD6ReadIterator->numCharsForGraphEncoding = _d6_GetNumCharsForEncoding(order);

    // A line is '&', the order and the encoding, read with sf_fgets(),
    // which takes an int size
    if (theD6ReadIterator->numCharsForGraphEncoding == 0 ||
        theD6ReadIterator->numCharsForGraphEncoding > (size_t)INT_MAX - 1 - theD6ReadIterator->numCharsForOrder - 3)
    {
        gp_ErrorMessage("Graphs of order %d are too large to read.", order);
        return NOTOK;
    }

    // Must add 3 bytes for newline, possible carriage return, and null terminator
    theD6ReadIterator->currGraphBuffSize = 1 + theD6ReadIterator->numCharsForOrder + theD6ReadIterator->numCharsForGraphEncoding + 3;
    theD6ReadIterator->currGraphBuff = (char *)calloc(theD6ReadIterator->currGraphBuffSize, sizeof(char));

    if (theD6ReadIterator->currGraphBuff == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for currGraphBuff.");
        return NOTOK;
    }

    return OK;
}

int _d6_ValidateHeader(strOrFileP inputContainer)
{
    int intChar = 0;
    char const *d6Header = ">>digraph6<<";
    char headerCandidateChars[13];

    if (inputContainer == NULL)
    {
        gp_ErrorMessage("Invalid .d6 string-or-file container.");
        return NOTOK;
    }

    for (int i = 0; i < 12; i++)
    {
        intChar = sf_getc(inputContainer);
        if (intChar == EOF)
        {
            gp_ErrorMessage("Truncated >>digraph6<< header.");
            return NOTOK;
        }
        headerCandidateChars[i] = (char)intChar;
    }

    headerCandidateChars[12] = '\0';

    if (strcmp(d6Header, headerCandidateChars) != 0)
    {
        gp_ErrorMessage("Invalid header for .d6 file.");
        return NOTOK;
    }

    return OK;
}

/********************************************************************
 _d6_GetNumCharsForEncoding()

 Returns the number of bytes that encode the n*n bits of the adjacency
 matrix of a digraph of the given order, six bits to a byte. An order
 is at most 100000, so n*n fits in a size_t of 64 bits; with a size_t
 of 32 bits a larger order is refused by returning 0.
 ********************************************************************/
size_t _d6_GetNumCharsForEncoding(int order)
{
    size_t numBits = 0;

    if (order < 1 || (size_t)order > SIZE_MAX / (size_t)order)
        return 0;

    numBits = (size_t)order * (size_t)order;
    return (numBits / 6) + (numBits % 6 ? 1 : 0);
}

/********************************************************************
 _d6_EncodeOrder()

 Writes the bytes that encode the given order, 1 to 100000, as graph6
 encodes it: one byte for an order up to 62, otherwise 126 followed by
 the 18 bits of the order in three bytes. orderChars receives one or
 four bytes, as _g6_GetNumCharsForOrder() says, and no null terminator.
 ********************************************************************/
void _d6_EncodeOrder(char *orderChars, int order)
{
    if (order > 62)
    {
        orderChars[0] = 126;
        for (int i = 0; i < 3; i++)
            orderChars[3 - i] = (char)(((order >> (6 * i)) & 63) + 63);
    }
    else
        orderChars[0] = (char)(order + 63);
}

// Returns 1 if the encoding in graphBuff, whose bytes still carry the
// offset of 63, has the arc u -> v, i.e. bit u*order+v of the adjacency
// matrix, and 0 otherwise. The bits of each byte are read from its most
// significant one.
int _d6_GetArcBit(char const *graphBuff, int order, int u, int v)
{
    size_t bitIndex = (size_t)u * (size_t)order + (size_t)v;
    int sixBits = (unsigned char)graphBuff[bitIndex / 6] - 63;

    return (sixBits >> (5 - (int)(bitIndex % 6))) & 1;
}

int d6_ReadGraph(D6ReadIteratorP theD6ReadIterator)
{
    strOrFileP inputContainer = NULL;
    int lineNum = 0;
    char *currGraphBuff = NULL;
    char *graphEncodingChars = NULL;
    size_t expectedLength = 0;
    graphP currGraph = NULL;

    if (!_d6_IsReaderInitialized(theD6ReadIterator, TRUE))
    {
        gp_ErrorMessage("D6ReadIterator is not initialized.");
        return NOTOK;
    }

    // Once the end is read the input is closed, and a failure to close it
    // leaves the input error flag set
    if (theD6ReadIterator->endReached)
        return theD6ReadIterator->inputContainer->inputErrorFlag ? NOTOK : OK;

    inputContainer = theD6ReadIterator->inputContainer;
    lineNum = theD6ReadIterator->numGraphsRead + 1;
    currGraphBuff = theD6ReadIterator->currGraphBuff;
    currGraph = theD6ReadIterator->currGraph;

    if (sf_fgets(currGraphBuff, (int)theD6ReadIterator->currGraphBuffSize, inputContainer) != NULL)
    {
        currGraphBuff[strcspn(currGraphBuff, "\n\r")] = '\0'; // works for LF, CR, CRLF, LFCR, ...

        // The '&' and the order of the first graph were read when the reader
        // was initialized. A line that is too long fills the buffer, so its
        // length shows here as well.
        expectedLength = theD6ReadIterator->numCharsForGraphEncoding;
        if (lineNum > 1)
            expectedLength += 1 + (size_t)theD6ReadIterator->numCharsForOrder;

        if (lineNum > 1 && currGraphBuff[0] != '&')
        {
            gp_ErrorMessage("Line %d does not begin with '&', so it is not a "
                            "digraph6 line.",
                            lineNum);
            return NOTOK;
        }

        if (strlen(currGraphBuff) != expectedLength)
        {
            gp_ErrorMessage("Invalid line length read on line %d", lineNum);
            return NOTOK;
        }

        // A later line must give the order of the first graph, in the bytes
        // that encode it, which the line length has shown to be there
        if (lineNum > 1)
        {
            char expectedOrderChars[4];

            _d6_EncodeOrder(expectedOrderChars, theD6ReadIterator->order);
            if (memcmp(currGraphBuff + 1, expectedOrderChars, (size_t)theD6ReadIterator->numCharsForOrder) != 0)
            {
                gp_ErrorMessage("Order of graph on line %d is incorrect.", lineNum);
                return NOTOK;
            }
        }

        graphEncodingChars = (lineNum == 1) ? currGraphBuff : currGraphBuff + 1 + theD6ReadIterator->numCharsForOrder;

        if (_d6_ValidateGraphEncoding(graphEncodingChars, theD6ReadIterator->order,
                                      theD6ReadIterator->numCharsForGraphEncoding, lineNum) != OK)
        {
            gp_ErrorMessage("Graph on line %d is invalid.", lineNum);
            return NOTOK;
        }

        if (lineNum > 1)
        {
            gp_ResetGraphStorage(currGraph);
            // Ensures zero-based flag is set after reinitializing graph.
            currGraph->graphFlags |= GRAPHFLAGS_ZEROBASEDIO;
        }

        if (_d6_DecodeGraph(graphEncodingChars, theD6ReadIterator->order, currGraph) != OK)
        {
            gp_ErrorMessage("Unable to add the arcs of the graph on line %d.", lineNum);
            return NOTOK;
        }

        theD6ReadIterator->numGraphsRead = lineNum;
    }
    else
    {
        if (inputContainer->inputErrorFlag)
        {
            gp_ErrorMessage("Unable to read line %d of .d6 input due to a file read error.", lineNum);
            return NOTOK;
        }

        // The first line was begun when the reader was initialized, so the
        // input cannot end before the rest of it
        if (lineNum == 1)
        {
            gp_ErrorMessage("Line 1 of .d6 input ends after the order of its graph.");
            return NOTOK;
        }

        theD6ReadIterator->endReached = TRUE;

        // The input is closed here, where a failure to close, e.g. of a stream
        // whose close reports an error, can still be returned
        if (sf_closeFile(inputContainer) != OK)
        {
            gp_ErrorMessage("Unable to close the .d6 input after its last line.");
            return NOTOK;
        }
    }

    return OK;
}

// Checks that the encoding of a graph on line lineNum has numChars bytes,
// each in the range 63 to 126, that the bits after the n*n bits of the
// adjacency matrix are zero, as the format pads them, and that no bit of
// the diagonal is set, since the graph library does not support loops.
int _d6_ValidateGraphEncoding(char *graphBuff, const int order, const size_t numChars, const int lineNum)
{
    size_t numBits = (size_t)order * (size_t)order;
    int numPaddingBits = (int)(numChars * 6 - numBits);
    int finalSixBits = 0;

    if (strlen(graphBuff) != numChars)
    {
        gp_ErrorMessage("Invalid number of bytes for graph of order %d on line %d.",
                        order, lineNum);
        return NOTOK;
    }

    for (size_t i = 0; i < numChars; i++)
    {
        if ((unsigned char)graphBuff[i] < 63 || (unsigned char)graphBuff[i] > 126)
        {
            gp_ErrorMessage("Invalid character at index %d of line %d.",
                            (int)i, lineNum);
            return NOTOK;
        }
    }

    finalSixBits = (unsigned char)graphBuff[numChars - 1] - 63;
    if (finalSixBits & ((1 << numPaddingBits) - 1))
    {
        gp_ErrorMessage("Expected %d padding zeroes at the end of line %d.",
                        numPaddingBits, lineNum);
        return NOTOK;
    }

    for (int v = 0; v < order; v++)
    {
        if (_d6_GetArcBit(graphBuff, order, v, v))
        {
            gp_ErrorMessage("Loop edge on vertex %d on line %d is not supported.",
                            v, lineNum);
            return NOTOK;
        }
    }

    return OK;
}

// Adds the arcs of a validated encoding to theGraph. For each pair of
// vertices u < v, the arcs u -> v and v -> u together are one undirected
// edge, as a mutual adjacency is in the adjacency list format, and either
// arc alone is a directed edge. The pairs are visited in the order the
// graph6 reader visits them, so that an undirected graph gets the same
// adjacency lists from either format.
int _d6_DecodeGraph(char *graphBuff, const int order, graphP theGraph)
{
    int lowerBound = 0;
    int forward = 0, backward = 0;
    int e = NIL;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Must initialize graph datastructure before decoding "
                        "the graph representation.");
        return NOTOK;
    }

    // The .d6 format is 0-based, but in-memory storage may not be
    lowerBound = gp_LowerBoundVertexStorage(theGraph);

    for (int v = 1; v < order; v++)
    {
        for (int u = 0; u < v; u++)
        {
            forward = _d6_GetArcBit(graphBuff, order, u, v);
            backward = _d6_GetArcBit(graphBuff, order, v, u);

            if (!forward && !backward)
                continue;

            if (gp_DynamicAddEdge(theGraph, u + lowerBound, 0, v + lowerBound, 0) != OK)
                return NOTOK;

            // The new edge's record in the list of u is now its first
            e = gp_GetFirstEdge(theGraph, u + lowerBound);

            if (forward && !backward)
            {
                gp_SetDirection(theGraph, e, EDGEFLAG_DIRECTION_OUTONLY);
            }
            else if (backward && !forward)
            {
                gp_SetDirection(theGraph, e, EDGEFLAG_DIRECTION_INONLY);
            }
        }
    }

    return OK;
}

void d6_FreeReader(D6ReadIteratorP *pD6ReadIterator)
{
    if (pD6ReadIterator != NULL && (*pD6ReadIterator) != NULL)
    {
        if ((*pD6ReadIterator)->inputContainer != NULL)
            sf_Free(&((*pD6ReadIterator)->inputContainer));

        (*pD6ReadIterator)->numGraphsRead = 0;
        (*pD6ReadIterator)->order = 0;

        if ((*pD6ReadIterator)->currGraphBuff != NULL)
        {
            free((*pD6ReadIterator)->currGraphBuff);
            (*pD6ReadIterator)->currGraphBuff = NULL;
        }

        // N.B. The D6ReadIterator doesn't "own" the graph, so we don't free it.
        (*pD6ReadIterator)->currGraph = NULL;

        free((*pD6ReadIterator));
        (*pD6ReadIterator) = NULL;
    }
}

int _d6_ReadGraphFromStrOrFile(graphP theGraph, strOrFileP *pInputContainer)
{
    D6ReadIteratorP theD6ReadIterator = NULL;
    int RetVal = OK;

    if (pInputContainer == NULL || !sf_IsValidStrOrFile((*pInputContainer)))
    {
        gp_ErrorMessage("Invalid D6 input container.");
        return NOTOK;
    }

    if (d6_NewReader((&theD6ReadIterator), theGraph) != OK)
    {
        gp_ErrorMessage("Unable to allocate D6ReadIterator.");
        return NOTOK;
    }

    // NOTE: (*pInputContainer) will be NULL after we return from this call,
    // since the read iterator will take ownership of the input container.
    if (_d6_InitReaderWithStrOrFile(theD6ReadIterator, pInputContainer) != OK)
    {
        gp_ErrorMessage("Unable to initialize D6ReadIterator.");
        d6_FreeReader((&theD6ReadIterator));
        return NOTOK;
    }

    RetVal = d6_ReadGraph(theD6ReadIterator);
    if (RetVal != OK)
        gp_ErrorMessage("Unable to read graph from .d6 read iterator.");

    d6_FreeReader((&theD6ReadIterator));

    return RetVal;
}
