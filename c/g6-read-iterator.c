/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "g6-read-iterator.h"
#include "g6-api-utilities.h"


int allocateG6ReadIterator(G6ReadIterator **ppG6ReadIterator, graphP pGraph)
{
    int exitCode = OK;

    if (ppG6ReadIterator != NULL && (*ppG6ReadIterator) != NULL)
    {
        printf("[ERROR] G6ReadIterator is not NULL and therefore can't be allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    // doIOwnFilePointer, numGraphsRead, graphOrder, numCharsForGraphOrder,
    // numCharsForGraphEncoding, and currGraphBuffSize all set to 0
    (*ppG6ReadIterator) = (G6ReadIterator *) calloc(1, sizeof(G6ReadIterator));

    if ((*ppG6ReadIterator) == NULL)
    {
        printf("[ERROR] Unable to allocate memory for G6ReadIterator.\n");
        fflush(stdout);
        return NOTOK;
    }

    (*ppG6ReadIterator)->g6Infile = NULL;

    if (pGraph == NULL)
    {
        printf("[ERROR] Must allocate graph to be used by G6ReadIterator.\n");
        fflush(stdout);

        exitCode = freeG6ReadIterator(ppG6ReadIterator);

        if (exitCode != OK) {
            printf("[ERROR] Unable to free the G6ReadIterator.\n");
            fflush(stdout);
        }
    }
    else
    {
        (*ppG6ReadIterator)->currGraph = pGraph;
    }

    return exitCode;
}

bool _isG6ReadIteratorAllocated(G6ReadIterator *pG6ReadIterator)
{
    bool g6ReadIteratorIsAllocated = true;

    if (pG6ReadIterator == NULL || pG6ReadIterator->currGraph == NULL)
    {
        g6ReadIteratorIsAllocated = false;
    }

    return g6ReadIteratorIsAllocated;
}

int getNumGraphsRead(G6ReadIterator *pG6ReadIterator, int *pNumGraphsRead)
{
    if (pG6ReadIterator == NULL)
    {
        printf("[ERROR] G6ReadIterator is not allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    (*pNumGraphsRead) = pG6ReadIterator->numGraphsRead;

    return OK;
}

int getOrderOfGraphToRead(G6ReadIterator *pG6ReadIterator, int *pGraphOrder)
{
    if (pG6ReadIterator == NULL)
    {
        printf("[ERROR] G6ReadIterator is not allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    (*pGraphOrder) = pG6ReadIterator->graphOrder;

    return OK;
}

int getPointerToGraphReadIn(G6ReadIterator *pG6ReadIterator, graphP *ppGraph)
{
    if (pG6ReadIterator == NULL)
    {
        printf("[ERROR] G6ReadIterator is not allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    (*ppGraph) = pG6ReadIterator->currGraph;

    return OK;
}

int beginG6ReadIteration(G6ReadIterator *pG6ReadIterator, char *g6FilePath)
{
    int exitCode = OK;

    if (!_isG6ReadIteratorAllocated(pG6ReadIterator))
    {
        printf("[ERROR] G6ReadIterator is not allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    if (g6FilePath == NULL || strlen(g6FilePath) == 0)
    {
        printf("[ERROR] g6FilePath is null or has length 0.\n");
        fflush(stdout);
        return NOTOK;
    }

    g6FilePath[strcspn(g6FilePath, "\n\r")] = '\0';

    FILE *g6Infile = fopen(g6FilePath, "r");

    if (g6Infile == NULL)
    {
        printf("[ERROR] Unable to open .g6 file with path \"%s\"\n", g6FilePath);
        fflush(stdout);
        return NOTOK;
    }

    pG6ReadIterator->doIOwnFilePointer = true;
    exitCode = beginG6ReadIterationFromFilePointer(pG6ReadIterator, g6Infile);

    return exitCode;
}

int beginG6ReadIterationFromFilePointer(G6ReadIterator *pG6ReadIterator, FILE *g6Infile)
{
    int exitCode = OK;

    if (g6Infile == NULL)
    {
        printf("[ERROR] .g6 file pointer is NULL.\n");
        fflush(stdout);
        return NOTOK;
    }

    pG6ReadIterator->g6Infile = g6Infile;

    int firstChar = getc(g6Infile);
    if (firstChar == EOF)
    {
        printf("[ERROR] File is empty; aborting.\n");
        fflush(stdout);
        return NOTOK;
    }
    else
    {
        ungetc(firstChar, g6Infile);
        if (firstChar == '>')
        {
            exitCode = _processAndCheckHeader(g6Infile);
            if (exitCode != OK)
            {
                printf("[ERROR] Unable to process file; aborting.\n");
                fflush(stdout);
                return exitCode;
            }
        }
    }

    int lineNum = 1;
    firstChar = getc(g6Infile);
    ungetc(firstChar, g6Infile);
    if (!_firstCharIsValid(firstChar, lineNum))
        return NOTOK;

    // Despite the general specification indicating that n \in [0, 68,719,476,735],
    // in practice n will be limited such that an integer will suffice in storing it.
    int graphOrder = -1;
    exitCode = _getGraphOrder(g6Infile, &graphOrder);

    if (exitCode != OK)
    {
        printf("[ERROR] Invalid graph order on line %d of .g6 file.\n", lineNum);
        fflush(stdout);
        return exitCode;
    }

    if (pG6ReadIterator->currGraph->N == 0)
    {
        exitCode = gp_InitGraph(pG6ReadIterator->currGraph, graphOrder);

        if (exitCode != OK)
        {
            printf("[ERROR] Unable to initialize graph datastructure with order %d for graph on line %d of the .g6 file.\n", graphOrder, lineNum);
            fflush(stdout);
            return exitCode;
        }

        pG6ReadIterator->graphOrder = graphOrder;
    }
    else
    {
        if (pG6ReadIterator->currGraph->N != graphOrder)
        {
            printf("[ERROR] Graph datastructure passed to G6ReadIterator already initialized with graph order %d,\n", pG6ReadIterator->currGraph->N);
            printf("\twhich doesn't match the graph order %d specified in the file.\n", graphOrder);
            fflush(stdout);
            return NOTOK;
        }
        else
        {
            gp_ReinitializeGraph(pG6ReadIterator->currGraph);
            pG6ReadIterator->graphOrder = graphOrder;
        }
    }

    // TODO: Is this the right place to do this? I want to make sure the flags are correctly set
    // regardless of whether the graph was initialized or re-initialized.
    pG6ReadIterator->currGraph->internalFlags |= FLAGS_ZEROBASEDIO;

    pG6ReadIterator->numCharsForGraphOrder = _getNumCharsForGraphOrder(graphOrder);
    pG6ReadIterator->numCharsForGraphEncoding = _getNumCharsForGraphEncoding(graphOrder);
    // Must add 3 bytes for newline, possible carriage return, and null terminator
    pG6ReadIterator->currGraphBuffSize = pG6ReadIterator->numCharsForGraphOrder + pG6ReadIterator->numCharsForGraphEncoding + 3;
    pG6ReadIterator->currGraphBuff = (char *) calloc(pG6ReadIterator->currGraphBuffSize, sizeof(char));

    if (pG6ReadIterator->currGraphBuff == NULL)
    {
        printf("[ERROR] Unable to allocate memory for currGraphBuff.\n");
        fflush(stdout);
        exitCode = NOTOK;
    }

    return exitCode;
}

int _processAndCheckHeader(FILE *g6File)
{
    int exitCode = OK;

    if (g6File == NULL)
    {
        printf("[ERROR] Invalid file pointer; please check .g6 file.\n");
        fflush(stdout);
        return NOTOK;
    }

    char *correctG6Header = ">>graph6<<";
    char *sparse6Header = ">>sparse6<";
    char *digraph6Header = ">>digraph6";

    char headerCandidateChars[11];
    headerCandidateChars[0] = '\0';

    for (int i = 0; i < 10; i++)
    {
        headerCandidateChars[i] = getc(g6File);
    }

    headerCandidateChars[10] = '\0';

    if (strcmp(correctG6Header, headerCandidateChars) != 0)
    {        
        if (strcmp(sparse6Header, headerCandidateChars) == 0)
            printf("[ERROR] Graph file is sparse6 format, which is not supported; aborting.\n");
        else if (strcmp(digraph6Header, headerCandidateChars) == 0)
            printf("[ERROR] Graph file is digraph6 format, which is not supported; aborting.\n");
        else
            printf("[ERROR] Invalid header for .g6 file; aborting.\n"); 
        fflush(stdout);

         exitCode = NOTOK;
    }

    return exitCode;
}

bool _firstCharIsValid(char c, const int lineNum)
{
    bool isValidFirstChar = false;

    if (strchr(":;&", c) != NULL)
    {
        printf("[ERROR] Invalid first character on line %d, i.e. one of ':', ';', or '&'; aborting.\n", lineNum);
        fflush(stdout);
    }
    else
        isValidFirstChar = true;

    return isValidFirstChar;
}

int _getGraphOrder(FILE *g6Infile, int *graphOrder)
{
    int exitCode = OK;

    if (g6Infile == NULL)
    {
        printf("[ERROR] Invalid file pointer; please check input file.\n");
        fflush(stdout);
        return NOTOK;
    }

    // Since geng: n must be in the range 1..32, and since edge-addition-planarity-suite
    // processing of random graphs may only handle up to n = 100,000, we will only check
    // if 1 or 4 bytes are necessary
    int n = 0;
    int graphChar = getc(g6Infile);
    if (graphChar == 126)
    {
        if ((graphChar = getc(g6Infile)) == 126)
        {
            printf("[ERROR] Graph order is too large; format suggests that 258048 <= n <= 68719476735, but we only support n <= 100000.\n");
            fflush(stdout);
            return NOTOK;
        }

        ungetc(graphChar, g6Infile);

        for (int i = 2; i >= 0; i--)
        {
            graphChar = getc(g6Infile) - 63;
            n |= graphChar << (6 * i);
        }

        if (n > 100000)
        {
            printf("[ERROR] Graph order is too large; we only support n <= 100000.\n");
            fflush(stdout);
            return NOTOK;
        }
    }
    else if (graphChar > 62 && graphChar < 126)
        n = graphChar - 63;
    else
    {
        printf("[ERROR] Graph order is too small; character doesn't correspond to a printable ASCII character.\n");
        fflush(stdout);
        return NOTOK;
    }

    (*graphOrder) = n;

    return exitCode;
}

int readGraphUsingG6ReadIterator(G6ReadIterator *pG6ReadIterator)
{
    int exitCode = OK;

    if (!_isG6ReadIteratorAllocated(pG6ReadIterator))
    {
        printf("[ERROR] G6ReadIterator is not allocated.\n");
        fflush(stdout);
        return NOTOK;
    }

    FILE *g6Infile = pG6ReadIterator->g6Infile;

    if (g6Infile == NULL)
    {
        printf("[ERROR] g6Infile pointer is null.\n");
        fflush(stdout);
        return NOTOK;
    }

    int numGraphsRead = pG6ReadIterator->numGraphsRead;

    char *currGraphBuff = pG6ReadIterator->currGraphBuff;

    if (currGraphBuff == NULL)
    {
        printf("[ERROR] currGraphBuff string is null.\n");
        fflush(stdout);
        return NOTOK;
    }

    const int graphOrder = pG6ReadIterator->graphOrder;
    const int numCharsForGraphOrder = pG6ReadIterator->numCharsForGraphOrder;
    const int numCharsForGraphEncoding = pG6ReadIterator->numCharsForGraphEncoding;
    const int currGraphBuffSize = pG6ReadIterator->currGraphBuffSize;

    graphP currGraph = pG6ReadIterator->currGraph;

    char firstChar = '\0';
    char *graphEncodingChars = NULL;
    if (fgets(currGraphBuff, currGraphBuffSize, g6Infile) != NULL)
    {
        numGraphsRead++;
        firstChar = currGraphBuff[0];

        if (!_firstCharIsValid(firstChar, numGraphsRead))
            return NOTOK;

        // From https://stackoverflow.com/a/28462221, strcspn finds the index of the first
        // char in charset; this way, I replace the char at that index with the null-terminator
        currGraphBuff[strcspn(currGraphBuff, "\r\n")] = '\0'; // works for LF, CR, CRLF, LFCR, ...

        // If the line was too long, then we would have placed the null terminator at the final
        // index (where it already was; see strcpn docs), and the length of the string will be 
        // longer than the line should have been, i.e. orderOffset + numCharsForGraphRepr
        if (strlen(currGraphBuff) != (((numGraphsRead == 1) ? 0 : numCharsForGraphOrder) + numCharsForGraphEncoding))
        {
            printf("[ERROR] Invalid line length read on line %d\n", numGraphsRead);
            fflush(stdout);
            return NOTOK;
        }

        if (numGraphsRead > 1)
        {
            exitCode = _checkGraphOrder(currGraphBuff, graphOrder);

            if (exitCode != OK)
            {
                printf("[ERROR] Order of graph on line %d is incorrect.\n", numGraphsRead);
                fflush(stdout);
                return exitCode;
            }
        }

        // On first line, we have already processed the characters corresponding to the graph
        // order, so there's no need to apply the offset. On subsequent lines, the orderOffset
        // must be applied so that we are only starting validation on the byte corresponding to
        // the encoding of the adjacency matrix.
        graphEncodingChars = (numGraphsRead == 1) ? currGraphBuff : currGraphBuff + numCharsForGraphOrder;
        
        exitCode = _validateGraphEncoding(graphEncodingChars, graphOrder, numCharsForGraphEncoding);

        if (exitCode != OK)
        {
            printf("[ERROR] Graph on line %d is invalid.", numGraphsRead);
            fflush(stdout);
            return exitCode;
        }

        if (numGraphsRead > 1)
        {
            gp_ReinitializeGraph(currGraph);
            // TODO: Double-checking that we want to re-set these flags on re-initalize as per convo Feb. 29, 2024
            currGraph->internalFlags |= FLAGS_ZEROBASEDIO;
        }

        exitCode = _decodeGraph(graphEncodingChars, graphOrder, numCharsForGraphEncoding, currGraph);

        if (exitCode != OK)
        {
            printf("[ERROR] Unable to interpret bits on line %d to populate adjacency matrix.\n", numGraphsRead);
            fflush(stdout);
            return exitCode;
        }
        
        pG6ReadIterator->numGraphsRead = numGraphsRead;
    }
    else
        pG6ReadIterator->currGraph = NULL;

    return exitCode;
}

int _checkGraphOrder(char *graphBuff, int graphOrder)
{
    int exitCode = OK;
    
    int n = 0;
    char currChar = graphBuff[0];
    if (currChar == 126)
    {
        if (graphBuff[1] == 126)
        {
            printf("[ERROR] Can only handle graphs of order <= 100,000.\n");
            fflush(stdout);
            return NOTOK;
        }
        else if (graphBuff[1] > 126)
        {
            printf("[ERROR] Invalid graph order signifier.\n");
            fflush(stdout);
            return NOTOK;
        }

        int orderCharIndex = 2;

        for (int i = 1; i < 4; i++)
            n |= (graphBuff[i] - 63) << (6 * orderCharIndex--);
    }
    else if (currChar > 62 && currChar < 126)
        n = currChar - 63;
    else
    {
        printf("[ERROR] Character doesn't correspond to a printable ASCII character.\n");
        fflush(stdout);
        return NOTOK;
    }

    if (n != graphOrder)
    {
        printf("[ERROR] Graph order %d doesn't match expected graph order %d", n, graphOrder);
        fflush(stdout);
        exitCode = NOTOK;
    }

    return exitCode;
}

int _validateGraphEncoding(char *graphBuff, const int graphOrder, const int numChars)
{
    int exitCode = OK;

    // Num edges of the graph (and therefore the number of bits) is (n * (n-1))/2, and
    // since each resulting byte needs to correspond to an ascii character between 63 and 126,
    // each group is only comprised of 6 bits (to which we add 63 for the final byte value)
    int expectedNumChars = _getNumCharsForGraphEncoding(graphOrder);
    int numCharsForGraphEncoding = strlen(graphBuff);
    if (expectedNumChars != numCharsForGraphEncoding)
    {
        printf("[ERROR] Invalid number of bytes for graph of order %d; got %d but expected %d\n", graphOrder, numCharsForGraphEncoding, expectedNumChars);
        fflush(stdout);
        return NOTOK;
    }

    // Check that characters are valid ASCII characters between 62 and 126
    for (int i = 0; i < numChars; i++)
    {
        if (graphBuff[i] < 63 || graphBuff[i] > 126)
        {
            printf("[ERROR] Invalid character at index %d: \"%c\"\n", i, graphBuff[i]);
            fflush(stdout);
            return NOTOK;
        }
    }

    // Check that there are no extraneous bits in representation (since we pad out to a
    // multiple of 6 before splitting into bytes and adding 63 to each byte)
    int expectedNumPaddingZeroes = _getExpectedNumPaddingZeroes(graphOrder, numChars);
    char finalByte = graphBuff[numChars - 1] - 63;
    int numPaddingZeroes = 0;
    for (int i = 0; i < expectedNumPaddingZeroes; i++)
    {
        if (finalByte & (1 << i))
            break;
        
        numPaddingZeroes++;
    }

    if (numPaddingZeroes != expectedNumPaddingZeroes)
    {
        printf("[ERROR] Expected %d padding zeroes, but got %d.\n", expectedNumPaddingZeroes, numPaddingZeroes);
        fflush(stdout);
        exitCode = NOTOK;
    }

    return exitCode;
}

// Takes the character array graphBuff, the derived number of vertices graphOrder,
// and the numChars corresponding to the number of characters after the first byte
// and performs the inverse transformation of the graph encoding: we subtract 63 from
// each byte, then only process the 6 least significant bits of the resulting byte. For
// the final byte, we determine how many padding zeroes to expect, and exclude them
// from being processed. We index into the adjacency matrix by row and column, which
// are incremented such that row ranges from 0 to one less than the column index.
int _decodeGraph(char *graphBuff, const int graphOrder, const int numChars, graphP pGraph)
{
    int exitCode = OK;

    if (pGraph == NULL)
    {
        printf("[ERROR] Must initialize graph datastructure before invoking _decodeGraph.\n");
        fflush(stdout);
        return NOTOK;
    }

    int numPaddingZeroes = _getExpectedNumPaddingZeroes(graphOrder, numChars);

    char currByte = '\0';
    int bitValue = 0;
    int row = 0;
    int col = 1;
    for (int i = 0; i < numChars; i++)
    {
        currByte = graphBuff[i] - 63;
        // j corresponds to the number of places one must bitshift the byte by to read
        // the next bit in the byte
        for (int j = sizeof(char) * 5; j >= 0; j--)
        {
            // If we are on the final byte, we know that the final numPaddingZeroes bits
            // can be ignored, so we break out of the loop
            if ((i == numChars) && j == numPaddingZeroes - 1)
                break;

            if (row == col)
            {
                row = 0;
                col++;
            }

            bitValue = ((currByte >> j) & 1u) ? 1 : 0;
            if (bitValue == 1)
            {
                // Add gp_GetFirstVertex(pGraph), which is 1 if NIL == 0 (i.e. 1-based labelling) and 0 if NIL == -1 (0-based)
                exitCode = gp_AddEdge(pGraph, row+gp_GetFirstVertex(pGraph), 0, col+gp_GetFirstVertex(pGraph), 0);
                // TODO: verify if breaking out entirely from decode is acceptable on gp_AddEdge returning NOTOK
                if (exitCode == NOTOK)
                    return exitCode;
            }

            row++;
        }    
    }

    pGraph->N = graphOrder;

    return exitCode;
}

int endG6ReadIteration(G6ReadIterator *pG6ReadIterator)
{
    int exitCode = OK;

    if (pG6ReadIterator != NULL)
    {
        if (pG6ReadIterator->g6Infile != NULL && pG6ReadIterator->doIOwnFilePointer)
        {
            int fcloseCode = fclose(pG6ReadIterator->g6Infile);

            if (fcloseCode != 0)
            {
                printf("[ERROR] Unable to close G6ReadIterator's g6Infile.\n");
                fflush(stdout);
                exitCode = NOTOK;
            }
        }

        pG6ReadIterator->g6Infile = NULL;

        if (pG6ReadIterator->currGraphBuff != NULL)
        {
            free(pG6ReadIterator->currGraphBuff);
            pG6ReadIterator->currGraphBuff = NULL;
        }
    }

    return exitCode;
}

int freeG6ReadIterator(G6ReadIterator **ppG6ReadIterator)
{
    int exitCode = OK;

    if (ppG6ReadIterator != NULL && (*ppG6ReadIterator) != NULL)
    {
        if ((*ppG6ReadIterator)->g6Infile != NULL && (*ppG6ReadIterator)->doIOwnFilePointer)
        {
            int fcloseCode = fclose((*ppG6ReadIterator)->g6Infile);

            if (fcloseCode != 0)
            {
                printf("[ERROR] Unable to close G6ReadIterator's g6Infile.\n");
                fflush(stdout);
                exitCode = NOTOK;
            }
        }

        (*ppG6ReadIterator)->g6Infile = NULL;

        (*ppG6ReadIterator)->numGraphsRead = 0;
        (*ppG6ReadIterator)->graphOrder = 0;

        if ((*ppG6ReadIterator)->currGraphBuff != NULL)
        {
            free((*ppG6ReadIterator)->currGraphBuff);
            (*ppG6ReadIterator)->currGraphBuff = NULL;
        }

        (*ppG6ReadIterator)->currGraph = NULL;

        free((*ppG6ReadIterator));
        (*ppG6ReadIterator) = NULL;
    }

    return exitCode;
}

int _ReadGraphFromG6File(graphP pGraphToRead, char *pathToG6File)
{
    int exitCode = OK;

    G6ReadIterator *pG6ReadIterator = NULL;
    exitCode = allocateG6ReadIterator(&pG6ReadIterator, pGraphToRead);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to allocate G6ReadIterator.\n");
        fflush(stdout);
        return exitCode;
    }

    exitCode = beginG6ReadIteration(pG6ReadIterator, pathToG6File);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to begin .g6 read iteration.\n");
        fflush(stdout);

        exitCode = freeG6ReadIterator(&pG6ReadIterator);

        if (exitCode != OK)
        {
            printf("[ERROR] Unable to free G6ReadIterator.\n");
            fflush(stdout);
        }

        return exitCode;
    }

    exitCode = readGraphUsingG6ReadIterator(pG6ReadIterator);
    if (exitCode != OK)
    {
        printf("[ERROR] Unable to read graph from .g6 read iterator.\n");
        fflush(stdout);
    }

    exitCode = endG6ReadIteration(pG6ReadIterator);
    if (exitCode != OK)
    {
        printf("[ERROR] Unable to end G6ReadIterator.\n");
        fflush(stdout);
    }
    
    exitCode = freeG6ReadIterator(&pG6ReadIterator);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to free G6ReadIterator.\n");
        fflush(stdout);
    }

    return exitCode;
}

int _ReadGraphFromG6FilePointer(graphP pGraphToRead, FILE *g6Infile)
{
    int exitCode = OK;

    G6ReadIterator *pG6ReadIterator = NULL;
    exitCode = allocateG6ReadIterator(&pG6ReadIterator, pGraphToRead);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to allocate G6ReadIterator.\n");
        fflush(stdout);
        return exitCode;
    }

    exitCode = beginG6ReadIterationFromFilePointer(pG6ReadIterator, g6Infile);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to begin .g6 read iteration.\n");
        fflush(stdout);

        exitCode = freeG6ReadIterator(&pG6ReadIterator);

        if (exitCode != OK)
        {
            printf("[ERROR] Unable to free G6ReadIterator.\n");
            fflush(stdout);
        }

        return exitCode;
    }

    exitCode = readGraphUsingG6ReadIterator(pG6ReadIterator);
    if (exitCode != OK)
    {
        printf("[ERROR] Unable to read graph from .g6 read iterator.\n");
        fflush(stdout);
    }

    exitCode = endG6ReadIteration(pG6ReadIterator);
    if (exitCode != OK)
    {
        printf("[ERROR] Unable to end G6ReadIterator.\n");
        fflush(stdout);
    }
    
    exitCode = freeG6ReadIterator(&pG6ReadIterator);

    if (exitCode != OK)
    {
        printf("[ERROR] Unable to free G6ReadIterator.\n");
        fflush(stdout);
    }

    return exitCode;
}