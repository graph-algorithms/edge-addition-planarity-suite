#ifndef GRAPH_EXTENSIONS_H
#define GRAPH_EXTENSIONS_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#ifdef __cplusplus
extern "C" {
#endif

#include "graphStructures.h"

int gp_AddExtension(graphP theGraph, 
                    char *module, 
                    void *context, 
                    void *(*dupContext)(void *, void *), 
                    void (*freeContext)(void *),
                    graphFunctionTableP overloadTable);

int gp_FindExtension(graphP theGraph, char *module, void **pContext);

int gp_RemoveExtension(graphP theGraph, char *module);

int gp_CopyExtensions(graphP dstGraph, graphP srcGraph);

void gp_FreeExtensions(graphP theGraph);

#ifdef __cplusplus
}
#endif

#endif
