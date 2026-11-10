/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef GP_WRITE_ITERATOR
#define GP_WRITE_ITERATOR

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct GPWriteIteratorStruct GPWriteIteratorStruct;
    typedef GPWriteIteratorStruct *GPWriteIteratorP;

    int gp_NewWriter(GPWriteIteratorP *pGPWriteIterator, graphP theGraph, int writeMode);

    int gp_InitWriterWithString(GPWriteIteratorP theGPWriteIterator, char **pOutputString);
    int gp_InitWriterWithFileName(GPWriteIteratorP theGPWriteIterator, char *outputFileName);

    void gp_SetOutputErrorFlag(GPWriteIteratorP theGPWriteIterator);

    int gp_StoreGraphChange(GPWriteIteratorP theGPWriteIterator, int e, int u, int v);
    int gp_WriteGraph(GPWriteIteratorP theGPWriteIterator);

    void gp_FreeWriter(GPWriteIteratorP *pGPWriteIterator);

#ifdef __cplusplus
}
#endif

#endif /* GP_WRITE_ITERATOR */
