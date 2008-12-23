#ifndef GRAPH_H
#define GRAPH_H

/* Copyright (c) 1997-2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include "graphStructures.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public functions for graphs */

#include "graphExtensions.h"

graphP gp_New(void);

int    gp_InitGraph(graphP theGraph, int N);
void   gp_ReinitializeGraph(graphP theGraph);
int    gp_CopyGraph(graphP dstGraph, graphP srcGraph);
graphP gp_DupGraph(graphP theGraph);

int    gp_CreateRandomGraph(graphP theGraph);
int    gp_CreateRandomGraphEx(graphP theGraph, int numEdges);

void   gp_Free(graphP *pGraph);

int    gp_Read(graphP theGraph, char *FileName);
int    gp_Write(graphP theGraph, char *FileName, int Mode);

int    gp_IsNeighbor(graphP theGraph, int u, int v);
int    gp_GetNeighborEdgeRecord(graphP theGraph, int u, int v);
int    gp_GetVertexDegree(graphP theGraph, int v);

int    gp_AddEdge(graphP theGraph, int u, int ulink, int v, int vlink);
int    gp_AddInternalEdge(graphP theGraph, int u, int e_u0, int e_u1, 
                                           int v, int e_v0, int e_v1);

void   gp_HideEdge(graphP theGraph, int arcPos);
void   gp_RestoreEdge(graphP theGraph, int arcPos);
int    gp_DeleteEdge(graphP theGraph, int J, int nextLink);

int    gp_CreateDFSTree(graphP theGraph);
int    gp_SortVertices(graphP theGraph);
void   gp_LowpointAndLeastAncestor(graphP theGraph);

int    gp_Embed(graphP theGraph, int embedFlags);
int    gp_TestEmbedResultIntegrity(graphP theGraph, graphP origGraph, int embedResult);

/********************************************************************
 int  gp_GetTwinArc(graphP theGraph, int Arc);
 This macro function returns the calculated twin arc of a given arc.
 If the arc location is even, then the successor is the twin.
 If the arc node is odd, then the predecessor is the twin.

 Logically, we return (Arc & 1) ? Arc-1 : Arc+1

 The original, first definition appears to be slightly faster
 ********************************************************************/

#define gp_GetTwinArc(theGraph, Arc) ((Arc) & 1) ? Arc-1 : Arc+1
//#define gp_GetTwinArc(theGraph, Arc) ((Arc)+1-(((Arc)&1)<<1))

/* Possible Mode parameter of gp_Write */

#define WRITE_ADJLIST   1
#define WRITE_ADJMATRIX 2
#define WRITE_DEBUGINFO 3

/* Possible Flags for gp_Embed.  The planar and outerplanar settings are supported
   natively.  The rest require extension modules. */

#define EMBEDFLAGS_PLANAR       1
#define EMBEDFLAGS_OUTERPLANAR  2

#define EMBEDFLAGS_DRAWPLANAR   (4|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK23 (16|EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK4  (32|EMBEDFLAGS_OUTERPLANAR)
#define EMBEDFLAGS_SEARCHFORK33 (64|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_SEARCHFORK5  (128|EMBEDFLAGS_PLANAR)

#define EMBEDFLAGS_MAXIMALPLANARSUBGRAPH    256
#define EMBEDFLAGS_PROJECTIVEPLANAR         512
#define EMBEDFLAGS_TOROIDAL                 1024

/* Private operations shared by modules in this implementation */ 

/********************************************************************
 _VertexActiveStatus()
 Tells whether a vertex is externally active, internally active 
 or inactive.
 ********************************************************************/

#define _VertexActiveStatus(theGraph, theVertex, I) \
        (EXTERNALLYACTIVE(theGraph, theVertex, I) \
         ? VAS_EXTERNAL \
         : PERTINENT(theGraph, theVertex) \
           ? VAS_INTERNAL \
           : VAS_INACTIVE)

/********************************************************************
 _PERTINENT()
 Tells whether a vertex is still pertinent to the processing of edges
 from I to its descendants.  A vertex can become non-pertinent during
 step I as edges are embedded.
 ********************************************************************/

#define PERTINENT(theGraph, theVertex) \
        (theGraph->V[theVertex].adjacentTo != NIL || \
         theGraph->V[theVertex].pertinentBicompList != NIL ? 1 : 0)

/********************************************************************
 _EXTERNALLYACTIVE()
 Tells whether a vertex is still externally active in step I.
 A vertex can become inactive during step I as edges are embedded.

 For outerplanar graph embedding (and related extension algorithms),
 we return externally active for all vertices since they must all be 
 kept on the external face.
 ********************************************************************/

#define EXTERNALLYACTIVE(theGraph, theVertex, I) \
        (( theGraph->embedFlags & EMBEDFLAGS_OUTERPLANAR) || \
           theGraph->V[theVertex].leastAncestor < I \
         ? 1 \
         : theGraph->V[theVertex].separatedDFSChildList != NIL && \
           theGraph->V[theGraph->V[theVertex].separatedDFSChildList].Lowpoint < I \
           ? 1 \
           : 0)

#ifdef __cplusplus
}
#endif

#endif

