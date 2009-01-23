#ifndef GRAPH_DRAWPLANAR_H
#define GRAPH_DRAWPLANAR_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRAWPLANAR_NAME "DrawPlanar"

int gp_AttachDrawPlanar(graphP theGraph);
int gp_DetachDrawPlanar(graphP theGraph);

int  gp_DrawPlanar_RenderToFile(graphP theEmbedding, char *theFileName);

#ifdef __cplusplus
}
#endif

#endif

