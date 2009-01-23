/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include <stdlib.h>

#include "graphDrawPlanar.private.h"
#include "graphDrawPlanar.h"

extern void _CollectDrawingData(DrawPlanarContext *context, int RootVertex, int W, int WPrevLink);
extern int  _BreakTie(DrawPlanarContext *context, int BicompRoot, int W, int WPrevLink);

extern int  _ComputeVisibilityRepresentation(DrawPlanarContext *context);
extern int  _CheckVisibilityRepresentationIntegrity(DrawPlanarContext *context);

/* Forward declarations of local functions */

void _DrawPlanar_ClearStructures(DrawPlanarContext *context);
int  _DrawPlanar_CreateStructures(DrawPlanarContext *context);
int  _DrawPlanar_InitStructures(DrawPlanarContext *context);

/* Forward declarations of overloading functions */

int  _DrawPlanar_MergeBicomps(graphP theGraph, int I, int RootVertex, int W, int WPrevLink);
int  _DrawPlanar_HandleInactiveVertex(graphP theGraph, int BicompRoot, int *pW, int *pWPrevLink);
int  _DrawPlanar_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult);
int  _DrawPlanar_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph);
int  _DrawPlanar_CheckObstructionIntegrity(graphP theGraph, graphP origGraph);

void _DrawPlanar_InitGraphNode(graphP theGraph, int I);
void _DrawPlanar_InitVertexRec(graphP theGraph, int I);
void _InitDrawGraphNode(DrawPlanarContext *context, int I);
void _InitDrawVertexRec(DrawPlanarContext *context, int I);

int  _DrawPlanar_InitGraph(graphP theGraph, int N);
void _DrawPlanar_ReinitializeGraph(graphP theGraph);
int  _DrawPlanar_SortVertices(graphP theGraph);

int  _DrawPlanar_ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize);
int  _DrawPlanar_WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize);

/* Forward declarations of functions used by the extension system */

void *_DrawPlanar_DupContext(void *pContext, void *theGraph);
void _DrawPlanar_FreeContext(void *);

/****************************************************************************
 gp_AttachDrawPlanar()

 This function adjusts the graph data structure to attach the planar graph
 drawing feature.

 To activate this feature during gp_Embed(), use EMBEDFLAGS_DRAWPLANAR.

 This method may be called immediately after gp_New() in the case of
 invoking gp_Read().  For generating graphs, gp_InitGraph() can be invoked
 before or after this enabling method.  This method detects if the core
 graph has already been initialized, and if so, it will initialize the
 additional data structures specific to planar graph drawing.  This makes
 it possible to invoke gp_New() and gp_InitGraph() together, and then attach
 this feature only if it is requested at run-time.

 Returns OK for success, NOTOK for failure.
 ****************************************************************************/

int  gp_AttachDrawPlanar(graphP theGraph)
{
     DrawPlanarContext *context = NULL;

     // If the drawing feature has already been attached to the graph,
     // then there is no need to attach it again
     gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);
     if (context != NULL)
     {
         return OK;
     }

     // Allocate a new extension context
     context = (DrawPlanarContext *) malloc(sizeof(DrawPlanarContext));
     if (context == NULL)
     {
         return NOTOK;
     }

     // First, tell the context that it is not initialized
     context->initialized = 0;

     // Save a pointer to theGraph in the context
     context->theGraph = theGraph;

     // Put the overload functions into the context function table.
     // gp_AddExtension will overload the graph's functions with these, and
     // return the base function pointers in the context function table
     memset(&context->functions, 0, sizeof(graphFunctionTable));

     context->functions.fpMergeBicomps = _DrawPlanar_MergeBicomps;
     context->functions.fpHandleInactiveVertex = _DrawPlanar_HandleInactiveVertex;
     context->functions.fpEmbedPostprocess = _DrawPlanar_EmbedPostprocess;
     context->functions.fpCheckEmbeddingIntegrity = _DrawPlanar_CheckEmbeddingIntegrity;
     context->functions.fpCheckObstructionIntegrity = _DrawPlanar_CheckObstructionIntegrity;

     context->functions.fpInitGraphNode = _DrawPlanar_InitGraphNode;
     context->functions.fpInitVertexRec = _DrawPlanar_InitVertexRec;

     context->functions.fpInitGraph = _DrawPlanar_InitGraph;
     context->functions.fpReinitializeGraph = _DrawPlanar_ReinitializeGraph;
     context->functions.fpSortVertices = _DrawPlanar_SortVertices;

     context->functions.fpReadPostprocess = _DrawPlanar_ReadPostprocess;
     context->functions.fpWritePostprocess = _DrawPlanar_WritePostprocess;

     _DrawPlanar_ClearStructures(context);

     // Store the Draw context, including the data structure and the
     // function pointers, as an extension of the graph
     if (gp_AddExtension(theGraph, DRAWPLANAR_NAME, (void *) context,
                         _DrawPlanar_DupContext, _DrawPlanar_FreeContext,
                         &context->functions) != OK)
     {
         _DrawPlanar_FreeContext(context);
         return NOTOK;
     }

     // Create the Draw-specific structures if the size of the graph is known
     // Attach functions are typically invoked after gp_New(), but if a graph
     // extension must be attached before gp_Read(), then the attachment
     // also happens before gp_InitGraph() because gp_Read() invokes init only
     // after it reads the order N of the graph.  Hence, this attach call would
     // occur when N==0 in the case of gp_Read().
     // But if a feature is attached after gp_InitGraph(), then N > 0 and so we
     // need to create and initialize all the custom data structures
     if (theGraph->N > 0)
     {
         if (_DrawPlanar_CreateStructures(context) != OK ||
             _DrawPlanar_InitStructures(context) != OK)
         {
             _DrawPlanar_FreeContext(context);
             return NOTOK;
         }
     }

     return OK;
}

/********************************************************************
 gp_DetachDrawPlanar()
 ********************************************************************/

int gp_DetachDrawPlanar(graphP theGraph)
{
    return gp_RemoveExtension(theGraph, DRAWPLANAR_NAME);
}

/********************************************************************
 _DrawPlanar_ClearStructures()
 ********************************************************************/

void _DrawPlanar_ClearStructures(DrawPlanarContext *context)
{
    if (!context->initialized)
    {
        // Before initialization, the pointers are stray, not NULL
        // Once NULL or allocated, free() or LCFree() can do the job
        context->G = NULL;
        context->V = NULL;

        context->initialized = 1;
    }
    else
    {
        if (context->G != NULL)
        {
            free(context->G);
            context->G = NULL;
        }
        if (context->V != NULL)
        {
            free(context->V);
            context->V = NULL;
        }
    }
}

/********************************************************************
 _DrawPlanar_CreateStructures()
 Create uninitialized structures for the vertex and graph node
 levels, and initialized structures for the graph level
 ********************************************************************/
int  _DrawPlanar_CreateStructures(DrawPlanarContext *context)
{
     int N = context->theGraph->N;
     int Gsize = context->theGraph->edgeOffset + 2*EDGE_LIMIT*N;

     if (N <= 0)
         return NOTOK;

     if ((context->G = (DrawPlanar_GraphNodeP) malloc(Gsize*sizeof(DrawPlanar_GraphNode))) == NULL ||
         (context->V = (DrawPlanar_VertexRecP) malloc(N*sizeof(DrawPlanar_VertexRec))) == NULL
        )
     {
         return NOTOK;
     }

     return OK;
}

/********************************************************************
 _DrawPlanar_InitStructures()
 Intended to be called when N>0.
 Creates any custom data structures for the graph, vertex and graph node
 levels, and initializes the custom structures only.
 ********************************************************************/
int  _DrawPlanar_InitStructures(DrawPlanarContext *context)
{
     int I, N = context->theGraph->N;
     int Gsize = context->theGraph->edgeOffset + 2*EDGE_LIMIT*N;

     if (N <= 0)
         return NOTOK;

     for (I = 0; I < Gsize; I++)
          _InitDrawGraphNode(context, I);

     for (I = 0; I < N; I++)
          _InitDrawVertexRec(context, I);

     return OK;
}

/********************************************************************
 _DrawPlanar_DupContext()
 ********************************************************************/

void *_DrawPlanar_DupContext(void *pContext, void *theGraph)
{
     DrawPlanarContext *context = (DrawPlanarContext *) pContext;
     DrawPlanarContext *newContext = (DrawPlanarContext *) malloc(sizeof(DrawPlanarContext));

     if (newContext != NULL)
     {
         int N = ((graphP) theGraph)->N;
         int Gsize = ((graphP) theGraph)->edgeOffset + 2*EDGE_LIMIT*N;

         *newContext = *context;

         newContext->theGraph = (graphP) theGraph;

         newContext->initialized = 0;
         _DrawPlanar_ClearStructures(newContext);
         if (N > 0)
         {
             if (_DrawPlanar_CreateStructures(newContext) != OK)
             {
                 _DrawPlanar_FreeContext(newContext);
                 return NULL;
             }

             // Initialize custom GraphNode and VertexRec data by copying
             memcpy(newContext->G, context->G, Gsize*sizeof(DrawPlanar_GraphNode));
             memcpy(newContext->V, context->V, N*sizeof(DrawPlanar_VertexRec));
         }
     }

     return newContext;
}

/********************************************************************
 _DrawPlanar_FreeContext()
 ********************************************************************/

void _DrawPlanar_FreeContext(void *pContext)
{
     DrawPlanarContext *context = (DrawPlanarContext *) pContext;

     _DrawPlanar_ClearStructures(context);
     free(pContext);
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_InitGraph(graphP theGraph, int N)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context == NULL)
    {
        return NOTOK;
    }
    else
    {
        theGraph->N = N;
        theGraph->edgeOffset = 2*N;

        if (_DrawPlanar_CreateStructures(context) != OK)
        {
            return NOTOK;
        }

        // This call initializes the base graph structures, but it also
        // initializes the custom graphnode and vertex level structures
        // due to the overloads of InitGraphNode and InitVertexRec
        context->functions.fpInitGraph(theGraph, N);
    }

    return OK;
}

/********************************************************************
 ********************************************************************/

void _DrawPlanar_ReinitializeGraph(graphP theGraph)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        // Reinitialization can go much faster if the underlying
        // init graph node and vertex rec functions are called,
        // rather than the overloads of this module, because it
        // avoids lots of unnecessary gp_FindExtension() calls.
        if (theGraph->functions.fpInitGraphNode == _DrawPlanar_InitGraphNode &&
            theGraph->functions.fpInitVertexRec == _DrawPlanar_InitVertexRec)
        {
            // Restore the graph function pointers
            theGraph->functions.fpInitGraphNode = context->functions.fpInitGraphNode;
            theGraph->functions.fpInitVertexRec = context->functions.fpInitVertexRec;

            // Reinitialize the graph
            context->functions.fpReinitializeGraph(theGraph);

            // Restore the function pointers that attach this feature
            theGraph->functions.fpInitGraphNode = _DrawPlanar_InitGraphNode;
            theGraph->functions.fpInitVertexRec = _DrawPlanar_InitVertexRec;

            // Do the reinitialization that is specific to this module
            _DrawPlanar_InitStructures(context);
        }

        // If optimization is not possible, then just stick with what works.
        // Reinitialize the graph-level structure (of which there are none)
        // and then invoke the reinitialize function.
        else
        {
            // The underlying function fpReinitializeGraph() already does this
            // due to the overloads of fpInitGraphNode() and fpInitVertexRec()
            // _DrawPlanar_InitStructures(context);
            context->functions.fpReinitializeGraph(theGraph);
        }
    }
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_SortVertices(graphP theGraph)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        if (theGraph->embedFlags == EMBEDFLAGS_DRAWPLANAR)
        {
            int I;
            DrawPlanar_GraphNodeP newG = NULL;
            DrawPlanar_VertexRecP newV = NULL;

            // Relabel the context data members that indicate vertices
            for (I=0; I < theGraph->N; I++)
            {
                context->V[I].ancestor = theGraph->G[context->V[I].ancestor].v;
                context->V[I].ancestorChild = theGraph->G[context->V[I].ancestorChild].v;
            }

            // Now we have to sort the first N positions of context G and V arrays
            // For simplicity we do this out-of-place with extra arrays
            if ((newG = (DrawPlanar_GraphNodeP) malloc(theGraph->N * sizeof(DrawPlanar_GraphNode))) == NULL)
            {
                return NOTOK;
            }

            if ((newV = (DrawPlanar_VertexRecP) malloc(theGraph->N * sizeof(DrawPlanar_VertexRec))) == NULL)
            {
                free(newG);
                return NOTOK;
            }

            // Let X==G[I].v be the location where the I^{th} record goes
            // Given newG and newV arrays, we want to move context G[I] to newG[X]
            // Then copy newG into G and newV into V
            for (I=0; I < theGraph->N; I++)
            {
                newG[theGraph->G[I].v] = context->G[I];
                newV[theGraph->G[I].v] = context->V[I];
            }

            memcpy(context->G, newG, theGraph->N * sizeof(DrawPlanar_GraphNode));
            memcpy(context->V, newV, theGraph->N * sizeof(DrawPlanar_VertexRec));

            free(newG);
            free(newV);
        }

        if (context->functions.fpSortVertices(theGraph) != OK)
            return NOTOK;

        return OK;
    }

    return NOTOK;
}

/********************************************************************
  Returns OK for a successful merge, NOTOK on an internal failure,
          or NONEMBEDDABLE if the merge is blocked
 ********************************************************************/

int  _DrawPlanar_MergeBicomps(graphP theGraph, int I, int RootVertex, int W, int WPrevLink)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        if (theGraph->embedFlags == EMBEDFLAGS_DRAWPLANAR)
        {
            _CollectDrawingData(context, RootVertex, W, WPrevLink);
        }

        return context->functions.fpMergeBicomps(theGraph, I, RootVertex, W, WPrevLink);
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int _DrawPlanar_HandleInactiveVertex(graphP theGraph, int BicompRoot, int *pW, int *pWPrevLink)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        int RetVal = context->functions.fpHandleInactiveVertex(theGraph, BicompRoot, pW, pWPrevLink);

        if (theGraph->embedFlags == EMBEDFLAGS_DRAWPLANAR)
        {
            if (_BreakTie(context, BicompRoot, *pW, *pWPrevLink) != OK)
                return NOTOK;
        }

        return RetVal;
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

void _DrawPlanar_InitGraphNode(graphP theGraph, int I)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        context->functions.fpInitGraphNode(theGraph, I);
        _InitDrawGraphNode(context, I);
    }
}

/********************************************************************
 ********************************************************************/

void _InitDrawGraphNode(DrawPlanarContext *context, int I)
{
    context->G[I].pos = 0;
    context->G[I].start = 0;
    context->G[I].end = 0;
}

/********************************************************************
 ********************************************************************/

void _DrawPlanar_InitVertexRec(graphP theGraph, int I)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        context->functions.fpInitVertexRec(theGraph, I);
        _InitDrawVertexRec(context, I);
    }
}

/********************************************************************
 ********************************************************************/

void _InitDrawVertexRec(DrawPlanarContext *context, int I)
{
    context->V[I].drawingFlag = DRAWINGFLAG_BEYOND;
    context->V[I].ancestorChild = 0;
    context->V[I].ancestor = 0;
    context->V[I].tie[0] = context->V[I].tie[1] = NIL;
}

/********************************************************************
 ********************************************************************/

int _DrawPlanar_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        int RetVal = context->functions.fpEmbedPostprocess(theGraph, I, edgeEmbeddingResult);

        if (theGraph->embedFlags == EMBEDFLAGS_DRAWPLANAR)
        {
            if (RetVal == OK)
            {
                RetVal = _ComputeVisibilityRepresentation(context);
            }
        }

        return RetVal;
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpCheckEmbeddingIntegrity(theGraph, origGraph) != OK)
            return NOTOK;

        return _CheckVisibilityRepresentationIntegrity(context);
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_CheckObstructionIntegrity(graphP theGraph, graphP origGraph)
{
     return OK;
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpReadPostprocess(theGraph, extraData, extraDataSize) != OK)
            return NOTOK;

        else if (extraData != NULL && extraDataSize > 0)
        {
            int I, tempInt;
            char line[64], tempChar;

            sprintf(line, "<%s>", DRAWPLANAR_NAME);

            // Find the start of the data for this feature
            extraData = strstr(extraData, line);
            if (extraData == NULL)
                return NOTOK;

            // Advance past the start tag
            extraData = (void *) ((char *) extraData + strlen(line)+1);

            // Read the N lines of vertex information
            for (I = 0; I < theGraph->N; I++)
            {
                sscanf(extraData, " %d%c %d %d %d", &tempInt, &tempChar,
                              &context->G[I].pos,
                              &context->G[I].start,
                              &context->G[I].end);

                extraData = strchr(extraData, '\n') + 1;
            }

            // Read the lines that contain edge information
            for (I = theGraph->edgeOffset; I < theGraph->edgeOffset+2*theGraph->M; I++)
            {
                sscanf(extraData, " %d%c %d %d %d", &tempInt, &tempChar,
                              &context->G[I].pos,
                              &context->G[I].start,
                              &context->G[I].end);

                extraData = strchr(extraData, '\n') + 1;
            }
        }

        return OK;
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _DrawPlanar_WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theGraph, DRAWPLANAR_NAME, (void *)&context);

    if (context != NULL)
    {
        if (context->functions.fpWritePostprocess(theGraph, pExtraData, pExtraDataSize) != OK)
            return NOTOK;
        else
        {
            char line[64];
            int maxLineSize = 64, extraDataPos = 0, I;
            int GSize = theGraph->edgeOffset + 2*EDGE_LIMIT*theGraph->N;
            char *extraData = (char *) malloc((GSize + 2) * maxLineSize * sizeof(char));

            if (extraData == NULL)
                return NOTOK;

            // Bit of an unlikely case, but for safety, a bigger maxLineSize
            // and line array size are needed to handle very large graphs
            if (theGraph->N > 2000000000)
            {
                free(extraData);
                return NOTOK;
            }

            sprintf(line, "<%s>\n", DRAWPLANAR_NAME);
            strcpy(extraData+extraDataPos, line);
            extraDataPos += (int) strlen(line);

            for (I = 0; I < theGraph->N; I++)
            {
                sprintf(line, "%d: %d %d %d\n", I,
                              context->G[I].pos,
                              context->G[I].start,
                              context->G[I].end);
                strcpy(extraData+extraDataPos, line);
                extraDataPos += (int) strlen(line);
            }

            for (I = theGraph->edgeOffset; I < theGraph->edgeOffset+2*theGraph->M; I++)
            {
                sprintf(line, "%d: %d %d %d\n", I,
                              context->G[I].pos,
                              context->G[I].start,
                              context->G[I].end);
                strcpy(extraData+extraDataPos, line);
                extraDataPos += (int) strlen(line);
            }

            sprintf(line, "</%s>\n", DRAWPLANAR_NAME);
            strcpy(extraData+extraDataPos, line);
            extraDataPos += (int) strlen(line);

            *pExtraData = (void *) extraData;
            *pExtraDataSize = extraDataPos * sizeof(char);
        }

        return OK;
    }

    return NOTOK;
}
