/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef S6_WRITE_ITERATOR
#define S6_WRITE_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "../graph.h"

    typedef struct S6WriteIteratorStruct S6WriteIteratorStruct;
    typedef S6WriteIteratorStruct *S6WriteIteratorP;

    int s6_NewWriter(S6WriteIteratorP *pS6WriteIterator, graphP theGraph);

    int s6_InitWriterWithString(S6WriteIteratorP theS6WriteIterator, char **pOutputString);
    int s6_InitWriterWithFileName(S6WriteIteratorP theS6WriteIterator, char *outputFileName);

    void s6_SetOutputErrorFlag(S6WriteIteratorP theS6WriteIterator);

    int s6_StoreGraphChange(S6WriteIteratorP theS6WriteIterator, int e, int u, int v);
    int s6_WriteGraph(S6WriteIteratorP theS6WriteIterator);

    void s6_FreeWriter(S6WriteIteratorP *pS6WriteIterator);

#ifdef __cplusplus
}
#endif

#endif /* S6_WRITE_ITERATOR */
