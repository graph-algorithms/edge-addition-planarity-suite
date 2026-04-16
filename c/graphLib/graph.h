#ifndef GRAPH_H
#define GRAPH_H

/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifdef __cplusplus
extern "C"
{
#endif

// Basic declarations, such as for OK, NOTOK, and NIL
#include "lowLevelUtils/appconst.h"

#include "graphStructures.h"

    ///////////////////////////////////////////////////////////////////////////////
    // The top-level operations at the graph, vertex, and edge levels
    ///////////////////////////////////////////////////////////////////////////////

    // Methods related to graph allocation, initialization, and destruction
    graphP gp_New(void);

    int gp_InitGraph(graphP theGraph, int N);
    void gp_ReinitializeGraph(graphP theGraph);

    void gp_Free(graphP *pGraph);

    int gp_EnsureEdgeCapacity(graphP theGraph, int requiredEdgeCapacity);

// Basic graph structure interrogators
// N=# of vertices; NV=# of virtual vertices; M=# of edges
#define gp_GetN(theGraph) ((theGraph)->N)
#define gp_GetNV(theGraph) ((theGraph)->NV)
#define gp_GetM(theGraph) ((theGraph)->M)

#define gp_GetEdgeCapacity(theGraph) ((theGraph)->edgeCapacity)

    // Basic graph utility methods
    int gp_CopyGraph(graphP dstGraph, graphP srcGraph);
    graphP gp_DupGraph(graphP theGraph);
    int gp_CopyAdjacencyLists(graphP dstGraph, graphP srcGraph);

    int gp_CreateRandomGraph(graphP theGraph);
    int gp_CreateRandomGraphEx(graphP theGraph, int numEdges);

    // Basic graph I/O methods: see graphIO.h

    // Basic vertex interrogators
    int gp_IsNeighbor(graphP theGraph, int u, int v);
    int gp_FindEdge(graphP theGraph, int u, int v);
    int gp_GetVertexDegree(graphP theGraph, int v);

    // Basic interrogators for directed graphs
    // The direction can be EDGEFLAG_DIRECTION_INONLY or EDGEFLAG_DIRECTION_OUTONLY
    int gp_IsNeighborDirected(graphP theGraph, int u, int v, unsigned direction);
    int gp_FindDirectedEdge(graphP theGraph, int u, int v, unsigned direction);
    int gp_GetVertexInDegree(graphP theGraph, int v);
    int gp_GetVertexOutDegree(graphP theGraph, int v);

    // Basic graph structure manipulators
    int gp_AddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
    int gp_DynamicAddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
    int gp_InsertEdge(graphP theGraph, int u, int e_u, int e_ulink,
                      int v, int e_v, int e_vlink);
    int gp_DeleteEdge(graphP theGraph, int e);

    // Intermediate graph structure manipulators
    void gp_HideEdge(graphP theGraph, int e);
    void gp_RestoreEdge(graphP theGraph, int e);
    int gp_HideVertex(graphP theGraph, int vertex);
    int gp_RestoreVertex(graphP theGraph);

    // Advanced graph structure manipulators
    int gp_ContractEdge(graphP theGraph, int e);
    int gp_IdentifyVertices(graphP theGraph, int u, int v, int eBefore);
    int gp_RestoreVertices(graphP theGraph);

    // For methods and declarations related to depth-first search (DFS), see graphDFSUtils.h

/* Graph Flags (set by some public APIs):
        GRAPHFLAGS_DFSNUMBERED is set if DFS numbering has been performed on the graph
        GRAPHFLAGS_SORTEDBYDFI records whether the graph is in original vertex order
                or sorted by depth first index. Successive calls to SortVertices()
                toggle this bit.
        GRAPHFLAGS_ZEROBASEDIO is typically set by gp_Read() to indicate that the
                adjacency list representation in a file began with index 0.
*/
#define gp_GetGraphFlags(theGraph) ((theGraph)->graphFlags)

    // For graph embedding methods and declarations, see graphPlanarity.h

#ifdef __cplusplus
}
#endif

#endif
