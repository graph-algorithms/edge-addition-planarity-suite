#ifndef GRAPH_EXTENSIONS_PRIVATE_H
#define GRAPH_EXTENSIONS_PRIVATE_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graphFunctionTable.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char *module;
    void *context;
    void *(*dupContext)(void *, void *);
    void (*freeContext)(void *);
    
    graphFunctionTableP functions;

    struct graphExtension *next;
} graphExtension;

typedef graphExtension * graphExtensionP;

#ifdef __cplusplus
}
#endif

#endif
