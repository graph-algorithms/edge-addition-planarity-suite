#ifndef GRAPH_K33SEARCH_PRIVATE_H
#define GRAPH_K33SEARCH_PRIVATE_H

/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../graph.h"

#ifdef __cplusplus
extern "C"
{
#endif

// K33CERT begin: Declarations for K3,3 embedding obsruction tree nodes
#define K33SEARCH_EOTYPE_ENODE 0
#define K33SEARCH_EOTYPE_ONODE 1
    typedef struct
    {
        int EOType;        // default EOTYPE_ENODE via memset 0 init
        graphP subgraph;   // default NULL via memset 0 init
        int subgraphOwner; // default FALSE via memset 0 init
    } K33Search_EONode;

    typedef K33Search_EONode *K33Search_EONodeP;

    K33Search_EONodeP _K33Search_EONode_New(graphP theSubgraph);
    void _K33Search_EONode_Free(K33Search_EONodeP *pEONode);
    int _K33Search_TestForEOTreeChildren(K33Search_EONodeP EOTreeNode);
    int _K33Search_AssembleMainPlanarEmbedding(K33Search_EONodeP EOTreeRoot);
    int _K33Search_ValidateEmbeddingObstructionTree(K33Search_EONodeP EOTreeRoot, graphP origGraph);
    // K33CERT end

    // Additional equipment for each EdgeRec
    typedef struct
    {
        int noStraddle, pathConnector;
        // K33CERT begin: Addition to enable edge rec to point to a K33_EONode instance
        K33Search_EONodeP EONode; // default NULL via memset 0 init
        // K33CERT end
    } K33Search_EdgeRec;

    typedef K33Search_EdgeRec *K33Search_EdgeRecP;

    // Additional equipment for each primary vertex
    typedef struct
    {
        int separatedDFSChildList, backArcList, mergeBlocker;
    } K33Search_VertexInfo;

    typedef K33Search_VertexInfo *K33Search_VertexInfoP;

    typedef struct
    {
        // Helps distinguish initialize from re-initialize
        int initialized;

        // The graph that this context augments
        graphP theGraph;

        // K33CERT begin: Addition to enable edge rec to point to a K33_EONode instance
        K33Search_EONodeP associatedEONode;
        // K33CERT end

        // Parallel array for additional edge level equipment
        K33Search_EdgeRecP E;

        // Parallel array for additional vertex info level equipment
        K33Search_VertexInfoP VI;

        // Storage for the separatedDFSChildLists, and
        // to help with linear time sorting of same by lowpoints
        listCollectionP separatedDFSChildLists;
        int *buckets;
        listCollectionP bin;

        // Overloaded function pointers
        graphFunctionTable functions;

    } K33SearchContext;

#ifdef __cplusplus
}
#endif

#endif
