#ifndef GRAPH_K33SEARCH_PRIVATE_H
#define GRAPH_K33SEARCH_PRIVATE_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Additional equipment for each graph node (edge arc or vertex)
typedef struct
{
     int  noStraddle, pathConnector;
} K33Search_GraphNode;

typedef K33Search_GraphNode * K33Search_GraphNodeP;

// Additional equipment for each vertex
typedef struct
{
        int sortedDFSChildList, backArcList;
        int externalConnectionAncestor, mergeBlocker;
} K33Search_VertexRec;

typedef K33Search_VertexRec * K33Search_VertexRecP;


typedef struct
{
    // Helps distinguish initialize from re-initialize
    int initialized;

    // The graph that this context augments
    graphP theGraph;

    // Additional graph-level equipment
    listCollectionP sortedDFSChildLists;

    // Parallel array for additional graph node level equipment
    K33Search_GraphNodeP G;

    // Parallel array for additional vertex level equipment
    K33Search_VertexRecP V;    

    // Overloaded function pointers
    graphFunctionTable functions;

} K33SearchContext;

#ifdef __cplusplus
}
#endif

#endif

