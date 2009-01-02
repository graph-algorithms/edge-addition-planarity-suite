#ifndef GRAPH_H
#define GRAPH_H

/* Copyright (c) 1997-2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public functions for graphs */

#include "graphExtensions.h"

graphP	gp_New(void);

int		gp_InitGraph(graphP theGraph, int N);
void	gp_ReinitializeGraph(graphP theGraph);
int		gp_CopyGraph(graphP dstGraph, graphP srcGraph);
graphP	gp_DupGraph(graphP theGraph);

int		gp_CreateRandomGraph(graphP theGraph);
int		gp_CreateRandomGraphEx(graphP theGraph, int numEdges);

void	gp_Free(graphP *pGraph);

int		gp_Read(graphP theGraph, char *FileName);
#define WRITE_ADJLIST   1
#define WRITE_ADJMATRIX 2
#define WRITE_DEBUGINFO 3
int		gp_Write(graphP theGraph, char *FileName, int Mode);

#define EDGEFLAG_DIRECTION_INONLY 1
#define EDGEFLAG_DIRECTION_OUTONLY 2
#define gp_GetDirection(theGraph, e, edgeFlag_Direction) (theGraph->G[e].flags & edgeFlag_Direction)
void	gp_SetDirection(graphP theGraph, int e, int edgeFlag_Direction);

// An edge is comprised of two arcs, each represented by a graphNode
// in the adjacency lists of the vertex endpoints of the edge.
// This macro returns the calculated twin arc of a given arc.
// If the arc location is even, then the successor is the twin.
// If the arc node is odd, then the predecessor is the twin.
#define gp_GetTwinArc(theGraph, Arc) (((Arc) & 1) ? Arc-1 : Arc+1)

int		gp_IsNeighbor(graphP theGraph, int u, int v);
int		gp_GetNeighborEdgeRecord(graphP theGraph, int u, int v);
int		gp_GetVertexDegree(graphP theGraph, int v);
int		gp_GetVertexInDegree(graphP theGraph, int v);
int		gp_GetVertexOutDegree(graphP theGraph, int v);

int		gp_AddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
int		gp_AddInternalEdge(graphP theGraph, int u, int e_u0, int e_u1,
                                            int v, int e_v0, int e_v1);

void	gp_HideEdge(graphP theGraph, int arcPos);
void	gp_RestoreEdge(graphP theGraph, int arcPos);
int		gp_DeleteEdge(graphP theGraph, int J, int nextLink);

int		gp_CreateDFSTree(graphP theGraph);
int		gp_SortVertices(graphP theGraph);
void	gp_LowpointAndLeastAncestor(graphP theGraph);

int		gp_Embed(graphP theGraph, int embedFlags);
int		gp_TestEmbedResultIntegrity(graphP theGraph, graphP origGraph, int embedResult);

/* Possible Flags for gp_Embed.  The planar and outerplanar settings are supported
   natively.  The rest require extension modules. */

#define EMBEDFLAGS_PLANAR       1
#define EMBEDFLAGS_OUTERPLANAR  2

#define EMBEDFLAGS_DRAWPLANAR   (4|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK23 (16|EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK4  (32|EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK33 (64|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK5  (128|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_MAXIMALPLANARSUBGRAPH    256
#define EMBEDFLAGS_PROJECTIVEPLANAR         512
#define EMBEDFLAGS_TOROIDAL                 1024

#ifdef __cplusplus
}
#endif

#endif
