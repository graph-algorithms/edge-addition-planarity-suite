#ifndef GRAPHFUNCTIONTABLE_H
#define GRAPHFUNCTIONTABLE_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#ifdef __cplusplus
extern "C" {
#endif

/*
 NOTE: If you add a function pointer to the function table, then it must 
 also be initialized in _InitFunctionTable() in graphUtils.c.
*/

/*
#define CreateFwdArcLists           0
#define CreateDFSTreeEmbedding      1
#define EmbedBackEdgeToDescendant   2
#define MergeBicomps                3
#define HandleInactiveVertex        4
#define MarkDFSPath                 5
#define EmbedIterationPostprocess   6
#define EmbedPostprocess            7
#define CheckEmbeddingIntegrity     8
#define CheckObstructionIntegrity   9

#define InitGraphNode               10
#define InitVertexRec               11

#define InitGraph                   12
#define ReinitializeGraph           13

#define SortVertices                14

#define ReadPostprocess             15
#define WritePostprocess            16
*/

typedef struct
{
        // These function pointers allow extension modules to overload some of
        // the behaviors of protected functions.  Only advanced applications 
        // will overload these functions
        int  (*fpCreateFwdArcLists)();
        void (*fpCreateDFSTreeEmbedding)();
        void (*fpEmbedBackEdgeToDescendant)();
        int  (*fpMergeBicomps)();
        int  (*fpHandleInactiveVertex)();
        int  (*fpMarkDFSPath)();
        int  (*fpEmbedIterationPostprocess)();
        int  (*fpEmbedPostprocess)();

        int  (*fpCheckEmbeddingIntegrity)();
        int  (*fpCheckObstructionIntegrity)();

        // These function pointers allow extension modules to overload
        // vertex and graphnode initialization. These are not part of the
        // public API, but many extensions are expected to overload them 
        // if they equip vertices or edges with additional parameters
        void (*fpInitGraphNode)();
        void (*fpInitVertexRec)();

        // These function pointers allow extension modules to overload some
        // of the behaviors of gp_* function in the public API
        int  (*fpInitGraph)();
        void (*fpReinitializeGraph)();
        int  (*fpSortVertices)();

        int  (*fpReadPostprocess)();
        int  (*fpWritePostprocess)();

} graphFunctionTable;

typedef graphFunctionTable * graphFunctionTableP;

#ifdef __cplusplus
}
#endif

#endif
