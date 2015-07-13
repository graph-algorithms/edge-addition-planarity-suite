#ifndef GRAPH_COLORVERTICES_PRIVATE_H
#define GRAPH_COLORVERTICES_PRIVATE_H

/*
Copyright (c) 1997-2015, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    // Helps distinguish initialize from re-initialize
    int initialized;

    // The graph that this context augments
    graphP theGraph;

    // Overloaded function pointers
    graphFunctionTable functions;

    // Data structures, unique to this extension, for managing lists of
    // vertices by degree (e.g. all vertices of degree K in list K), and
    // for storing each vertex color (e.g. vertex K has color[K])
    listCollectionP degLists;
    int *degListHeads;
    int *degree;
    int *color;
    int numVerticesToReduce, highestColorUsed;

    int *colorDetector;

} ColorVerticesContext;

#ifdef __cplusplus
}
#endif

#endif

