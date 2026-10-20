/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef D6_READ_ITERATOR
#define D6_READ_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include "../graph.h"

    typedef struct D6ReadIteratorStruct D6ReadIteratorStruct;
    typedef D6ReadIteratorStruct *D6ReadIteratorP;

    int d6_IsDigraph6Input(char const *const firstLine);

    int d6_NewReader(D6ReadIteratorP *pD6ReadIterator, graphP theGraph);

    int d6_InitReaderWithString(D6ReadIteratorP theD6ReadIterator, char *inputString);
    int d6_InitReaderWithFileName(D6ReadIteratorP theD6ReadIterator, char const *const infileName);

    int d6_ReadGraph(D6ReadIteratorP theD6ReadIterator);

    int d6_EndReached(D6ReadIteratorP theD6ReadIterator);
    void d6_FreeReader(D6ReadIteratorP *pD6ReadIterator);

#ifdef __cplusplus
}
#endif

#endif /* D6_READ_ITERATOR */
