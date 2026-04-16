#ifndef GRAPHOUTERPLANARITY_H
#define GRAPHOUTERPLANARITY_H

/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../graph.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define OUTERPLANARITY_NAME "Outerplanarity"

    int gp_ExtendWith_Outerplanarity(graphP theGraph);
    int gp_Detach_Outerplanarity(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif
