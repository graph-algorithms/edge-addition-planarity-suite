/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef G6_READ_ITERATOR
#define G6_READ_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdbool.h>

#include "../graph.h"
#include "strOrFile.h"

    typedef struct
    {
        strOrFileP g6Input;
        int numGraphsRead;

        int graphOrder;
        int numCharsForGraphOrder;
        int numCharsForGraphEncoding;
        int currGraphBuffSize;
        char *currGraphBuff;

        graphP currGraph;
    } G6ReadIterator;

    typedef G6ReadIterator *G6ReadIteratorP;

    int allocateG6ReadIterator(G6ReadIteratorP *, graphP);
    bool _isG6ReadIteratorAllocated(G6ReadIteratorP);

    int getNumGraphsRead(G6ReadIteratorP, int *);
    int getOrderOfGraphToRead(G6ReadIteratorP, int *);
    int getPointerToGraphReadIn(G6ReadIteratorP, graphP *);

    int beginG6ReadIterationFromG6String(G6ReadIteratorP, char *);
    int beginG6ReadIterationFromG6FilePath(G6ReadIteratorP, char *);
    int beginG6ReadIterationFromG6StrOrFile(G6ReadIteratorP, strOrFileP);
    int _beginG6ReadIteration(G6ReadIteratorP);
    int _processAndCheckHeader(strOrFileP);
    bool _firstCharIsValid(char, const int);
    int _getGraphOrder(strOrFileP, int *);

    int readGraphUsingG6ReadIterator(G6ReadIteratorP);
    int _checkGraphOrder(char *, int);
    int _validateGraphEncoding(char *, const int, const int);
    int _decodeGraph(char *, const int, const int, graphP);

    int endG6ReadIteration(G6ReadIteratorP);

    int freeG6ReadIterator(G6ReadIteratorP *);

    int _ReadGraphFromG6FilePath(graphP, char *);
    int _ReadGraphFromG6String(graphP, char *);
    int _ReadGraphFromG6StrOrFile(graphP, strOrFileP);

#ifdef __cplusplus
}
#endif

#endif /* G6_READ_ITERATOR */
