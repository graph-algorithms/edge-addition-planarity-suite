#ifndef GRAPHPLANARITY_H
#define GRAPHPLANARITY_H

/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../graphStructures.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define PLANARITY_NAME "Planarity"

    int gp_ExtendWith_Planarity(graphP theGraph);
    int gp_Detach_Planarity(graphP theGraph);

    // Graph embedding and result validation methods
    // The embedResult output by gp_Embed() and input to gp_TestEmbedResultIntegrity()
    // can be OK if the graph is embedded or embeddable, NONEMBEDDABLE if a minimal
    // subgraph obstructing embedding has been isolated, or NOTOK on error
    int gp_Embed(graphP theGraph, unsigned embedFlags);
    int gp_TestEmbedResultIntegrity(graphP theGraph, graphP origGraph, int embedResult);

// Possible graph embedFlags to pass to gp_Embed() and which are then set
// into the graph by gp_Embed().
#define gp_GetEmbedFlags(theGraph) ((theGraph)->embedFlags)

#define EMBEDFLAGS_PLANAR 1
#define EMBEDFLAGS_OUTERPLANAR 2

#define EMBEDFLAGS_DRAWPLANAR (4 | EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK23 (8 | EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK33 (16 | EMBEDFLAGS_PLANAR)
#define EMBEDFLAGS_SEARCHFORK4 (32 | EMBEDFLAGS_OUTERPLANAR)

// Reserve flag bits for possible future embedding-related extension modules
#define EMBEDFLAGS_SEARCHFORK5 (64 | EMBEDFLAGS_PLANAR)
#define EMBEDFLAGS_SEARCHFORK5MINOR (128 | EMBEDFLAGS_PLANAR)
#define EMBEDFLAGS_MAXIMALPLANARSUBGRAPH (256 | EMBEDFLAGS_PLANAR)
#define EMBEDFLAGS_PROJECTIVEPLANAR 512
#define EMBEDFLAGS_TOROIDAL 1024

#ifdef __cplusplus
}
#endif

#endif
