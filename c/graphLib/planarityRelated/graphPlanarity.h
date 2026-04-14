#ifndef GRAPH_PLANARITY_H
#define GRAPH_PLANARITY_H

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

#define PLANARITY_NAME "PlanarEmbed"

    int gp_ExtendWith_Planarity(graphP theGraph);
    int gp_Detach_Planarity(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif
