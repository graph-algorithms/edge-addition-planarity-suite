#ifndef GRAPH_K4SEARCH_PRIVATE_H
#define GRAPH_K4SEARCH_PRIVATE_H

/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved.
No part of this file can be copied or used for any reason without
the expressed written permission of the copyright holder.
*/

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Additional equipment for each graph node (edge arc or vertex)
typedef struct
{
	// We only need a pathConnector because we reduce subgraphs that
	// are separable by a 2-cut, so they can contribute at most one
	// path to a subgraph homeomorphic to K4, if one is indeed found.
	// Thus, we first delete all edges except for the desired path(s),
	// then we reduce any retained path to an edge.
     int  pathConnector;
} K4Search_GraphNode;

typedef K4Search_GraphNode * K4Search_GraphNodeP;

// Additional equipment for each vertex
typedef struct
{
        int sortedDFSChildList;
} K4Search_VertexRec;

typedef K4Search_VertexRec * K4Search_VertexRecP;


typedef struct
{
    // Helps distinguish initialize from re-initialize
    int initialized;

    // The graph that this context augments
    graphP theGraph;

    // Additional graph-level equipment
    listCollectionP sortedDFSChildLists;

    // Parallel array for additional graph node level equipment
    K4Search_GraphNodeP G;

    // Parallel array for additional vertex level equipment
    K4Search_VertexRecP V;

    // Overloaded function pointers
    graphFunctionTable functions;

} K4SearchContext;

#ifdef __cplusplus
}
#endif

#endif
