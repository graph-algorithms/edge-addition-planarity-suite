/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef G6_WRITE_ITERATOR
#define G6_WRITE_ITERATOR

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
        strOrFileP g6Output;
        int numGraphsWritten;

        int order;
        int numCharsForOrder;
        int numCharsForGraphEncoding;
        int currGraphBuffSize;
        char *currGraphBuff;

        int *columnOffsets;

        graphP currGraph;
    } G6WriteIterator;

    typedef G6WriteIterator *G6WriteIteratorP;

    int g6_NewWriter(G6WriteIteratorP *, graphP);

    int g6_GetNumGraphsWritten(G6WriteIteratorP, int *);
    int g6_GetOrderFromWriter(G6WriteIteratorP, int *);
    int g6_GetGraphFromWriter(G6WriteIteratorP, graphP *);

    int g6_InitWriterWithString(G6WriteIteratorP);
    int g6_InitWriterWithFileName(G6WriteIteratorP, char *);

    int g6_WriteGraph(G6WriteIteratorP);

    void g6_FreeWriter(G6WriteIteratorP *);

#ifdef __cplusplus
}
#endif

#endif /* G6_WRITE_ITERATOR */
