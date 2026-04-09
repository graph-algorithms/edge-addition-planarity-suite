#ifndef GRAPH_K33SEARCH_PRIVATE_H
#define GRAPH_K33SEARCH_PRIVATE_H

/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../graph.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef INCLUDE_K33SEARCH_EMBEDDER
// Declarations for K3,3 embedding obsruction tree nodes
#define K33SEARCH_EOTYPE_ENODE 0
#define K33SEARCH_EOTYPE_ONODE 1
        typedef struct
        {
                int EOType;        // set by constructor parameter
                graphP subgraph;   // set by constructor parameter
                int subgraphOwner; // set by constructor parameter
                int visited;       // default FALSE set by constructor
        } K33Search_EONode;

        typedef K33Search_EONode *K33Search_EONodeP;
#endif

        // Additional equipment for each EdgeRec
        typedef struct
        {
                int noStraddle, pathConnector;
#ifdef INCLUDE_K33SEARCH_EMBEDDER
                // Addition to enable edge rec to point to a K33_EONode instance
                K33Search_EONodeP EONode; // default NULL via memset 0 init
#endif
        } K33Search_EdgeRec;

        typedef K33Search_EdgeRec *K33Search_EdgeRecP;

        // Additional equipment for each vertex (non-virtual only)
        typedef struct
        {
                int separatedDFSChildList, backEdgeList, mergeBlocker;
#ifdef INCLUDE_K33SEARCH_EMBEDDER
                // Addition of a variable often and temporarily used to enable
                // internal subroutines to map between a graph's vertex indices
                // and the vertex indices of a subgraph being extracted from it
                int graphToSubgraphIndex, subgraphToGraphIndex;
#endif
        } K33Search_VertexInfo;

        typedef K33Search_VertexInfo *K33Search_VertexInfoP;

        typedef struct
        {
                // Helps distinguish initialize from re-initialize
                int initialized;

                // The graph that this context augments
                graphP theGraph;

#ifdef INCLUDE_K33SEARCH_EMBEDDER
                // Addition to enable edge rec to point to a K33_EONode instance
                K33Search_EONodeP associatedEONode;
#endif

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

        extern int K33SEARCH_ID;

#ifdef __cplusplus
}
#endif

#endif
