/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/
#include <stdlib.h>
#include <stdio.h>
#include "graph.h"
#include "graphEdgeDetector.h"
unsigned ged_Hash(graphEdgeDetectorP theDetector, int v, int w){
    unsigned uv = (unsigned) v;
    unsigned uw = (unsigned) w;
    unsigned FNV_PRIME = 16777619u;
    unsigned FNV_OFFSET_BASIS = 2166136261u;
    unsigned hash = FNV_OFFSET_BASIS;
    unsigned totalBits;
    if (uv > uw)
    {
        unsigned temp = uv;
        uv = uw;
        uw = temp;
    }
    hash ^= uv;
    hash *= FNV_PRIME;
    
    hash ^= uw;
    hash *= FNV_PRIME;
    totalBits = (unsigned)(theDetector->edgeDetectorCapacity) << 5;
    hash = hash % totalBits;
    return hash;
}
int ged_Set(graphEdgeDetectorP theDetector, int v, int w){
    unsigned H;
    unsigned arrayidx;
    unsigned bitmask;
    if (theDetector == NULL || theDetector->edgeDetector == NULL)
    {
        return NOTOK;
    }
    H = ged_Hash(theDetector, v, w);
    arrayidx = H >> 5;
    bitmask = 1u << (H & 31);
    theDetector->edgeDetector[arrayidx] |= bitmask;
    return OK;
}
int ged_IsSet(graphEdgeDetectorP theDetector, int v, int w){
    unsigned H;
    unsigned arrayidx;
    unsigned bitmask;
    if (theDetector == NULL || theDetector->edgeDetector == NULL)
    {
        return FALSE;
    }
    H = ged_Hash(theDetector, v, w);
    arrayidx = H >> 5;
    bitmask = 1u << (H & 31);
    if (((theDetector->edgeDetector[arrayidx]) &(bitmask)) != 0){
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}
graphEdgeDetectorP ged_New(int theCapacity){
    if (theCapacity <=0)
    {
        return NULL;
    }
    graphEdgeDetectorP theDetector = NULL;
    theDetector = (graphEdgeDetectorP) malloc(sizeof(graphEdgeDetectorStruct));
    if (theDetector == NULL)
    {
        return NULL;
    }
    theDetector->edgeDetector = (unsigned *) calloc(theCapacity, sizeof(unsigned));
    if (theDetector->edgeDetector == NULL)
    {
        free(theDetector);
        return NULL;
    }
    theDetector->edgeDetectorCapacity = theCapacity;
    return theDetector;
}
graphEdgeDetectorP ged_Duplicate(graphEdgeDetectorP srcDetector){
    if (srcDetector == NULL)
    {
        return NULL;
    }
    graphEdgeDetectorP newDetector = NULL;
    newDetector = ged_New(srcDetector->edgeDetectorCapacity);
    if (newDetector == NULL)
    {
        return NULL;
    }
    for(int i=0;i<srcDetector->edgeDetectorCapacity;i++)
    {
        newDetector->edgeDetector[i] = srcDetector->edgeDetector[i];
    }
    return newDetector;   
}
void ged_Free(graphEdgeDetectorP * pDetector){
    if (pDetector == NULL || *pDetector == NULL)
    {
        return;
    }
    if ((*pDetector)->edgeDetector != NULL)
    {
        free((*pDetector)->edgeDetector);
        (*pDetector)->edgeDetector = NULL;
    }
    free(*pDetector);
    *pDetector = NULL;
}
