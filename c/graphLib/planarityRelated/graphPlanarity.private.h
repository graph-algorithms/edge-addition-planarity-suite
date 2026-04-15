/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef GRAPHPLANARITY_PRIVATE_H
#define GRAPHPLANARITY_PRIVATE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "../graph.h"

// PLANARITY-RELATED ONLY VERTEX FLAGS
//
// The ANYVERTEX_OBSTRUCTIONMARK_MASK bits are bits 2-4, 4+8+16=28
// They are used by planarity-related algorithms to identify the four
// regions of the external face cycle of a bicomp, relative to an
// XY-path in the bicomp.
// Bit 2 - 4 if the OBSTRUCTIONMARK is set, 0 if not
// Bit 3 - 8 if the OBSTRUCTIONMARK indicates Y side, 0 if X side
// Bit 4 - 16 if the OBSTRUCTIONMARK indicates high, 0 if low
#define ANYVERTEX_OBSTRUCTIONMARK_MASK 28

// Call gp_GetObstructionMark, then compare to one of these four possibilities
// ANYVERTEX_OBSTRUCTIONMARK_HIGH_RXW - On the external face path between vertices R and X
// ANYVERTEX_OBSTRUCTIONMARK_LOW_RXW  - X or on the external face path between vertices X and W
// ANYVERTEX_OBSTRUCTIONMARK_HIGH_RYW - On the external face path between vertices R and Y
// ANYVERTEX_OBSTRUCTIONMARK_LOW_RYW  - Y or on the external face path between vertices Y and W
// ANYVERTEX_OBSTRUCTIONMARK_UNMARKED  - corresponds to all three bits off
#define ANYVERTEX_OBSTRUCTIONMARK_HIGH_RXW 20
#define ANYVERTEX_OBSTRUCTIONMARK_LOW_RXW 4
#define ANYVERTEX_OBSTRUCTIONMARK_HIGH_RYW 28
#define ANYVERTEX_OBSTRUCTIONMARK_LOW_RYW 12
#define ANYVERTEX_OBSTRUCTIONMARK_UNMARKED 0

#define gp_GetObstructionMark(theGraph, v) (theGraph->V[v].flags & ANYVERTEX_OBSTRUCTIONMARK_MASK)
#define gp_ClearObstructionMark(theGraph, v) (theGraph->V[v].flags &= ~ANYVERTEX_OBSTRUCTIONMARK_MASK)
#define gp_SetObstructionMark(theGraph, v, type) (theGraph->V[v].flags |= type)
#define gp_ResetObstructionMark(theGraph, v, type) \
    (theGraph->V[v].flags = (theGraph->V[v].flags & ~ANYVERTEX_OBSTRUCTIONMARK_MASK) | type)

    /********************************************************************
    // PLANARITY-RELATED ONLY
    //
     This structure defines a pair of links used by each vertex and virtual vertex
        to create "short circuit" paths that eliminate unimportant vertices from
        the external face, enabling more efficient traversal of the external face.

        It is also possible to embed the "short circuit" edges, but this approach
        creates a better separation of concerns, imparts greater clarity, and
        removes exceptionalities for handling additional fake "short circuit" edges.

        vertex[2]: The two adjacent vertices along the external face, possibly
                short-circuiting paths of inactive vertices.
    */

    struct extFaceLinkRec
    {
        int vertex[2];
    };

    typedef struct extFaceLinkRec extFaceLinkRec;
    typedef extFaceLinkRec *extFaceLinkRecP;

#define gp_GetExtFaceVertex(theGraph, v, link) (theGraph->extFace[v].vertex[link])
#define gp_SetExtFaceVertex(theGraph, v, link, theVertex) (theGraph->extFace[v].vertex[link] = theVertex)

    /********************************************************************
    // PLANARITY-RELATED ONLY
    //
     Variables needed in embedding by Kuratowski subgraph isolator:
            minorType: the type of planarity obstruction found.
            v: the current vertex being processed
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

    struct isolatorContext
    {
        int minorType;
        int v, r, x, y, w, px, py, z;
        int ux, dx, uy, dy, dw, uz, dz;
    };

    typedef struct isolatorContext isolatorContext;
    typedef isolatorContext *isolatorContextP;

//********************************************************************
// A few simple integer selection macros for obstruction isolation
//********************************************************************
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define MIN3(x, y, z) MIN(MIN((x), (y)), MIN((y), (z)))
#define MAX3(x, y, z) MAX(MAX((x), (y)), MAX((y), (z)))

#ifdef __cplusplus
}
#endif

#endif /* GRAPHPLANARITY_PRIVATE_H */
