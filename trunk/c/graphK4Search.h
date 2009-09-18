#ifndef GRAPH_K4SEARCH_H
#define GRAPH_K4SEARCH_H

/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved.
No part of this file can be copied or used for any reason without
the expressed written permission of the copyright holder.
*/

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

int gp_AttachK4Search(graphP theGraph);
int gp_DetachK4Search(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif

