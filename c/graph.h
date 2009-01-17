#ifndef GRAPH_H
#define GRAPH_H

/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:
John M. Boyer and Wendy J. Myrvold, "On the Cutting Edge: Simplified O(n)
Planarity by Edge Addition,"  Journal of Graph Algorithms and Applications,
Vol. 8, No. 3, pp. 241-273, 2004.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

* Neither the name of the Planarity-Related Graph Algorithms Project nor the names
  of its contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
#define gp_GetTwinArc(theGraph, Arc) (((Arc) & 1) ? (Arc)-1 : (Arc)+1)

// Definitions that enable iteration of arcs in adjacency lists
#define gp_GetFirstArc(theGraph, v) (theGraph->G[v].link[0])
#define gp_GetLastArc(theGraph, v) (theGraph->G[v].link[1])
#define gp_GetNextArc(theGraph, e) (theGraph->G[e].link[0])
#define gp_GetPrevArc(theGraph, e) (theGraph->G[e].link[1])
#define gp_IsArc(theGraph, e) ((e) >= theGraph->edgeOffset)

// Definitions that parameterize getting either a first or last arc in a vertex
// and getting either a next or prev arc in a given arc
#define gp_GetArc(theGraph, v, theLink) (theGraph->G[v].link[theLink])
#define gp_GetAdjacentArc(theGraph, e, theLink) (theGraph->G[e].link[theLink])

// Definitions that enable getting the next or previous arc
// as if the adjacency list were circular, i.e. that the
// first arc and last arc were linked
#define gp_GetNextArcCircular(theGraph, e) \
	(gp_IsArc(theGraph, theGraph->G[e].link[0]) ? \
			theGraph->G[e].link[0] : \
			gp_GetFirstArc(theGraph, theGraph->G[gp_GetTwinArc(theGraph, e)].v))

#define gp_GetPrevArcCircular(theGraph, e) \
(gp_IsArc(theGraph, theGraph->G[e].link[1]) ? \
		theGraph->G[e].link[1] : \
		gp_GetLastArc(theGraph, theGraph->G[gp_GetTwinArc(theGraph, e)].v))

// This definition is used to mark the adjacency links in arcs that are the
// first and last arcs in an adjacency list
// CHANGE_ADJ_LIST: Change this to NIL
#define gp_AdjacencyListEndMark(v) (v)

// Definitions for very low-level adjacency list manipulations
#define gp_SetFirstArc(theGraph, v, newFirstArc) (theGraph->G[v].link[0] = newFirstArc)
#define gp_SetLastArc(theGraph, v, newLastArc) (theGraph->G[v].link[1] = newLastArc)
#define gp_SetNextArc(theGraph, e, newNextArc) (theGraph->G[e].link[0] = newNextArc)
#define gp_SetPrevArc(theGraph, e, newPrevArc) (theGraph->G[e].link[1] = newPrevArc)

#define gp_SetArc(theGraph, v, theLink, newArc) (theGraph->G[v].link[theLink] = newArc)
#define gp_SetAdjacentArc(theGraph, e, theLink, newArc) (theGraph->G[e].link[theLink] = newArc)

#define gp_AttachFirstArc(theGraph, v, arc) \
	gp_SetPrevArc(theGraph, arc, gp_AdjacencyListEndMark(v)); \
    gp_SetFirstArc(theGraph, v, arc)

#define gp_AttachLastArc(theGraph, v, arc) \
	gp_SetNextArc(theGraph, arc, gp_AdjacencyListEndMark(v)); \
    gp_SetLastArc(theGraph, v, arc)

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
