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

    ///////////////////////////////////////////////////////////////////////////////
    // The top-level operations at the graph, vertex, and edge levels
    ///////////////////////////////////////////////////////////////////////////////

    // Forward declaration of graph structure and graph pointer type definitions
    // (see the end of this header file).
    typedef struct graphStruct graphStruct;
    typedef graphStruct *graphP;

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

/* Graph Flags (bit flags set by various public graphLib APIs):
        Bits 0-3 reserved for base graph class
        Bits 4-7 reserved for graph I/O
        Bits 8-15 reserved for DFS Utils
        Bits 16-23 reserved for Planarity-Related
        bits 24-31 reserved for future expansion
*/
#define gp_GetGraphFlags(theGraph) ((theGraph)->graphFlags)

    // For graph embedding methods and declarations, see graphPlanarity.h

// The initial setting for the edge storage capacity expressed as a constant factor of N,
// which is the number of vertices in the graph. By default, array E is allocated enough
// space to contain 3N edges, which is 6N edge records, but this initial setting
// can be overridden using gp_EnsureEdgeCapacity(). It is especially efficient to change
// to ensure a higher edge capacity if done before calling gp_InitGraph() or gp_Read().
#define DEFAULT_EDGE_LIMIT 3

// This value is returned by gp_AddEdge() and gp_InsertEdge() if adding or inserting
// the edge would exceed the edge capacity limit. The limit can be increased by
// calling gp_EnsureEdgeCapacity(), or by calling gp_DynamicAddEdge().
#define AT_EDGE_CAPACITY_LIMIT -1

    /********************************************************************
     Edge Record Definition

     An edge is defined by a pair of edge records allocated in array E
     of a graph. A pair of edge records represents the edge in the
     adjacency lists of each vertex to which the edge is incident.

     link[2]: the next and previous edge records in the adjacency
              list that contains this edge record.

     v: The vertex neighbor of the vertex whose adjacency list contains
        this edge record (an index into array V).

     flags: Bits 0-15 reserved for library; bits 16 and higher for apps
            Bit 0: Visited
            Bit 1: Marked (2nd visited flag, for while visiting all)
            Bit 2: DFS type has been set, versus not set
            Bit 3: DFS tree edge, versus cycle edge (co-tree edge, etc.)
            Bit 4: DFS edge pointing to descendant, versus to ancestor
            Bit 5: Inverted (same as marking an edge with a "sign" of -1)
            Bit 6: Edge record is directed into the containing vertex only
            Bit 7: Edge record is directed from the containing vertex only
     ********************************************************************/

    typedef struct
    {
        int link[2];
        int neighbor;
        unsigned flags;
    } edgeRec;

    typedef edgeRec *edgeRecP;

#ifdef USE_1BASEDARRAYS

#ifndef DEBUG
#define gp_IsEdge(theGraph, e) (e)
#else
#define gp_IsEdge(theGraph, e)                                                     \
    ((e) == NIL                                                                    \
         ? 0                                                                       \
         : ((e) < gp_EdgeArrayStart(theGraph) || (e) >= gp_EdgeArraySize(theGraph) \
                ? (NOTOK, 0)                                                       \
                : 1))
#endif

#define gp_IsNotEdge(theGraph, e) (!(e))
#define gp_EdgeArrayStart(theGraph) (2)

#else // When using slower 0-based Arrays

#ifndef DEBUG
#define gp_IsEdge(theGraph, e) ((e) != NIL)
#else
#define gp_IsEdge(theGraph, e)                                                     \
    ((e) == NIL                                                                    \
         ? 0                                                                       \
         : ((e) < gp_EdgeArrayStart(theGraph) || (e) >= gp_EdgeArraySize(theGraph) \
                ? (NOTOK, 0)                                                       \
                : 1))
#endif

#define gp_IsNotEdge(theGraph, e) ((e) == NIL)
#define gp_EdgeArrayStart(theGraph) (0)
#endif

#define gp_EdgeInUse(theGraph, e) (gp_IsAnyTypeVertex(theGraph, gp_GetNeighbor(theGraph, e)))
#define gp_EdgeNotInUse(theGraph, e) (gp_IsNotAnyTypeVertex(theGraph, gp_GetNeighbor(theGraph, e)))
#define gp_EdgeArraySize(theGraph) (gp_EdgeArrayStart(theGraph) + ((theGraph)->edgeCapacity << 1))
    int gp_EdgeInUseArraySize(graphP theGraph);

// An edge is represented by two consecutive edge records in the edge array E.
// If an even number, xor 1 will add one; if an odd number, xor 1 will subtract 1
#define gp_GetTwin(theGraph, e) ((e) ^ 1)

// Access to adjacency list pointers
#define gp_GetNextEdge(theGraph, e) (theGraph->E[e].link[0])
#define gp_GetPrevEdge(theGraph, e) (theGraph->E[e].link[1])
#define gp_GetAdjacentEdge(theGraph, e, theLink) (theGraph->E[e].link[theLink])

#define gp_SetNextEdge(theGraph, e, newNextEdge) (theGraph->E[e].link[0] = newNextEdge)
#define gp_SetPrevEdge(theGraph, e, newPrevEdge) (theGraph->E[e].link[1] = newPrevEdge)
#define gp_SetAdjacentEdge(theGraph, e, theLink, newEdge) (theGraph->E[e].link[theLink] = newEdge)

// Get/set 'neighbor' member indicated by edge record e
#define gp_GetNeighbor(theGraph, e) (theGraph->E[e].neighbor)
#define gp_SetNeighbor(theGraph, e, v) (theGraph->E[e].neighbor = v)

// Initializer for edge flags
#define gp_InitEdgeFlags(theGraph, e) (theGraph->E[e].flags = 0)

// Definitions of and access to edge flags
#define EDGE_VISITED_MASK 1
#define gp_GetEdgeVisited(theGraph, e) (theGraph->E[e].flags & EDGE_VISITED_MASK)
#define gp_ClearEdgeVisited(theGraph, e) (theGraph->E[e].flags &= ~EDGE_VISITED_MASK)
#define gp_SetEdgeVisited(theGraph, e) (theGraph->E[e].flags |= EDGE_VISITED_MASK)

// Definition and accessors for the edge marked flag
// Essentially, this is a second visitation flag that can help applications that
// must visit all edges to analyze and mark the ones important for some purpose.
#define EDGE_MARKED_MASK 2
#define gp_GetEdgeMarked(theGraph, e) (theGraph->E[e].flags & EDGE_MARKED_MASK)
#define gp_ClearEdgeMarked(theGraph, e) (theGraph->E[e].flags &= ~EDGE_MARKED_MASK)
#define gp_SetEdgeMarked(theGraph, e) (theGraph->E[e].flags |= EDGE_MARKED_MASK)

// The edge type is defined by bits 2-4, 4+8+16=28
#define EDGE_TYPE_MASK 28

// Call gp_GetEdgeType(), then compare to one of these four possibilities
// EDGE_TYPE_CHILD - edge record points to a neighboring DFS child
// EDGE_TYPE_FORWARD - edge record points to a DFS descendant, not a DFS child
// EDGE_TYPE_PARENT - edge record points to the DFS parent
// EDGE_TYPE_BACK - edge record points to a DFS ancestor, not the DFS parent
// NOTE: A parent/child tree edge has bit 3 (8) set, forward/back edges do not
#define EDGE_TYPE_CHILD 28
#define EDGE_TYPE_FORWARD 20
#define EDGE_TYPE_PARENT 12
#define EDGE_TYPE_BACK 4

// EDGE_TYPE_NOTDEFINED - the edge record type has not been defined
// EDGE_TYPE_TREE - edge record is part of a randomly generated tree
// NOTE: EDGE_TYPE_TREE uses the same bit 3 as DFS parent and child edges above
#define EDGE_TYPE_NOTDEFINED 0
#define EDGE_TYPE_TREE 8

#define gp_GetEdgeType(theGraph, e) (theGraph->E[e].flags & EDGE_TYPE_MASK)
#define gp_ClearEdgeType(theGraph, e) (theGraph->E[e].flags &= ~EDGE_TYPE_MASK)
#define gp_SetEdgeType(theGraph, e, type) (theGraph->E[e].flags |= type)
#define gp_ResetEdgeType(theGraph, e, type) \
    (theGraph->E[e].flags = (theGraph->E[e].flags & ~EDGE_TYPE_MASK) | type)

#define EDGEFLAG_INVERTED_MASK 32
#define gp_GetEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags & EDGEFLAG_INVERTED_MASK)
#define gp_SetEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags |= EDGEFLAG_INVERTED_MASK)
#define gp_ClearEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags &= (~EDGEFLAG_INVERTED_MASK))
#define gp_XorEdgeFlagInverted(theGraph, e) (theGraph->E[e].flags ^= EDGEFLAG_INVERTED_MASK)

#define EDGEFLAG_DIRECTION_INONLY 64
#define EDGEFLAG_DIRECTION_OUTONLY 128
#define EDGEFLAG_DIRECTION_MASK 192

// Returns the direction, if any, of the edge record
#define gp_GetDirection(theGraph, e) (theGraph->E[e].flags & EDGEFLAG_DIRECTION_MASK)

// A direction of 0 clears directedness. Otherwise, edge record e is set
// to direction and e's twin edge record is set to the opposing setting.
#define gp_SetDirection(theGraph, e, direction)                                       \
    {                                                                                 \
        if (direction == EDGEFLAG_DIRECTION_INONLY)                                   \
        {                                                                             \
            theGraph->E[e].flags |= EDGEFLAG_DIRECTION_INONLY;                        \
            theGraph->E[gp_GetTwin(theGraph, e)].flags |= EDGEFLAG_DIRECTION_OUTONLY; \
        }                                                                             \
        else if (direction == EDGEFLAG_DIRECTION_OUTONLY)                             \
        {                                                                             \
            theGraph->E[e].flags |= EDGEFLAG_DIRECTION_OUTONLY;                       \
            theGraph->E[gp_GetTwin(theGraph, e)].flags |= EDGEFLAG_DIRECTION_INONLY;  \
        }                                                                             \
        else                                                                          \
        {                                                                             \
            theGraph->E[e].flags &= ~EDGEFLAG_DIRECTION_MASK;                         \
            theGraph->E[gp_GetTwin(theGraph, e)].flags &= ~EDGEFLAG_DIRECTION_MASK;   \
        }                                                                             \
    }

    /********************************************************************
     Vertex Record Definition (Any Type of Vertex)

     This record definition provides the data members needed for the
     core structural information for both vertices and virtual vertices.
     Vertices are also equipped with additional information provided by
     the vertexInfo structure.

     The vertices of a graph are stored in the first N locations of array V.
     Virtual vertices are secondary vertices used to help represent the
     main vertices in substructural components of a graph (such as in
     biconnected components).

     link[2]: the first and last edge records in the adjacency list
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
            Bit 1: marked, 2nd visited flag, for while visiting all
                    Used in K4 homeomorph search algorithm
            Bit 2: Obstruction type VERTEX_TYPE_SET (versus not set, i.e. VERTEX_TYPE_UNKNOWN)
            Bit 3: Obstruction type qualifier RYW (set) versus RXW (clear)
            Bit 4: Obstruction type qualifier high (set) versus low (clear)
                    Bits 2-4 used in planarity-related algorithms
     ********************************************************************/

    typedef struct
    {
        int link[2];
        int index;
        unsigned flags;
    } anyTypeVertexRec;

    typedef anyTypeVertexRec *anyTypeVertexRecP;

////////////////////////////////////////////
// Accessors for vertex adjacency list links
////////////////////////////////////////////
#define gp_GetFirstEdge(theGraph, v) (theGraph->V[v].link[0])
#define gp_GetLastEdge(theGraph, v) (theGraph->V[v].link[1])
#define gp_GetEdgeByLink(theGraph, v, theLink) (theGraph->V[v].link[theLink])

#define gp_SetFirstEdge(theGraph, v, newFirstEdge) (theGraph->V[v].link[0] = newFirstEdge)
#define gp_SetLastEdge(theGraph, v, newLastEdge) (theGraph->V[v].link[1] = newLastEdge)
#define gp_SetEdgeByLink(theGraph, v, theLink, newEdge) (theGraph->V[v].link[theLink] = newEdge)

///////////////////////////////////
// Vertex iteration-related methods
///////////////////////////////////
#ifdef USE_1BASEDARRAYS

    // The use of *Vertex* alone consistently refers to the initial N vertices.
    // The use of *VirtualVertex* refers to vertex array locations after the first N.
    // The use of *AnyTypeVertex* refers to any non-virtual or virtual vertex

#define gp_GetFirstVertex(theGraph) (1)
#define gp_GetLastVertex(theGraph) (gp_GetN(theGraph))

#define gp_GetFirstVirtualVertex(theGraph) (gp_GetN(theGraph) + 1)
#define gp_GetLastVirtualVertex(theGraph) (gp_GetN(theGraph) + gp_GetNV(theGraph))

#define gp_GetFirstAnyTypeVertex(theGraph) (gp_GetFirstVertex(theGraph))
#define gp_GetLastAnyTypeVertex(theGraph) (gp_GetLastVirtualVertex(theGraph))

#ifndef DEBUG
#define gp_IsVertex(theGraph, v) (v)
#define gp_IsVirtualVertex(theGraph, v) ((v) > gp_GetN(theGraph))
#define gp_IsAnyTypeVertex(theGraph, v) (v)
#else
#define gp_IsVertex(theGraph, v) \
    ((v) == NIL ? 0 : ((v) < gp_GetFirstVertex(theGraph) ? (NOTOK, 0) : ((v) > gp_GetLastVertex(theGraph) ? (NOTOK, 0) : 1)))

// NOTE: gp_IsVirtualVertex() is sometimes called to distinguish between
// an existing non-virtual and a virtual
#define gp_IsVirtualVertex(theGraph, v)                                \
    ((v) == NIL                                                        \
         ? 0                                                           \
         : ((v) < gp_GetFirstVirtualVertex(theGraph)                   \
                ? ((v) < gp_GetFirstVertex(theGraph) ? (NOTOK, 0) : 0) \
                : ((v) > gp_GetLastVirtualVertex(theGraph) ? (NOTOK, 0) : 1)))

#define gp_IsAnyTypeVertex(theGraph, v)              \
    ((v) == NIL                                      \
         ? 0                                         \
         : ((v) < gp_GetFirstAnyTypeVertex(theGraph) \
                ? (NOTOK, 0)                         \
                : ((v) > gp_GetLastAnyTypeVertex(theGraph) ? (NOTOK, 0) : 1)))

#endif

#define gp_IsNotVertex(theGraph, v) (!(gp_IsVertex(theGraph, v)))
#define gp_IsNotVirtualVertex(theGraph, v) (!(gp_IsVirtualVertex(theGraph, v)))
#define gp_IsNotAnyTypeVertex(theGraph, v) (!(gp_IsAnyTypeVertex(theGraph, v)))

#define gp_VertexInRangeAscending(theGraph, v) ((v) <= gp_GetN(theGraph))
#define gp_VertexInRangeDescending(theGraph, v) (v)

#define gp_VirtualVertexInRangeAscending(theGraph, v) ((v) <= gp_GetN(theGraph) + gp_GetNV(theGraph))
#define gp_VirtualVertexInRangeDescending(theGraph, v) ((v) > gp_GetN(theGraph))

#define gp_AnyTypeVertexInRangeAscending(theGraph, v) (gp_VirtualVertexInRangeAscending(theGraph, v))
#define gp_AnyTypeVertexInRangeDescending(theGraph, v) (gp_VirtualVertexInRangeDescending(theGraph, v))

#define gp_VertexArraySize(theGraph) (gp_GetFirstVertex(theGraph) + gp_GetN(theGraph))
#define gp_AnyTypeVertexArraySize(theGraph) (gp_VertexArraySize(theGraph) + gp_GetNV(theGraph))

#define gp_VirtualVertexInUse(theGraph, virtualVertex) (gp_IsEdge(theGraph, gp_GetFirstEdge(theGraph, virtualVertex)))
#define gp_VirtualVertexNotInUse(theGraph, virtualVertex) (gp_IsNotEdge(theGraph, gp_GetFirstEdge(theGraph, virtualVertex)))

#else // Using Slower 0-based Arrays

#define gp_GetFirstVertex(theGraph) (0)
#define gp_GetLastVertex(theGraph) (gp_GetN(theGraph) - 1)

#define gp_GetFirstVirtualVertex(theGraph) (gp_GetN(theGraph))
#define gp_GetLastVirtualVertex(theGraph) (gp_GetN(theGraph) + gp_GetNV(theGraph) - 1)

#define gp_GetFirstAnyTypeVertex(theGraph) (gp_GetFirstVertex(theGraph))
#define gp_GetLastAnyTypeVertex(theGraph) (gp_GetLastVirtualVertex(theGraph))

#ifndef DEBUG
#define gp_IsVertex(theGraph, v) ((v) != NIL)
#define gp_IsVirtualVertex(theGraph, v) ((v) >= gp_GetN(theGraph))
#define gp_IsAnyTypeVertex(theGraph, v) ((v) != NIL)
#else
#define gp_IsVertex(theGraph, v) \
    ((v) == NIL ? 0 : ((v) < gp_GetFirstVertex(theGraph) ? (NOTOK, 0) : ((v) > gp_GetLastVertex(theGraph) ? (NOTOK, 0) : 1)))

#define gp_IsVirtualVertex(theGraph, v)                                \
    ((v) == NIL                                                        \
         ? 0                                                           \
         : ((v) < gp_GetFirstVirtualVertex(theGraph)                   \
                ? ((v) < gp_GetFirstVertex(theGraph) ? (NOTOK, 0) : 0) \
                : ((v) > gp_GetLastVirtualVertex(theGraph) ? (NOTOK, 0) : 1)))

#define gp_IsAnyTypeVertex(theGraph, v) \
    ((v) == NIL ? 0 : ((v) < gp_GetFirstAnyTypeVertex(theGraph) ? (NOTOK, 0) : ((v) > gp_GetLastAnyTypeVertex(theGraph) ? (NOTOK, 0) : 1)))
#endif

#define gp_IsNotVertex(theGraph, v) (!(gp_IsVertex(theGraph, v)))
#define gp_IsNotVirtualVertex(theGraph, v) (!(gp_IsVirtualVertex(theGraph, v)))
#define gp_IsNotAnyTypeVertex(theGraph, v) (!(gp_IsAnyTypeVertex(theGraph, v)))

#define gp_VertexInRangeAscending(theGraph, v) ((v) < gp_GetN(theGraph))
#define gp_VertexInRangeDescending(theGraph, v) ((v) >= 0)

#define gp_VirtualVertexInRangeAscending(theGraph, v) ((v) < gp_GetN(theGraph) + gp_GetNV(theGraph))
#define gp_VirtualVertexInRangeDescending(theGraph, v) ((v) >= gp_GetN(theGraph))

#define gp_AnyTypeVertexInRangeAscending(theGraph, v) (gp_VirtualVertexInRangeAscending(theGraph, v))
#define gp_AnyTypeVertexInRangeDescending(theGraph, v) (gp_VirtualVertexInRangeDescending(theGraph, v))

#define gp_VertexArraySize(theGraph) (gp_GetFirstVertex(theGraph) + gp_GetN(theGraph))
#define gp_AnyTypeVertexArraySize(theGraph) (gp_VertexArraySize(theGraph) + gp_GetNV(theGraph))

#define gp_VirtualVertexInUse(theGraph, virtualVertex) (gp_IsEdge(theGraph, gp_GetFirstEdge(theGraph, virtualVertex)))
#define gp_VirtualVertexNotInUse(theGraph, virtualVertex) (gp_IsNotEdge(theGraph, gp_GetFirstEdge(theGraph, virtualVertex)))

#endif
    ///////////////////////////////////////////
    // End of Vertex iteration-related methods
    //////////////////////////////////////////

// Accessors for "any type" vertex index
#define gp_GetIndex(theGraph, v) (theGraph->V[v].index)
#define gp_SetIndex(theGraph, v, theIndex) (theGraph->V[v].index = theIndex)

// Initializer for "any type" vertex flags
#define gp_InitFlags(theGraph, v) (theGraph->V[v].flags = 0)

// Definition and accessors for the "any type" vertex visited flag
#define ANYTYPEVERTEX_VISITED_MASK 1
#define gp_GetVisited(theGraph, v) (theGraph->V[v].flags & ANYTYPEVERTEX_VISITED_MASK)
#define gp_ClearVisited(theGraph, v) (theGraph->V[v].flags &= ~ANYTYPEVERTEX_VISITED_MASK)
#define gp_SetVisited(theGraph, v) (theGraph->V[v].flags |= ANYTYPEVERTEX_VISITED_MASK)

// Definition and accessors for the "any type" vertex marked flag
// Essentially, this is a second visitation flag that can help applications that
// must visit all vertices to analyze and mark the ones important for some purpose.
#define ANYTYPEVERTEX_MARKED_MASK 2
#define gp_GetMarked(theGraph, v) (theGraph->V[v].flags & ANYTYPEVERTEX_MARKED_MASK)
#define gp_ClearMarked(theGraph, v) (theGraph->V[v].flags &= ~ANYTYPEVERTEX_MARKED_MASK)
#define gp_SetMarked(theGraph, v) (theGraph->V[v].flags |= ANYTYPEVERTEX_MARKED_MASK)

    // DFS-RELATED and PLANARITY-RELATED ONLY
    // Declaration of package-private data type for managing additonal DFS-
    // and planarity-related information associated with each non-virtual vertex
    typedef struct vertexInfoRec vertexInfoRec;
    typedef vertexInfoRec *vertexInfoP;

    // PLANARITY-RELATED ONLY
    // Declaration of package private data type for optimizing management of
    // the external face of a planar embedding as it is being built
    typedef struct extFaceLinkRec extFaceLinkRec;
    typedef extFaceLinkRec *extFaceLinkRecP;

    // DFS-RELATED and PLANARITY-RELATED ONLY
    // Declaration of package-private data type for managing a
    // stack of integers
    typedef struct stackStruct stackStruct;
    typedef stackStruct *stackP;

    // PLANARITY-RELATED ONLY
    // Declaration of package private data type for isolating
    // minimal subgraphs obstructing planarity-related embedding
    typedef struct isolatorContextStruct isolatorContextStruct;
    typedef isolatorContextStruct *isolatorContextP;

    // DFS-RELATED and PLANARITY-RELATED ONLY
    // Declaration of package-private data type for managing a
    // collection of lists of integers
    typedef struct listCollectionStruct listCollectionStruct;
    typedef listCollectionStruct *listCollectionP;

    // PLANARITY-RELATED ONLY
    // Declaration of package private data types for extending the
    // planarity algorithm to implement planarity-related algorithms
    typedef struct graphExtensionStruct graphExtensionStruct;
    typedef graphExtensionStruct *graphExtensionP;

    typedef struct graphFunctionTableStruct graphFunctionTableStruct;
    typedef graphFunctionTableStruct *graphFunctionTableP;

    /********************************************************************
         Graph structure definition
                V : Array of vertex records (allocated size N + NV)
                VI: Array of additional vertexInfoRec structures (allocated size N)
                N : Number of non-virtual vertices (the "order" of the graph)
                NV: Number of virtual vertices (currently always equal to N)

                E : Array of edge records (edge records come in pairs and represent
                    an edge in each of the two vertex endpoints of the edge)
                M: Number of edges (the "size" of the graph)
                edgeCapacity: the maximum number of edges allowed in E
                edgeHoles: free locations in E where edges have been deleted

                theStack: Used by various graph routines needing a stack
                graphFlags: Additional state information about the graph
                embedFlags: records the type of embedding requested (uses EMBEDFLAGS)

                IC: contains additional useful variables for Kuratowski subgraph isolation.
                BicompRootLists: storage space for pertinent bicomp root lists that develop
                                during embedding
                sortedDFSChildLists: storage for the sorted DFS child lists of each vertex
                extFace: Array of (N + NV) external face short circuit records

                extensions: a list of extension data structures
                functions: a table of function pointers that can be overloaded to provide
                           extension behaviors to the graph
        */

    struct graphStruct
    {
        anyTypeVertexRecP V;
        vertexInfoP VI;
        int N, NV;

        edgeRecP E;
        int M, edgeCapacity;
        stackP edgeHoles;

        stackP theStack;
        unsigned graphFlags, embedFlags;

        isolatorContextP IC;
        listCollectionP BicompRootLists, sortedDFSChildLists;
        extFaceLinkRecP extFace;

        graphExtensionP extensions;
        graphFunctionTableP functions;
    };

    typedef struct graphStruct graphStruct;
    typedef graphStruct *graphP;

#ifdef __cplusplus
}
#endif

#endif
