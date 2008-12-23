/* Copyright (c) 2008 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#include <stdlib.h>

#include "graphK23Search.private.h"
#include "graphK23Search.h"

extern int  _SearchForK23(graphP theGraph, int I);

extern int  _TestForK23GraphObstruction(graphP theGraph, int *degrees, int *imageVerts);
extern int  _getImageVertices(graphP theGraph, int *degrees, int maxDegree,
                              int *imageVerts, int maxNumImageVerts);
extern int  _TestSubgraph(graphP theSubgraph, graphP theGraph);

/* Forward declarations of overloading functions */

int  _K23Search_EmbedIterationPostprocess(graphP theGraph, int I);
int  _K23Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult);
int  _K23Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph);
int  _K23Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph);

/* Forward declarations of functions used by the extension system */

void *_K23Search_DupContext(void *pContext, void *theGraph);
void _K23Search_FreeContext(void *);

/****************************************************************************
 gp_AttachK23Search()

 This function adjusts the graph data structure to attach the K2,3 search
 feature.
 ****************************************************************************/

int  gp_AttachK23Search(graphP theGraph)
{
     K23SearchContext *context = NULL;

     // If the K2,3 search feature has already been attached to the graph
     // then there is no need to attach it again
     gp_FindExtension(theGraph, "K23Search", (void *)&context);
     if (context != NULL)
     {
         return OK;
     }

     // Allocate a new extension context
     context = (K23SearchContext *) malloc(sizeof(K23SearchContext));
     if (context == NULL)
     {
         return NOTOK;
     }

     // Put the overload functions into the context function table.
     // gp_AddExtension will overload the graph's functions with these, and
     // return the base function pointers in the context function table
     memset(&context->functions, 0, sizeof(graphFunctionTable));

     context->functions.fpEmbedIterationPostprocess = _K23Search_EmbedIterationPostprocess;
     context->functions.fpEmbedPostprocess = _K23Search_EmbedPostprocess;
     context->functions.fpCheckEmbeddingIntegrity = _K23Search_CheckEmbeddingIntegrity;
     context->functions.fpCheckObstructionIntegrity = _K23Search_CheckObstructionIntegrity;

     // Store the K23 search context, including the data structure and the
     // function pointers, as an extension of the graph
     if (gp_AddExtension(theGraph, "K23Search", (void *) context,
                         _K23Search_DupContext, _K23Search_FreeContext,
                         &context->functions) != OK)
     {
         _K23Search_FreeContext(context);
         return NOTOK;
     }

     return OK;
}

/********************************************************************
 gp_DetachK23Search()
 ********************************************************************/

int gp_DetachK23Search(graphP theGraph)
{
    return gp_RemoveExtension(theGraph, "K23Search");
}

/********************************************************************
 _K23Search_DupContext()
 ********************************************************************/

void *_K23Search_DupContext(void *pContext, void *theGraph)
{
     K23SearchContext *context = (K23SearchContext *) pContext;
     K23SearchContext *newContext = (K23SearchContext *) malloc(sizeof(K23SearchContext));

     if (newContext != NULL)
     {
         *newContext = *context;
     }

     return newContext;
}

/********************************************************************
 _K23Search_FreeContext()
 ********************************************************************/

void _K23Search_FreeContext(void *pContext)
{
     free(pContext);
}

/********************************************************************
 ********************************************************************/

int  _K23Search_EmbedIterationPostprocess(graphP theGraph, int I)
{
    if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK23)
        return _SearchForK23(theGraph, I);

    else
    {
        K23SearchContext *context = NULL;
        gp_FindExtension(theGraph, "K23Search", (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpEmbedIterationPostprocess(theGraph, I);
        }
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K23Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult)
{
     // For K2,3 search, we just return the edge embedding result because the
     // search result has been obtained already.
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK23)
     {
         return edgeEmbeddingResult;
     }

     // When not searching for K2,3, we let the superclass do the work
     else
     {
        K23SearchContext *context = NULL;
        gp_FindExtension(theGraph, "K23Search", (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpEmbedPostprocess(theGraph, I, edgeEmbeddingResult);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K23Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph)
{
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK23)
     {
         return OK;
     }

     // When not searching for K2,3, we let the superclass do the work
     else
     {
        K23SearchContext *context = NULL;
        gp_FindExtension(theGraph, "K23Search", (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckEmbeddingIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K23Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph)
{
     // When searching for K2,3, we ensure that theGraph is a subgraph of
     // the original graph and that it contains a K2,3 homeomorph
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK23)
     {
         int  degrees[4], imageVerts[5];

         if (_TestSubgraph(theGraph, origGraph) != OK)
             return NOTOK;

         if (_getImageVertices(theGraph, degrees, 4, imageVerts, 5) != OK)
             return NOTOK;

         if (_TestForK23GraphObstruction(theGraph, degrees, imageVerts) == OK)
         {
             return OK;
         }

         return NOTOK;
     }

     // When not searching for K2,3, we let the superclass do the work
     else
     {
        K23SearchContext *context = NULL;
        gp_FindExtension(theGraph, "K23Search", (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckObstructionIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}
