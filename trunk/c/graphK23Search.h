#ifndef GRAPH_K23SEARCH_H
#define GRAPH_K23SEARCH_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

int gp_AttachK23Search(graphP theGraph);
int gp_DetachK23Search(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif

