/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef GRAPHDFSUTILS_H
#define GRAPHDFSUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "graph.h"

    int gp_CreateDFSTree(graphP theGraph);
    int gp_SortVertices(graphP theGraph);
    int gp_ComputeLowpoints(graphP theGraph);
    int gp_ComputeLeastAncestors(graphP theGraph);

/* Graph Flags: see gp_GetGraphFlags()
        GRAPHFLAGS_DFSNUMBERED is set if DFS numbering has been performed on the graph
        GRAPHFLAGS_SORTEDBYDFI records whether the graph is in original vertex order
                or sorted by depth first index. Successive calls to SortVertices()
                toggle this bit.
*/
#define GRAPHFLAGS_DFSNUMBERED 1
#define GRAPHFLAGS_SORTEDBYDFI 2

#ifdef __cplusplus
}
#endif

#endif /* GRAPHDFSUTILS_H */
