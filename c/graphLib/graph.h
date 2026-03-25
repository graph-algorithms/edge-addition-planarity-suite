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

#include "graphStructures.h"

#include "io/g6-read-iterator.h"
#include "io/g6-write-iterator.h"
#include "io/strbuf.h"
#include "io/strOrFile.h"

#include "extensionSystem/graphExtensions.h"

    ///////////////////////////////////////////////////////////////////////////////
    // Definitions for higher-order operations at the vertex, edge and graph levels
    ///////////////////////////////////////////////////////////////////////////////

    graphP gp_New(void);

    int gp_InitGraph(graphP theGraph, int N);
    void gp_ReinitializeGraph(graphP theGraph);
    int gp_CopyAdjacencyLists(graphP dstGraph, graphP srcGraph);
    int gp_CopyGraph(graphP dstGraph, graphP srcGraph);
    graphP gp_DupGraph(graphP theGraph);

    int gp_CreateRandomGraph(graphP theGraph);
    int gp_CreateRandomGraphEx(graphP theGraph, int numEdges);

    void gp_Free(graphP *pGraph);

    int gp_Read(graphP theGraph, char const *FileName);
    int gp_ReadFromString(graphP theGraph, char *inputStr);

#define WRITE_ADJLIST 1
#define WRITE_ADJMATRIX 2
#define WRITE_DEBUGINFO 3
#define WRITE_G6 4

    int gp_Write(graphP theGraph, char const *FileName, int Mode);
    int gp_WriteToString(graphP theGraph, char **pOutputStr, int Mode);

    int gp_IsNeighbor(graphP theGraph, int u, int v);
    int gp_IsNeighborDirected(graphP theGraph, int u, int v, unsigned direction);
    int gp_FindArc(graphP theGraph, int u, int v);
    int gp_FindDirectedArc(graphP theGraph, int u, int v, unsigned direction);

    int gp_GetVertexDegree(graphP theGraph, int v);
    int gp_GetVertexInDegree(graphP theGraph, int v);
    int gp_GetVertexOutDegree(graphP theGraph, int v);

    int gp_GetArcCapacity(graphP theGraph);
    int gp_EnsureArcCapacity(graphP theGraph, int requiredArcCapacity);

    int gp_AddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
    int gp_DynamicAddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
    int gp_InsertEdge(graphP theGraph, int u, int e_u, int e_ulink,
                      int v, int e_v, int e_vlink);
    int gp_DeleteEdge(graphP theGraph, int e);

    void gp_HideEdge(graphP theGraph, int e);
    void gp_RestoreEdge(graphP theGraph, int e);
    int gp_HideVertex(graphP theGraph, int vertex);
    int gp_RestoreVertex(graphP theGraph);

    int gp_ContractEdge(graphP theGraph, int e);
    int gp_IdentifyVertices(graphP theGraph, int u, int v, int eBefore);
    int gp_RestoreVertices(graphP theGraph);

    int gp_CreateDFSTree(graphP theGraph);
    int gp_SortVertices(graphP theGraph);
    int gp_LowpointAndLeastAncestor(graphP theGraph);
    int gp_LeastAncestor(graphP theGraph);

    int gp_Embed(graphP theGraph, int embedFlags);
    int gp_TestEmbedResultIntegrity(graphP theGraph, graphP origGraph, int embedResult);

    /* Possible graph embedFlags for gp_Embed.
        The planar and outerplanar settings are supported natively
        The rest are supported via  extension modules. */

#define EMBEDFLAGS_PLANAR 1
#define EMBEDFLAGS_OUTERPLANAR 2

#define EMBEDFLAGS_DRAWPLANAR (4 | EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK23 (16 | EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK4 (32 | EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK33 (64 | EMBEDFLAGS_PLANAR)

// Reserved for the future possible extension modules
#define EMBEDFLAGS_SEARCHFORK5 (128 | EMBEDFLAGS_PLANAR)
#define EMBEDFLAGS_MAXIMALPLANARSUBGRAPH 256
#define EMBEDFLAGS_PROJECTIVEPLANAR 512
#define EMBEDFLAGS_TOROIDAL 1024

#ifdef __cplusplus
}
#endif

#endif
