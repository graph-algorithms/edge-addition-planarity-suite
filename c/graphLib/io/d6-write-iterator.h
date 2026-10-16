/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef D6_WRITE_ITERATOR
#define D6_WRITE_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "../graph.h"

    typedef struct D6WriteIteratorStruct D6WriteIteratorStruct;
    typedef D6WriteIteratorStruct *D6WriteIteratorP;

    int d6_NewWriter(D6WriteIteratorP *pD6WriteIterator, graphP theGraph);

    int d6_InitWriterWithString(D6WriteIteratorP theD6WriteIterator, char **pOutputString);
    int d6_InitWriterWithFileName(D6WriteIteratorP theD6WriteIterator, char *outputFileName);

    void d6_SetOutputErrorFlag(D6WriteIteratorP theD6WriteIterator);

    int d6_WriteGraph(D6WriteIteratorP theD6WriteIterator);

    void d6_FreeWriter(D6WriteIteratorP *pD6WriteIterator);

#ifdef __cplusplus
}
#endif

#endif /* D6_WRITE_ITERATOR */
