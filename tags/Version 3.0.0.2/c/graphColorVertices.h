#ifndef GRAPH_COLORVERTICES_H
#define GRAPH_COLORVERTICES_H

/*
Copyright (c) 1997-2015, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COLORVERTICES_NAME "ColorVertices"

int gp_AttachColorVertices(graphP theGraph);
int gp_DetachColorVertices(graphP theGraph);

int gp_ColorVertices(graphP theGraph);
int gp_GetNumColorsUsed(graphP theGraph);
int gp_ColorVerticesIntegrityCheck(graphP theGraph, graphP origGraph);

#ifdef __cplusplus
}
#endif

#endif
