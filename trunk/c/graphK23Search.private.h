#ifndef GRAPH_K23SEARCH_PRIVATE_H
#define GRAPH_K23SEARCH_PRIVATE_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    // Overloaded function pointers
    graphFunctionTable functions;

} K23SearchContext;

#ifdef __cplusplus
}
#endif

#endif

