/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef S6_READ_ITERATOR
#define S6_READ_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "../graph.h"

    typedef struct S6ReadIteratorStruct S6ReadIteratorStruct;
    typedef S6ReadIteratorStruct *S6ReadIteratorP;

    int s6_NewReader(S6ReadIteratorP *pS6ReadIterator, graphP theGraph);

    int s6_InitReaderWithString(S6ReadIteratorP theS6ReadIterator, char *inputString);
    int s6_InitReaderWithFileName(S6ReadIteratorP theS6ReadIterator, char const *const infileName);

    int s6_ReadGraph(S6ReadIteratorP theS6ReadIterator);

    int s6_EndReached(S6ReadIteratorP theS6ReadIterator);
    void s6_FreeReader(S6ReadIteratorP *pS6ReadIterator);

#ifdef __cplusplus
}
#endif

#endif /* S6_READ_ITERATOR */
