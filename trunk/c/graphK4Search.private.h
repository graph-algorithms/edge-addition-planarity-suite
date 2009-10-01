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

/* Additional equipment for each graph node (edge arc or vertex)

   pathConnector:
      Used in the edge records (arcs) of a reduction edge to indicate the
      endpoints of a path that has been reduced from (removed from) the
      embedding so that the search for a K4 can continue.
	  We only need a pathConnector because we reduce subgraphs that are
	  separable by a 2-cut, so they can contribute at most one path to a
	  subgraph homeomorphic to K4, if one is indeed found. Thus, we first
	  delete all edges except for the desired path(s), then we reduce any
	  retained path to an edge.

   subtree:
      Used in each forward arc (V, D) to indicate the DFS child C of V whose
      DFS subtree contains the DFS descendant endpoint D of the forward arc.
      This helps to efficiently find C when (V, D) is embedded so that the
      p2dFwdArcCount of C can be decremented.
      In order to efficiently calculate this value in preprocessing, the
      fwdArcList of each vertex is sorted by descendant endpoint DFS number,
      and the sortedDFSChildList of each vertex is computed. This enables
      the subtree settings to be made with a single pass simultaneously
      through the fwdArcList and sortedDFSChildList. See CreateFwdArcLists
      for implementation details.
 */
typedef struct
{
     int  pathConnector, subtree;
} K4Search_GraphNode;

typedef K4Search_GraphNode * K4Search_GraphNodeP;

/* Additional equipment for each vertex

   p2dFwdArcCount:
      During preprocessing, for each vertex we need to know how many forward arcs
      there are from the DFS parent of the vertex to the DFS descendants of the
      vertex. As each forward arc is embedded, we decrement this count. When this
      count reaches zero, we remove the vertex from the sortedDFSChildList of its
      parent.

   sortedDFSChildList:
      During preprocessing, we need a list of the DFS children for each vertex,
      sorted by their DFS numbers. The core planarity/outerplanarity algorithm
      calculates a separatedDFSChildList that is sorted by the children's Lowpoints.
      During processing, a child is removed from the sortedDFSChildList of its
      parent when the p2dFwdArcCount of the child reaches zero. Thus, this list
      indicates which subtree children of a vertex still contain unresolved
      pertinence (unembedded forward arcs). The main search for K4 homeomorphs
      uses this list to efficiently determine the portion of the graph in which
      to either find a K4 or to perform reductions than enable more forward arcs
      to be embedded.
*/
typedef struct
{
        int p2dFwdArcCount, sortedDFSChildList;
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
