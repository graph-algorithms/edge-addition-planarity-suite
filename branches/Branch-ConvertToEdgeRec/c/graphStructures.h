#ifndef GRAPHSTRUCTURE_H
#define GRAPHSTRUCTURE_H

/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2010, John M. Boyer
All rights reserved. Includes a reference implementation of the following:

* John M. Boyer. "Simplified O(n) Algorithms for Planar Graph Embedding,
  Kuratowski Subgraph Isolation, and Related Problems". Ph.D. Dissertation,
  University of Victoria, 2001.

* John M. Boyer and Wendy J. Myrvold. "On the Cutting Edge: Simplified O(n)
  Planarity by Edge Addition". Journal of Graph Algorithms and Applications,
  Vol. 8, No. 3, pp. 241-273, 2004.

* John M. Boyer. "A New Method for Efficiently Generating Planar Graph
  Visibility Representations". In P. Eades and P. Healy, editors,
  Proceedings of the 13th International Conference on Graph Drawing 2005,
  Lecture Notes Comput. Sci., Volume 3843, pp. 508-511, Springer-Verlag, 2006.

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

#include <stdio.h>
#include "appconst.h"
#include "listcoll.h"
#include "stack.h"

#include "graphFunctionTable.h"
#include "graphExtensions.private.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The DEFAULT_EDGE_LIMIT expresses the initial setting for the arcCapacity
 * as a constant factor of N, the number of vertices. By default, E is
 * allocated enough space to contain 3N edges, which is 6N arcs (half edges),
 * but this setting can be overridden using gp_EnsureArcCapacity().
 */

#define DEFAULT_EDGE_LIMIT      3

/********************************************************************
 Edge Record Definition

 An edge is defined by a pair of edge records, or arcs, allocated in
 array E of a graph.  An edge record represents the edge in the
 adjacency list of each vertex to which the edge is incident.

 link[2]: the next and previous edge records (arcs) in the adjacency
          list that contains this edge record.

 v: The vertex neighbor of the vertex whose adjacency list contains
    this edge record (an index into array V).

 flags: Bits 0-15 reserved for library; bits 16 and higher for apps
        Bit 0: Visited
        Bit 1: DFS type has been set, versus not set
        Bit 2: DFS tree edge, versus cycle edge (co-tree edge, etc.)
        Bit 3: DFS arc to descendant, versus arc to ancestor
        Bit 4: Inverted (same as marking an edge with a "sign" of -1)
        Bit 5: Arc is directed into the containing vertex only
        Bit 6: Arc is directed from the containing vertex only
 ********************************************************************/

typedef struct
{
	int  link[2];
	int  v;
	unsigned flags;
} edgeRec;

typedef edgeRec * edgeRecP;

#define gp_IsArc(theGraph, e) ((e) != NIL)

// An edge is represented by two consecutive edge records (arcs)
// in the edge array E.
#define gp_GetTwinArc(theGraph, Arc) (((Arc) & 1) ? (Arc)-1 : (Arc)+1)

// Accessors for link[] array
#define gp_GetNextArc(theGraph, e) (theGraph->E[e].link[0])
#define gp_GetPrevArc(theGraph, e) (theGraph->E[e].link[1])
#define gp_GetAdjacentArc(theGraph, e, theLink) (theGraph->E[e].link[theLink])

#define gp_SetNextArc(theGraph, e, newNextArc) (theGraph->E[e].link[0] = newNextArc)
#define gp_SetPrevArc(theGraph, e, newPrevArc) (theGraph->E[e].link[1] = newPrevArc)
#define gp_SetAdjacentArc(theGraph, e, theLink, newArc) (theGraph->E[e].link[theLink] = newArc)

// Accessors for 'v' member
#define gp_GetNeighbor(theGraph, e) (theGraph->E[e].v)
#define gp_SetNeighbor(theGraph, e, neighbor) (theGraph->E[e].v = neighbor)

// Initializer for vertex flags
#define gp_InitEdgeFlags(theGraph, e) (theGraph->E[e].flags = 0)

// Definitions and accessors for edge flags
#define EDGE_VISITED_MASK		1
#define gp_GetEdgeVisited(theGraph, e) (theGraph->E[e].flags&EDGE_VISITED_MASK)
#define gp_ClearEdgeVisited(theGraph, e) (theGraph->E[e].flags &= ~EDGE_VISITED_MASK)
#define gp_SetEdgeVisited(theGraph, e) (theGraph->E[e].flags |= EDGE_VISITED_MASK)

// The edge type is defined by bits 1-3, 2+4+8=14
#define EDGE_TYPE_MASK		14

// Call gp_GetEdgeType(), then compare to one of these four possibilities
// EDGE_TYPE_CHILD - edge record is an arc to a DFS child
// EDGE_TYPE_FORWARD - edge record is an arc to a DFS descendant, not a DFS child
// EDGE_TYPE_PARENT - edge record is an arc to the DFS parent
// EDGE_TYPE_BACK - edge record is an arc to a DFS ancestor, not the DFS parent
#define EDGE_TYPE_CHILD     14
#define EDGE_TYPE_FORWARD   10
#define EDGE_TYPE_PARENT    6
#define EDGE_TYPE_BACK      2

// EDGE_TYPE_NOTDEFINED - the edge record type has not been defined
// EDGE_TYPE_RANDOMTREE - edge record is part of a randomly generated tree
#define EDGE_TYPE_NOTDEFINED	0
#define EDGE_TYPE_RANDOMTREE	4

#define gp_GetEdgeType(theGraph, e) (theGraph->E[e].flags&EDGE_TYPE_MASK)
#define gp_ClearEdgeType(theGraph, e) (theGraph->E[e].flags &= ~EDGE_TYPE_MASK)
#define gp_SetEdgeType(theGraph, e, type) (theGraph->E[e].flags |= type)
#define gp_ResetEdgeType(theGraph, e, type) \
	(theGraph->E[e].flags = (theGraph->E[e].flags & ~EDGE_TYPE_MASK) | type)

#define EDGEFLAG_INVERTED_MASK 16
#define gp_GetEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags & EDGEFLAG_INVERTED_MASK)
#define gp_SetEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags |= EDGEFLAG_INVERTED_MASK)
#define gp_ClearEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags &= (~EDGEFLAG_INVERTED_MASK))
#define gp_XorEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags ^= EDGEFLAG_INVERTED_MASK)

#define EDGEFLAG_DIRECTION_INONLY	32
#define EDGEFLAG_DIRECTION_OUTONLY	64
#define EDGEFLAG_DIRECTION_MASK		96

// Returns the direction, if any, of the edge record
#define gp_GetDirection(theGraph, e) (theGraph->E[e].flags & EDGEFLAG_DIRECTION_MASK)

//A direction of 0 clears directedness. Otherwise, edge record e is set
//to edgeFlag_Direction and e's twin arc is set to the opposing setting.
#define gp_SetDirection(theGraph, e, edgeFlag_Direction) \
{ \
	if (edgeFlag_Direction == EDGEFLAG_DIRECTION_INONLY) \
	{ \
		theGraph->E[e].flags |= EDGEFLAG_DIRECTION_INONLY; \
		theGraph->E[gp_GetTwinArc(theGraph, e)].flags |= EDGEFLAG_DIRECTION_OUTONLY; \
	} \
	else if (edgeFlag_Direction == EDGEFLAG_DIRECTION_OUTONLY) \
	{ \
		theGraph->E[e].flags |= EDGEFLAG_DIRECTION_OUTONLY; \
		theGraph->E[gp_GetTwinArc(theGraph, e)].flags |= EDGEFLAG_DIRECTION_INONLY; \
	} \
	else \
	{ \
		theGraph->E[e].flags &= ~(EDGEFLAG_DIRECTION_INONLY|EDGEFLAG_DIRECTION_OUTONLY); \
		theGraph->E[gp_GetTwinArc(theGraph, e)].flags &= ~EDGEFLAG_DIRECTION_MASK; \
	} \
}

#define gp_CopyEdgeRec(dstGraph, edst, srcGraph, esrc) (dstGraph->E[edst] = srcGraph->E[esrc])

/********************************************************************
 Vertex Record Definition

 This record definition provides the data members needed for the
 core structural information for both vertices and virtual vertices.
 Vertices are also equipped with additional information provided by
 the vertexInfo structure.

 The vertices of a graph are stored in the first N locations of array V.
 Virtual vertices are secondary vertices used to help represent the
 main vertices in substructural components of a graph (e.g. biconnected
 components).

 link[2]: the first and last edge records (arcs) in the adjacency list
          of the vertex.

 index: In vertices, stores either the depth first index of a vertex or
        the original array index of the vertex if the vertices of the
        graph are sorted by DFI.
        In virtual vertices, the index may be used to indicate the vertex
        that the virtual vertex represents, unless an algorithm has some
        other way of making the association (for example, the planarity
        algorithms rely on biconnected components and therefore place
        virtual vertices of a vertex at positions corresponding to the
        DFS children of the vertex).

 flags: Bits 0-15 reserved for library; bits 16 and higher for apps
        Bit 0: visited, for vertices and virtual vertices
				Use in lieu of TYPE_VERTEX_VISITED in K4 algorithm
		Bit 1: Obstruction type VERTEX_TYPE_SET (versus not set, i.e. VERTEX_TYPE_UNKNOWN)
		Bit 2: Obstruction type qualifier RYW (set) versus RXW (clear)
		Bit 3: Obstruction type qualifier high (set) versus low (clear)
 ********************************************************************/

typedef struct
{
	int  link[2];
	int  index;
	unsigned flags;
} vertexRec;

typedef vertexRec * vertexRecP;

#define gp_AdjacencyListEndMark(v) (NIL)

// Accessors for vertex adjacency list links
#define gp_GetFirstArc(theGraph, v) (theGraph->V[v].link[0])
#define gp_GetLastArc(theGraph, v) (theGraph->V[v].link[1])
#define gp_GetArc(theGraph, v, theLink) (theGraph->V[v].link[theLink])

#define gp_SetFirstArc(theGraph, v, newFirstArc) (theGraph->V[v].link[0] = newFirstArc)
#define gp_SetLastArc(theGraph, v, newLastArc) (theGraph->V[v].link[1] = newLastArc)
#define gp_SetArc(theGraph, v, theLink, newArc) (theGraph->V[v].link[theLink] = newArc)

// Accessors for vertex index
#define gp_GetVertexIndex(theGraph, v) (theGraph->V[v].index)
#define gp_SetVertexIndex(theGraph, v, theIndex) (theGraph->V[v].index = theIndex)

// Initializer for vertex flags
#define gp_InitVertexFlags(theGraph, v) (theGraph->V[v].flags = 0)

// Definitions and accessors for vertex flags
#define VERTEX_VISITED_MASK		1
#define gp_GetVertexVisited(theGraph, v) (theGraph->V[v].flags&VERTEX_VISITED_MASK)
#define gp_ClearVertexVisited(theGraph, v) (theGraph->V[v].flags &= ~VERTEX_VISITED_MASK)
#define gp_SetVertexVisited(theGraph, v) (theGraph->V[v].flags |= VERTEX_VISITED_MASK)

// The obstruction type is defined by bits 1-3, 2+4+8=14
#define VERTEX_OBSTRUCTIONTYPE_MASK		14

// Call gp_GetVertexObstructionType, then compare to one of these four possibilities
// VERTEX_OBSTRUCTIONTYPE_HIGH_RXW - On the external face path between vertices R and X
// VERTEX_OBSTRUCTIONTYPE_LOW_RXW  - X or on the external face path between vertices X and W
// VERTEX_OBSTRUCTIONTYPE_HIGH_RYW - On the external face path between vertices R and Y
// VERTEX_OBSTRUCTIONTYPE_LOW_RYW  - Y or on the external face path between vertices Y and W
// VERTEX_OBSTRUCTIONTYPE_UNKNOWN  - corresponds to all three bits off
#define VERTEX_OBSTRUCTIONTYPE_HIGH_RXW    	10
#define VERTEX_OBSTRUCTIONTYPE_LOW_RXW     	2
#define VERTEX_OBSTRUCTIONTYPE_HIGH_RYW    	14
#define VERTEX_OBSTRUCTIONTYPE_LOW_RYW    	6
#define VERTEX_OBSTRUCTIONTYPE_UNKNOWN		0

#define gp_GetVertexObstructionType(theGraph, v) (theGraph->V[v].flags&VERTEX_OBSTRUCTIONTYPE_MASK)
#define gp_ClearVertexObstructionType(theGraph, v) (theGraph->V[v].flags &= ~VERTEX_OBSTRUCTIONTYPE_MASK)
#define gp_SetVertexObstructionType(theGraph, v, type) (theGraph->V[v].flags |= type)
#define gp_ResetVertexObstructionType(theGraph, v, type) \
	(theGraph->V[v].flags = (theGraph->V[v].flags & ~VERTEX_OBSTRUCTIONTYPE_MASK) | type)

#define gp_CopyVertexRec(dstGraph, vdst, srcGraph, vsrc) (dstGraph->V[vdst] = srcGraph->V[vsrc])

#define gp_SwapVertexRec(dstGraph, vdst, srcGraph, vsrc) \
	{ \
		vertexRec tempV = dstGraph->V[vdst]; \
		dstGraph->V[vdst] = srcGraph->V[vsrc]; \
		srcGraph->V[vsrc] = tempV; \
	}

/********************************************************************
 This structure defines a pair of links used by each vertex and virtual vertex
 to create "short circuit" paths that eliminate unimportant vertices from
 the external face, enabling more efficient traversal of the external face.

 It is also possible to embed the "short circuit" edges, but this approach
 creates a better separation of concerns, imparts greater clarity, and
 removes exceptionalities for handling additional false edges.

 vertex[2]: The two adjacent vertices along the external face, ignoring
            inactive vertices.
 inversionFlag: In the special case where the external face is reduced to
            two vertices, a virtual vertex bicomp root R plus one non-virtual
            vertex W, then vertex[0] becomes equal to vertex[1], so this
            flag is used to indicate whether W has an inverse orientation
            from R.  This is needed when (R, W) is eventually merged into
            a larger bicomp.
            This is distinct from the edge inverted flag, which takes a record
            of whether a bicomp was flipped when it was merged so that the
            imparting of a consistent orientation of vertices in a bicomp
            can be deferred to a post-processing step of the embedding method.
*/

typedef struct
{
    int vertex[2];
    int inversionFlag;
} extFaceLinkRec;

typedef extFaceLinkRec * extFaceLinkRecP;

#define gp_GetExtFaceVertex(theGraph, v, link) (theGraph->extFace[v].vertex[link])
#define gp_SetExtFaceVertex(theGraph, v, link, theVertex) (theGraph->extFace[v].vertex[link] = theVertex)

#define gp_GetExtFaceInversionFlag(theGraph, v) (theGraph->extFace[v].inversionFlag)
#define gp_ClearExtFaceInversionFlag(theGraph, v) (theGraph->extFace[v].inversionFlag = 0)
#define gp_SetExtFaceInversionFlag(theGraph, v) (theGraph->extFace[v].inversionFlag = 1)
#define gp_ResetExtFaceInversionFlag(theGraph, v, flag) (theGraph->extFace[v].inversionFlag = flag)
#define gp_XorExtFaceInversionFlag(theGraph, v) (theGraph->extFace[v].inversionFlag ^= 1)

/********************************************************************
 Vertex Info Structure Definition.

 This structure equips the primary (non-virtual) vertices with additional
 information needed for lowpoint and planarity-related algorithms.

	parent: The DFI of the DFS tree parent of this vertex
	leastAncestor: min(DFI of neighbors connected by backedge)
	lowpoint: min(leastAncestor, min(lowpoint of DFS Children))

	visitedInfo: enables algorithms to manage vertex visitation with more than
				 just a flag.  For example, the planarity test flags visitation
				 as a step number that implicitly resets on each step, whereas
				 part of the planar drawing method signifies a first visitation
				 by storing the index of the first edge used to reach a vertex

	pertinentAdjacencyInfo: Used by the planarity method; during walk-up, each vertex
	            that is directly adjacent via a back edge to the vertex currently
                being embedded will have the forward edge's index stored in
                this field.  During walkdown, each vertex for which this
                field is set will cause a back edge to be embedded.
                Implicitly resets at each vertex step of the planarity method
	pertinentBicompList: used by Walkup to store a list of child bicomps of
                a vertex descendant of the current vertex that are pertinent
                and must be merged by the Walkdown in order to embed the cycle
                edges of the current vertex.  In this implementation,
                externally active pertinent child bicomps are placed at the end
                of the list as an easy way to make sure all internally active
                bicomps are processed first.
	separatedDFSChildList: contains list DFS children of this vertex in
                non-descending order by lowpoint (sorted in linear time).
                When merging bicomp rooted by edge (r, c) into vertex v (i.e.
                merging root copy r with parent copy v), the vertex c is
                removed from the separatedDFSChildList of v.
                A vertex's status-- inactive, internally active, externally
                active-- is determined by the lesser of its leastAncestor and
                the least lowpoint from among only those DFS children that
                aren't in the same bicomp with the vertex.
	fwdArcList: at the start of embedding, the back edges from a vertex
                to its DFS descendants are separated from the main adjacency
                list and placed in a circular list until they are embedded.
                This member indicates a node in that list.
*/

typedef struct
{
	int parent, leastAncestor, lowpoint;

    int visitedInfo;

    int pertinentAdjacencyInfo,
		pertinentBicompList,
		separatedDFSChildList,
		fwdArcList;
} vertexInfo;

typedef vertexInfo * vertexInfoP;

#define gp_GetVertexVisitedInfo(theGraph, v) (theGraph->VI[v].visitedInfo)
#define gp_SetVertexVisitedInfo(theGraph, v, theVisitedInfo) (theGraph->VI[v].visitedInfo = theVisitedInfo)

#define gp_GetVertexParent(theGraph, v) (theGraph->VI[v].parent)
#define gp_SetVertexParent(theGraph, v, theParent) (theGraph->VI[v].parent = theParent)

#define gp_GetVertexLeastAncestor(theGraph, v) (theGraph->VI[v].leastAncestor)
#define gp_SetVertexLeastAncestor(theGraph, v, theLeastAncestor) (theGraph->VI[v].leastAncestor = theLeastAncestor)

#define gp_GetVertexLowpoint(theGraph, v) (theGraph->VI[v].lowpoint)
#define gp_SetVertexLowpoint(theGraph, v, theLowpoint) (theGraph->VI[v].lowpoint = theLowpoint)

#define gp_GetVertexPertinentAdjacencyInfo(theGraph, v) (theGraph->VI[v].pertinentAdjacencyInfo)
#define gp_SetVertexPertinentAdjacencyInfo(theGraph, v, thePertinentAdjacencyInfo) (theGraph->VI[v].pertinentAdjacencyInfo = thePertinentAdjacencyInfo)

#define gp_GetVertexPertinentBicompList(theGraph, v) (theGraph->VI[v].pertinentBicompList)
#define gp_SetVertexPertinentBicompList(theGraph, v, thePertinentBicompList) (theGraph->VI[v].pertinentBicompList = thePertinentBicompList)

#define gp_GetVertexSeparatedDFSChildList(theGraph, v) (theGraph->VI[v].separatedDFSChildList)
#define gp_SetVertexSeparatedDFSChildList(theGraph, v, theSeparatedDFSChildList) (theGraph->VI[v].separatedDFSChildList = theSeparatedDFSChildList)

#define gp_GetVertexFwdArcList(theGraph, v) (theGraph->VI[v].fwdArcList)
#define gp_SetVertexFwdArcList(theGraph, v, theFwdArcList) (theGraph->VI[v].fwdArcList = theFwdArcList)

#define gp_CopyVertexInfo(dstGraph, dstI, srcGraph, srcI) (dstGraph->VI[dstI] = srcGraph->VI[srcI])

#define gp_SwapVertexInfo(dstGraph, dstPos, srcGraph, srcPos) \
	{ \
		vertexInfo tempVI = dstGraph->VI[dstPos]; \
		dstGraph->VI[dstPos] = srcGraph->VI[srcPos]; \
		srcGraph->VI[srcPos] = tempVI; \
	}

/********************************************************************
 Variables needed in embedding by Kuratowski subgraph isolator:
        minorType: the type of planarity obstruction found.
        v: the current vertex I
        r: the root of the bicomp on which the Walkdown failed
        x,y: stopping vertices on bicomp rooted by r
        w: pertinent vertex on ext. face path below x and y
        px, py: attachment points of x-y path,
        z: Unused except in minors D and E (not needed in A, B, C).

        ux,dx: endpoints of unembedded edge that helps connext x with
                ancestor of v
        uy,dy: endpoints of unembedded edge that helps connext y with
                ancestor of v
        dw: descendant endpoint in unembedded edge to v
        uz,dz: endpoints of unembedded edge that helps connext z with
                ancestor of v (for minors B and E, not A, C, D).
*/

typedef struct
{
    int minorType;
    int v, r, x, y, w, px, py, z;
    int ux, dx, uy, dy, dw, uz, dz;
} isolatorContext;

typedef isolatorContext * isolatorContextP;

#define MINORTYPE_A         1
#define MINORTYPE_B         2
#define MINORTYPE_C         4
#define MINORTYPE_D         8
#define MINORTYPE_E         16
#define MINORTYPE_E1        32
#define MINORTYPE_E2        64
#define MINORTYPE_E3        128
#define MINORTYPE_E4        256

#define MINORTYPE_E5        512
#define MINORTYPE_E6        1024
#define MINORTYPE_E7        2048

/********************************************************************
 Graph structure definition
        V : Array of vertex records (allocated size N + NV)
        VI: Array of additional vertexInfo structures (allocated size N)
        N : Number of primary vertices (the "order" of the graph)
        NV: Number of virtual vertices (currently always equal to N)

        E : Array of edge records (edge records come in pairs and represent half edges, or arcs)
        M: Number of edges (the "size" of the graph)
        arcCapacity: the maximum number of edge records allowed in E (the size of E)
        edgeHoles: free locations in E where edges have been deleted

        theStack: Used by various graph routines needing a stack
        internalFlags: Additional state information about the graph
        embedFlags: controls type of embedding (e.g. planar)

        IC: contains additional useful variables for Kuratowski subgraph isolation.
        BicompLists: storage space for pertinent bicomp lists that develop
                        during embedding
        DFSChildLists: storage space for separated DFS child lists that
                        develop during embedding
        buckets: Used to help bucket sort the separatedDFSChildList elements
                    of all vertices (see _CreateSortedSeparatedDFSChildLists())
        bin: Used to help bucket sort the separatedDFSChildList elements
                    of all vertices (see _CreateSortedSeparatedDFSChildLists())
        extFace: Array of (N + NV) external face short circuit records

        extensions: a list of extension data structures
        functions: a table of function pointers that can be overloaded to provide
                   extension behaviors to the graph
*/

typedef struct
{
        vertexRecP V;
        vertexInfoP VI;
        int N, NV;

        edgeRecP E;
        int M, arcCapacity;
        stackP edgeHoles;

        stackP theStack;
        int internalFlags, embedFlags;

        isolatorContext IC;
        listCollectionP BicompLists, DFSChildLists;
        int *buckets;
        listCollectionP bin;
        extFaceLinkRecP extFace;

        graphExtensionP extensions;
        graphFunctionTable functions;

} baseGraphStructure;

typedef baseGraphStructure * graphP;

/* Flags for graph:
        FLAGS_DFSNUMBERED is set if DFSNumber() has succeeded for the graph
        FLAGS_SORTEDBYDFI records whether the graph is in original vertex
                order or sorted by depth first index.  Successive calls to
                SortVertices() toggle this bit.
        FLAGS_OBSTRUCTIONFOUND is set by gp_Embed() if an embedding obstruction
                was isolated in the graph returned.  It is cleared by gp_Embed()
                if an obstruction was not found.  The flag is used by
                gp_TestEmbedResultIntegrity() to decide what integrity tests to run.
*/

#define FLAGS_DFSNUMBERED       1
#define FLAGS_SORTEDBYDFI       2
#define FLAGS_OBSTRUCTIONFOUND  4

/********************************************************************
 More link structure accessors/manipulators
 ********************************************************************/

// Definitions that enable getting the next or previous arc
// as if the adjacency list were circular, i.e. that the
// first arc and last arc were linked
#define gp_GetNextArcCircular(theGraph, e) \
	(gp_IsArc(theGraph, theGraph->E[e].link[0]) ? \
			theGraph->E[e].link[0] : \
			gp_GetFirstArc(theGraph, theGraph->E[gp_GetTwinArc(theGraph, e)].v))

#define gp_GetPrevArcCircular(theGraph, e) \
	(gp_IsArc(theGraph, theGraph->E[e].link[1]) ? \
		theGraph->E[e].link[1] : \
		gp_GetLastArc(theGraph, theGraph->E[gp_GetTwinArc(theGraph, e)].v))

// Definitions that make the cross-link binding between a vertex and an arc
// The old first or last arc should be bound to this arc by separate calls,
// e.g. see gp_AttachFirstArc() and gp_AttachLastArc()
#define gp_BindFirstArc(theGraph, v, arc) \
	{ \
		gp_SetPrevArc(theGraph, arc, gp_AdjacencyListEndMark(v)); \
		gp_SetFirstArc(theGraph, v, arc); \
    }

#define gp_BindLastArc(theGraph, v, arc) \
	{ \
    	gp_SetNextArc(theGraph, arc, gp_AdjacencyListEndMark(v)); \
    	gp_SetLastArc(theGraph, v, arc); \
    }

// Attaches an arc between the current binding between a vertex and its first arc
#define gp_AttachFirstArc(theGraph, v, arc) \
	{ \
		if (gp_IsArc(theGraph, gp_GetFirstArc(theGraph, v))) \
		{ \
			gp_SetNextArc(theGraph, arc, gp_GetFirstArc(theGraph, v)); \
			gp_SetPrevArc(theGraph, gp_GetFirstArc(theGraph, v), arc); \
		} \
		else gp_BindLastArc(theGraph, v, arc); \
		gp_BindFirstArc(theGraph, v, arc); \
	}

// Attaches an arc between the current binding betwen a vertex and its last arc
#define gp_AttachLastArc(theGraph, v, arc) \
	{ \
		if (gp_IsArc(theGraph, gp_GetLastArc(theGraph, v))) \
		{ \
			gp_SetPrevArc(theGraph, arc, gp_GetLastArc(theGraph, v)); \
			gp_SetNextArc(theGraph, gp_GetLastArc(theGraph, v), arc); \
		} \
		else gp_BindFirstArc(theGraph, v, arc); \
		gp_BindLastArc(theGraph, v, arc); \
	}

// Moves an arc that is in the adjacency list of v to the start of the adjacency list
#define gp_MoveArcToFirst(theGraph, v, arc) \
	if (arc != gp_GetFirstArc(theGraph, v)) \
	{ \
		/* If the arc is last in the adjacency list of uparent,
		   then we delete it by adjacency list end management */ \
		if (arc == gp_GetLastArc(theGraph, v)) \
		{ \
		    gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, arc), gp_AdjacencyListEndMark(v)); \
			gp_SetLastArc(theGraph, v, gp_GetPrevArc(theGraph, arc)); \
		} \
		/* Otherwise, we delete the arc from the middle of the list */ \
		else \
		{ \
			gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, arc), gp_GetNextArc(theGraph, arc)); \
			gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, arc), gp_GetPrevArc(theGraph, arc)); \
		} \
\
		/* Now add arc e as the new first arc of uparent.
		   Note that the adjacency list is non-empty at this time */ \
		 gp_SetNextArc(theGraph, arc, gp_GetFirstArc(theGraph, v)); \
		 gp_SetPrevArc(theGraph, gp_GetFirstArc(theGraph, v), arc); \
		 gp_BindFirstArc(theGraph, v, arc); \
	}

// Moves an arc that is in the adjacency list of v to the end of the adjacency list
#define gp_MoveArcToLast(theGraph, v, arc) \
	if (arc != gp_GetLastArc(theGraph, v)) \
	{ \
		 /* If the arc is first in the adjacency list of vertex v,
		    then we delete it by adjacency list end management */ \
		 if (arc == gp_GetFirstArc(theGraph, v)) \
		 { \
			 gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, arc), gp_AdjacencyListEndMark(v)); \
			 gp_SetFirstArc(theGraph, v, gp_GetNextArc(theGraph, arc)); \
		 } \
		 /* Otherwise, we delete the arc from the middle of the list */ \
		 else \
		 { \
			 gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, arc), gp_GetNextArc(theGraph, arc)); \
			 gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, arc), gp_GetPrevArc(theGraph, arc)); \
		 } \
\
		 /* Now add the arc as the new last arc of v.
		    Note that the adjacency list is non-empty at this time */ \
		 gp_SetPrevArc(theGraph, arc, gp_GetLastArc(theGraph, v)); \
		 gp_SetNextArc(theGraph, gp_GetLastArc(theGraph, v), arc); \
		 gp_BindLastArc(theGraph, v, arc); \
	}

// Methods for attaching an arc into the adjacency list or detaching an arc from it.
// The terms AddArc, InsertArc and DeleteArc are not used because the arcs are not
// inserted or added to or deleted from storage (only whole edges are inserted or deleted)
void	gp_AttachArc(graphP theGraph, int v, int e, int link, int newArc);
void 	gp_DetachArc(graphP theGraph, int arc);

/********************************************************************
 Vertex activity categories
 ********************************************************************/

#define VAS_INACTIVE    0
#define VAS_INTERNAL    1
#define VAS_EXTERNAL    2

/********************************************************************
 _VertexActiveStatus()
 Tells whether a vertex is externally active, internally active
 or inactive.
 ********************************************************************/

#define _VertexActiveStatus(theGraph, theVertex, I) \
        (EXTERNALLYACTIVE(theGraph, theVertex, I) \
         ? VAS_EXTERNAL \
         : PERTINENT(theGraph, theVertex) \
           ? VAS_INTERNAL \
           : VAS_INACTIVE)

/********************************************************************
 PERTINENT()
 A vertex is pertinent in a partially processed graph if there is an
 unprocessed back edge between the vertex I whose edges are currently
 being processed and either the vertex or a DFS descendant D of the
 vertex not in the same bicomp as the vertex.

 The vertex is either directly adjacent to I by an unembedded back edge
 or there is an unembedded back edge (I, D) and the vertex is a cut
 vertex in the partially processed graph along the DFS tree path from
 D to I.

 Pertinence is a dynamic property that can change for a vertex after
 each edge addition.  In other words, a vertex can become non-pertinent
 during step I as more back edges to I are embedded.
 ********************************************************************/

#define PERTINENT(theGraph, theVertex) \
        (theGraph->VI[theVertex].pertinentAdjacencyInfo != NIL || \
         theGraph->VI[theVertex].pertinentBicompList != NIL)

/********************************************************************
 FUTUREPERTINENT()
 A vertex is future-pertinent in a partially processed graph if
 there is an unprocessed back edge between a DFS ancestor A of the
 vertex I whose edges are currently being processed and either the
 vertex or a DFS descendant D of the vertex not in the same bicomp
 as the vertex.

 The vertex is either directly adjacent to A by an unembedded back edge
 or there is an unembedded back edge (A, D) and the vertex is a cut
 vertex in the partially processed graph along the DFS tree path from
 D to A.

 If no more edges are added to the partially processed graph prior to
 processing the edges of A, then the vertex would be pertinent.
 The addition of edges to the partially processed graph can alter
 both the pertinence and future pertinence of a vertex.  For example,
 if the vertex is pertinent due to an unprocessed back edge (I, D1) and
 future pertinent due to an unprocessed back edge (A, D2), then the
 vertex may lose both its pertinence and future pertinence when edge
 (I, D1) is added if D2 is equal to or an ancestor of D1.

 Generally, pertinence and future pertinence are dynamic properties
 that can change for a vertex after each edge addition.
 ********************************************************************/

#define FUTUREPERTINENT(theGraph, theVertex, I) \
        (  theGraph->VI[theVertex].leastAncestor < I || \
           (theGraph->VI[theVertex].separatedDFSChildList != NIL && \
            theGraph->VI[theGraph->VI[theVertex].separatedDFSChildList].lowpoint < I) )

/********************************************************************
 EXTERNALLYACTIVE()
 Tells whether a vertex is still externally active in step I.
 A vertex can become inactive during step I as edges are embedded.

 In planarity-related algorithms, external activity is the same as
 future pertinence.  A vertex must be kept on the external face of
 the partial embedding until its pertinence and future pertinence
 are resolved through edge additions.

 For outerplanarity-related algorithms, all vertices are always
 externally active, since they must always remain on the external face.
 ********************************************************************/

#define EXTERNALLYACTIVE(theGraph, theVertex, I) \
        ( ( theGraph->embedFlags & EMBEDFLAGS_OUTERPLANAR) || \
          FUTUREPERTINENT(theGraph, theVertex, I) )

#ifdef __cplusplus
}
#endif

#endif

