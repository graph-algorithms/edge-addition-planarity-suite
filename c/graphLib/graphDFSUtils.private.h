/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef GRAPHDFSUTILS_PRIVATE_H
#define GRAPHDFSUTILS_PRIVATE_H

#include "graph.private.h"
#include "graphDFSUtils.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /********************************************************************
     Vertex Info Structure Definition.

     This structure equips the non-virtual vertices with additional
     information needed for DFS-related and planarity-related algorithms.

        parent: The DFI of the DFS tree parent of this vertex
        leastAncestor: min(DFI of neighbors connected by backedge)
        lowpoint: min(leastAncestor, min(lowpoint of DFS Children))
    */

    struct DFSUtils_VertexInfo
    {
        int parent, leastAncestor, lowpoint;
    };

    typedef struct DFSUtils_VertexInfo DFSUtils_VertexInfo;
    typedef DFSUtils_VertexInfo *DFSUtils_VertexInfoP;

#define gp_GetVertexParent(theGraph, v) (theGraphDVI(theGraph)[v].parent)
#define gp_SetVertexParent(theGraph, v, theParent) (theGraphDVI(theGraph)[v].parent = theParent)

#define _gp_IsDFSTreeRoot(theGraph, v) gp_IsNotVertex(theGraph, gp_GetVertexParent(theGraph, v))
#define _gp_IsNotDFSTreeRoot(theGraph, v) gp_IsVertex(theGraph, gp_GetVertexParent(theGraph, v))
#define _gp_GetVertexFromBicompRoot(theGraph, R) gp_GetVertexParent(theGraph, gp_GetDFSChildFromBicompRoot(theGraph, R))

#define gp_GetVertexLeastAncestor(theGraph, v) (theGraphDVI(theGraph)[v].leastAncestor)
#define gp_SetVertexLeastAncestor(theGraph, v, theLeastAncestor) (theGraphDVI(theGraph)[v].leastAncestor = theLeastAncestor)

#define gp_GetVertexLowpoint(theGraph, v) (theGraphDVI(theGraph)[v].lowpoint)
#define gp_SetVertexLowpoint(theGraph, v, theLowpoint) (theGraphDVI(theGraph)[v].lowpoint = theLowpoint)

#ifdef __cplusplus
}
#endif

#endif /* GRAPHPDFSUTILS_PRIVATE_H */
