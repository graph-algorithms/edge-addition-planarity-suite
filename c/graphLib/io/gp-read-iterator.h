/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef GP_READ_ITERATOR
#define GP_READ_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct GPReadIteratorStruct GPReadIteratorStruct;
    typedef GPReadIteratorStruct *GPReadIteratorP;

    int gp_NewReader(GPReadIteratorP *pGPReadIterator, graphP theGraph);

    int gp_InitReaderWithString(GPReadIteratorP theGPReadIterator, char *inputString);
    int gp_InitReaderWithFileName(GPReadIteratorP theGPReadIterator, char const *const infileName);

    int gp_ReadGraph(GPReadIteratorP theGPReadIterator);
    int gp_RetrieveGraphChange(GPReadIteratorP theGPReadIterator, int *e, int *u, int *v);

    int gp_EndReached(GPReadIteratorP theGPReadIterator);
    void gp_FreeReader(GPReadIteratorP *pGPReadIterator);

#ifdef __cplusplus
}
#endif

#endif /* GP_READ_ITERATOR */
