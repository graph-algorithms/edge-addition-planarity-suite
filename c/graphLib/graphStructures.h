#ifndef GRAPHSTRUCTURE_H
#define GRAPHSTRUCTURE_H

/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdio.h>

#include "lowLevelUtils/listcoll.h"
#include "lowLevelUtils/stack.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /********************************************************************
     Vertex Info Structure Definition.

     This structure equips the non-virtual vertices with additional
     information needed for lowpoint and planarity-related algorithms.

        parent: The DFI of the DFS tree parent of this vertex
        leastAncestor: min(DFI of neighbors connected by backedge)
        lowpoint: min(leastAncestor, min(lowpoint of DFS Children))

        visitedInfo: enables algorithms to manage vertex visitation with more than
                    just a flag.  For example, the planarity test flags visitation
                    as a step number that implicitly resets on each step, whereas
                    part of the planar drawing method signifies a first visitation
                    by storing the index of the first edge used to reach a vertex
        pertinentEdge: Used by the planarity method; during Walkup, each vertex
                    that is directly adjacent via a back edge to the vertex v
                    currently being embedded will have the forward edge's index
                    stored in this field.  During Walkdown, each vertex for which
                    this field is set will cause a back edge to be embedded.
                    Implicitly resets at each vertex step of the planarity method
        pertinentRootsList: used by Walkup to store a list of child bicomp roots of
                    a vertex descendant of the current vertex that are pertinent
                    and must be merged by the Walkdown in order to embed the cycle
                    edges of the current vertex.  Future pertinent child bicomp roots
                    are placed at the end of the list to ensure bicomps that are
                    only pertinent are processed first.
        futurePertinentChild: indicates a DFS child with a lowpoint less than the
                    current vertex v.  This member is initialized to the start of
                    the sortedDFSChildList and is advanced in a relaxed manner as
                    needed until one with a lowpoint less than v is found or until
                    there are no more children.
        sortedDFSChildList: at the start of embedding, the list of DFS children of
                    this vertex is calculated in ascending order by DFI (sorted in
                    linear time). The list is used during Walkdown processing of
                    a vertex to process all of its children.  It is also used in
                    future pertinence management when processing the ancestors of
                    the vertex. When a child C is merged into the same bicomp as
                    the vertex, it is removed from the list.
        fwdEdgeList: at the start of embedding, the "back" edges from a vertex to
                    its DFS *descendants* (i.e. the forward edge records) are
                    separated from the main adjacency list and placed in a
                    circular list until they are embedded. The list is sorted in
                    ascending DFI order of the descendants (in linear time).
                    This member indicates (by index) a node in that list.
    */

    typedef struct
    {
        int parent, leastAncestor, lowpoint;

        int visitedInfo;

        int pertinentEdge,
            pertinentRoots,
            futurePertinentChild,
            sortedDFSChildList,
            fwdEdgeList;
    } vertexInfo;

    typedef vertexInfo *vertexInfoP;

#define gp_GetVertexVisitedInfo(theGraph, v) (theGraph->VI[v].visitedInfo)
#define gp_SetVertexVisitedInfo(theGraph, v, theVisitedInfo) (theGraph->VI[v].visitedInfo = theVisitedInfo)

#define gp_GetVertexParent(theGraph, v) (theGraph->VI[v].parent)
#define gp_SetVertexParent(theGraph, v, theParent) (theGraph->VI[v].parent = theParent)

#define gp_GetVertexLeastAncestor(theGraph, v) (theGraph->VI[v].leastAncestor)
#define gp_SetVertexLeastAncestor(theGraph, v, theLeastAncestor) (theGraph->VI[v].leastAncestor = theLeastAncestor)

#define gp_GetVertexLowpoint(theGraph, v) (theGraph->VI[v].lowpoint)
#define gp_SetVertexLowpoint(theGraph, v, theLowpoint) (theGraph->VI[v].lowpoint = theLowpoint)

#define gp_GetVertexPertinentEdge(theGraph, v) (theGraph->VI[v].pertinentEdge)
#define gp_SetVertexPertinentEdge(theGraph, v, e) (theGraph->VI[v].pertinentEdge = e)

#define gp_GetVertexPertinentRootsList(theGraph, v) (theGraph->VI[v].pertinentRoots)
#define gp_SetVertexPertinentRootsList(theGraph, v, pertinentRootsHead) (theGraph->VI[v].pertinentRoots = pertinentRootsHead)

#define gp_GetVertexFirstPertinentRoot(theGraph, v) gp_GetBicompRootFromDFSChild(theGraph, theGraph->VI[v].pertinentRoots)
#define gp_GetVertexFirstPertinentRootChild(theGraph, v) (theGraph->VI[v].pertinentRoots)
#define gp_GetVertexLastPertinentRoot(theGraph, v) gp_GetBicompRootFromDFSChild(theGraph, LCGetPrev(theGraph->BicompRootLists, theGraph->VI[v].pertinentRoots, NIL))
#define gp_GetVertexLastPertinentRootChild(theGraph, v) LCGetPrev(theGraph->BicompRootLists, theGraph->VI[v].pertinentRoots, NIL)

#define gp_DeleteVertexPertinentRoot(theGraph, v, R)                                     \
    gp_SetVertexPertinentRootsList(theGraph, v,                                          \
                                   LCDelete(theGraph->BicompRootLists,                   \
                                            gp_GetVertexPertinentRootsList(theGraph, v), \
                                            gp_GetDFSChildFromBicompRoot(theGraph, R)))

#define gp_PrependVertexPertinentRoot(theGraph, v, R)                                     \
    gp_SetVertexPertinentRootsList(theGraph, v,                                           \
                                   LCPrepend(theGraph->BicompRootLists,                   \
                                             gp_GetVertexPertinentRootsList(theGraph, v), \
                                             gp_GetDFSChildFromBicompRoot(theGraph, R)))

#define gp_AppendVertexPertinentRoot(theGraph, v, R)                                     \
    gp_SetVertexPertinentRootsList(theGraph, v,                                          \
                                   LCAppend(theGraph->BicompRootLists,                   \
                                            gp_GetVertexPertinentRootsList(theGraph, v), \
                                            gp_GetDFSChildFromBicompRoot(theGraph, R)))

#define gp_GetVertexFuturePertinentChild(theGraph, v) (theGraph->VI[v].futurePertinentChild)
#define gp_SetVertexFuturePertinentChild(theGraph, v, theFuturePertinentChild) (theGraph->VI[v].futurePertinentChild = theFuturePertinentChild)

// Used to advance futurePertinentChild of w to the next separated DFS child with a lowpoint less than v
// Once futurePertinentChild advances past a child, no future planarity operation could make that child
// relevant to future pertinence.
#define gp_UpdateVertexFuturePertinentChild(theGraph, w, v)                                             \
    while (gp_IsVertex(theGraph, theGraph->VI[w].futurePertinentChild))                                 \
    {                                                                                                   \
        /* Skip children that 1) aren't future pertinent, 2) have been merged into the bicomp with w */ \
        if (gp_GetVertexLowpoint(theGraph, theGraph->VI[w].futurePertinentChild) >= v ||                \
            gp_IsNotSeparatedDFSChild(theGraph, theGraph->VI[w].futurePertinentChild))                  \
        {                                                                                               \
            theGraph->VI[w].futurePertinentChild =                                                      \
                gp_GetVertexNextDFSChild(theGraph, w, gp_GetVertexFuturePertinentChild(theGraph, w));   \
        }                                                                                               \
        else                                                                                            \
            break;                                                                                      \
    }

#define gp_GetVertexSortedDFSChildList(theGraph, v) (theGraph->VI[v].sortedDFSChildList)
#define gp_SetVertexSortedDFSChildList(theGraph, v, theSortedDFSChildList) (theGraph->VI[v].sortedDFSChildList = theSortedDFSChildList)

#define gp_GetVertexNextDFSChild(theGraph, v, c) LCGetNext(theGraph->sortedDFSChildLists, gp_GetVertexSortedDFSChildList(theGraph, v), c)

#define gp_AppendDFSChild(theGraph, v, c) \
    LCAppend(theGraph->sortedDFSChildLists, gp_GetVertexSortedDFSChildList(theGraph, v), c)

#define gp_GetVertexFwdEdgeList(theGraph, v) (theGraph->VI[v].fwdEdgeList)
#define gp_SetVertexFwdEdgeList(theGraph, v, theFwdEdgeList) (theGraph->VI[v].fwdEdgeList = theFwdEdgeList)

#ifdef __cplusplus
}
#endif

#endif
