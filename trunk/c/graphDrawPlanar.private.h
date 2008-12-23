#ifndef GRAPH_DRAWPLANAR_PRIVATE_H
#define GRAPH_DRAWPLANAR_PRIVATE_H

/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Additional equipment for each graph node (edge arc or vertex)
/*
        pos, start, end: used to store a visibility representation, or 
                horvert diagram of a planar graph.  
                For vertices, vertical position, horizontal range
                For edges, horizontal position, vertical range
*/
typedef struct
{
     int  pos, start, end;
} DrawPlanar_GraphNode;

typedef DrawPlanar_GraphNode * DrawPlanar_GraphNodeP;

// Additional equipment for each vertex
/*
        drawingFlag, ancestor, ancestorChild: used to collect information needed 
                to help 'draw' a visibility representation.  During planar 
                embedding, a vertex is determined to be between its DFS parent and 
                a given ancestor (the vertex being processed) or beyond the parent 
                relative to the ancestor. In post processing, the relative 
                orientation of the parent and ancestor are determined, 
                then the notion of between/beyond resolves to above/below or 
                below/above depending on whether the ancestor is above or below, 
                respectively, the parent.  The ancestorChild are used to help r
                esolve this latter question.
        tie[2]  stores information along the external face during embedding
                that is pertinent to helping break ties in the decisions about
                vertical vertex positioning.  When vertices are first merged
                together into a bicomp, we cannot always decide right away which
                vertices will be above or below others.  But as we traverse the
                external face removing inactive vertices, these positional ties
                can be resolved.
*/
typedef struct
{
        int drawingFlag, ancestor, ancestorChild;
        int tie[2];
} DrawPlanar_VertexRec;

typedef DrawPlanar_VertexRec * DrawPlanar_VertexRecP;

#define DRAWINGFLAG_BEYOND     0
#define DRAWINGFLAG_TIE        1
#define DRAWINGFLAG_BETWEEN    2
#define DRAWINGFLAG_BELOW      3
#define DRAWINGFLAG_ABOVE      4

typedef struct
{
    // Helps distinguish initialize from re-initialize
    int initialized;

    // The graph that this context augments
    graphP theGraph;

    // Parallel array for additional graph node level equipment
    DrawPlanar_GraphNodeP G;

    // Parallel array for additional vertex level equipment
    DrawPlanar_VertexRecP V;    

    // Overloaded function pointers
    graphFunctionTable functions;

} DrawPlanarContext;

#ifdef __cplusplus
}
#endif

#endif

