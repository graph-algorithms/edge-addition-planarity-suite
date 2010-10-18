/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2010, John M. Boyer
All rights reserved. Includes a reference implementation of the following:

* John M. Boyer. "Simplified O(n) Algorithms for Planar Graph Embedding,
  Kuratowski Subgraph Isolation, and Related Problems". Ph.D. Dissertation,
  University of Victoria, 2001.

* John M. Boyer and Wendy J. Myrvold. "On the Cutting Edge: Simplified O(n)
  Planarity by Edge Addition". Journal of Graph Algorithms and Applications,
  Vol. 8, No. 3, pp. 241-273, 2004.

* John M. Boyer. "A New Method for Efficiently Generating Planar Graph
  Visibility Representations". In P. Eades and P. Healy, editors,
  Proceedings of the 13th International Conference on Graph Drawing 2005,
  Lecture Notes Comput. Sci., Volume 3843, pp. 508-511, Springer-Verlag, 2006.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

* Neither the name of the Planarity-Related Graph Algorithms Project nor the names
  of its contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>

#include "graphK4Search.private.h"
#include "graphK4Search.h"

extern int  _GetNextVertexOnExternalFace(graphP theGraph, int curVertex, int *pPrevLink);

extern int  _SearchForK4InBicomps(graphP theGraph, int I);
extern int  _SearchForK4InBicomp(graphP theGraph, K4SearchContext *context, int I, int R);

extern int _TestForCompleteGraphObstruction(graphP theGraph, int numVerts,
                                            int *degrees, int *imageVerts);

extern int  _getImageVertices(graphP theGraph, int *degrees, int maxDegree,
                              int *imageVerts, int maxNumImageVerts);

extern int  _TestSubgraph(graphP theSubgraph, graphP theGraph);

/* Forward declarations of local functions */

void _K4Search_ClearStructures(K4SearchContext *context);
int  _K4Search_CreateStructures(K4SearchContext *context);
int  _K4Search_InitStructures(K4SearchContext *context);

/* Forward declarations of overloading functions */
int  _K4Search_HandleBlockedEmbedIteration(graphP theGraph, int I);
int  _K4Search_HandleBlockedDescendantBicomp(graphP theGraph, int I, int RootVertex, int R, int *pRout, int *pW, int *pWPrevLink);
int  _K4Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult);
int  _K4Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph);
int  _K4Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph);

void _K4Search_InitEdgeRec(graphP theGraph, int J);
void _InitK4SearchEdgeRec(K4SearchContext *context, int J);

int  _K4Search_InitGraph(graphP theGraph, int N);
void _K4Search_ReinitializeGraph(graphP theGraph);
int  _K4Search_EnsureArcCapacity(graphP theGraph, int requiredArcCapacity);

/* Forward declarations of functions used by the extension system */

void *_K4Search_DupContext(void *pContext, void *theGraph);
void _K4Search_FreeContext(void *);

/****************************************************************************
 * K4SEARCH_ID - the variable used to hold the integer identifier for this
 * extension, enabling this feature's extension context to be distinguished
 * from other features' extension contexts that may be attached to a graph.
 ****************************************************************************/

int K4SEARCH_ID = 0;

/****************************************************************************
 gp_AttachK4Search()

 This function adjusts the graph data structure to attach the K4 search
 feature.
 ****************************************************************************/

int  gp_AttachK4Search(graphP theGraph)
{
     K4SearchContext *context = NULL;

     // If the K4 search feature has already been attached to the graph,
     // then there is no need to attach it again
     gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);
     if (context != NULL)
     {
         return OK;
     }

     // Allocate a new extension context
     context = (K4SearchContext *) malloc(sizeof(K4SearchContext));
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
     context->functions.fpHandleBlockedEmbedIteration = _K4Search_HandleBlockedEmbedIteration;
     context->functions.fpHandleBlockedDescendantBicomp = _K4Search_HandleBlockedDescendantBicomp;
     context->functions.fpEmbedPostprocess = _K4Search_EmbedPostprocess;
     context->functions.fpCheckEmbeddingIntegrity = _K4Search_CheckEmbeddingIntegrity;
     context->functions.fpCheckObstructionIntegrity = _K4Search_CheckObstructionIntegrity;

     context->functions.fpInitEdgeRec = _K4Search_InitEdgeRec;

     context->functions.fpInitGraph = _K4Search_InitGraph;
     context->functions.fpReinitializeGraph = _K4Search_ReinitializeGraph;
     context->functions.fpEnsureArcCapacity = _K4Search_EnsureArcCapacity;

     _K4Search_ClearStructures(context);

     // Store the K33 search context, including the data structure and the
     // function pointers, as an extension of the graph
     if (gp_AddExtension(theGraph, &K4SEARCH_ID, (void *) context,
                         _K4Search_DupContext, _K4Search_FreeContext,
                         &context->functions) != OK)
     {
         _K4Search_FreeContext(context);
         return NOTOK;
     }

     // Create the K33-specific structures if the size of the graph is known
     // Attach functions are always invoked after gp_New(), but if a graph
     // extension must be attached before gp_Read(), then the attachment
     // also happens before gp_InitGraph(), which means N==0.
     // However, sometimes a feature is attached after gp_InitGraph(), in
     // which case N > 0
     if (theGraph->N > 0)
     {
         if (_K4Search_CreateStructures(context) != OK ||
             _K4Search_InitStructures(context) != OK)
         {
             _K4Search_FreeContext(context);
             return NOTOK;
         }
     }

     return OK;
}

/********************************************************************
 gp_DetachK4Search()
 ********************************************************************/

int gp_DetachK4Search(graphP theGraph)
{
    return gp_RemoveExtension(theGraph, K4SEARCH_ID);
}

/********************************************************************
 _K4Search_ClearStructures()
 ********************************************************************/

void _K4Search_ClearStructures(K4SearchContext *context)
{
    if (!context->initialized)
    {
        // Before initialization, the pointers are stray, not NULL
        // Once NULL or allocated, free() or LCFree() can do the job
        context->E = NULL;

        context->initialized = 1;
    }
    else
    {
        if (context->E != NULL)
        {
            free(context->E);
            context->E = NULL;
        }
    }
}

/********************************************************************
 _K4Search_CreateStructures()
 Create uninitialized structures for the vertex and edge levels, and
 initialized structures for the graph level
 ********************************************************************/
int  _K4Search_CreateStructures(K4SearchContext *context)
{
     int N = context->theGraph->N;
     int Esize = context->theGraph->arcCapacity;

     if (N <= 0)
         return NOTOK;

     if ((context->E = (K4Search_EdgeRecP) malloc(Esize*sizeof(K4Search_EdgeRec))) == NULL ||
        0)
     {
         return NOTOK;
     }

     return OK;
}

/********************************************************************
 _K4Search_InitStructures()
 ********************************************************************/
int  _K4Search_InitStructures(K4SearchContext *context)
{
     int J, Esize = context->theGraph->arcCapacity;

     if (context->theGraph->N <= 0)
         return OK;

     for (J = 0; J < Esize; J++)
          _InitK4SearchEdgeRec(context, J);

     return OK;
}

/********************************************************************
 ********************************************************************/

int  _K4Search_InitGraph(graphP theGraph, int N)
{
    K4SearchContext *context = NULL;
    gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

    if (context == NULL)
        return NOTOK;
    {
        theGraph->N = N;
        theGraph->NV = N;
        if (theGraph->arcCapacity == 0)
        	theGraph->arcCapacity = 2*DEFAULT_EDGE_LIMIT*N;

        if (_K4Search_CreateStructures(context) != OK)
            return NOTOK;

        // This call initializes the base graph structures, but it also
        // initializes the custom dge and vertex level structures due to
        // overloads of InitEdgeRec, InitVertexRec and/or InitVertexInfo
        context->functions.fpInitGraph(theGraph, N);
    }

    return OK;
}

/********************************************************************
 ********************************************************************/

void _K4Search_ReinitializeGraph(graphP theGraph)
{
    K4SearchContext *context = NULL;
    gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        // Reinitialization can go much faster if the underlying init
        // functions for edges (and vertices, when applicable) are called,
        // rather than the overloads of this module, because it avoids
        // lots of unnecessary gp_FindExtension() calls.
        if (theGraph->functions.fpInitEdgeRec == _K4Search_InitEdgeRec &&
            1)
        {
            // Restore selected graph function pointer(s)
            theGraph->functions.fpInitEdgeRec = context->functions.fpInitEdgeRec;

            // Reinitialize the graph
            context->functions.fpReinitializeGraph(theGraph);

            // Restore the selected function pointer(s) that attach this feature
            theGraph->functions.fpInitEdgeRec = _K4Search_InitEdgeRec;

            // Do the reinitialization that is specific to this module
            _K4Search_InitStructures(context);
        }

        // If optimization is not possible, then just stick with what works.
        // Reinitialize the graph-level structure and then invoke the
        // reinitialize function.
        else
        {
            // The underlying function fpReinitializeGraph() implicitly initializes the K33
            // structures due to the overloads of fpInitEdgeRec() and fpInitVertexInfo().
            // It just does so less efficiently because each invocation of InitEdgeRec
            // and InitVertexInfo has to look up the extension again.
            //// _K4Search_InitStructures(context);
            context->functions.fpReinitializeGraph(theGraph);
        }
    }
}

/********************************************************************
 The current implementation does not support an increase of arc
 (edge record) capacity once the extension is attached to the graph
 data structure.  This is only due to not being necessary to support.
 For now, it is easy to ensure the correct capacity before attaching
 the extension, but support could be added later if there is some
 reason to do so.
 ********************************************************************/

int  _K4Search_EnsureArcCapacity(graphP theGraph, int requiredArcCapacity)
{
	return NOTOK;
}

/********************************************************************
 _K4Search_DupContext()
 ********************************************************************/

void *_K4Search_DupContext(void *pContext, void *theGraph)
{
     K4SearchContext *context = (K4SearchContext *) pContext;
     K4SearchContext *newContext = (K4SearchContext *) malloc(sizeof(K4SearchContext));

     if (newContext != NULL)
     {
         int N = ((graphP) theGraph)->N;
         int Esize = ((graphP) theGraph)->arcCapacity;

         *newContext = *context;

         newContext->theGraph = (graphP) theGraph;

         newContext->initialized = 0;
         _K4Search_ClearStructures(newContext);
         if (N > 0)
         {
             if (_K4Search_CreateStructures(newContext) != OK)
             {
                 _K4Search_FreeContext(newContext);
                 return NULL;
             }

             memcpy(newContext->E, context->E, Esize*sizeof(K4Search_EdgeRec));
         }
     }

     return newContext;
}

/********************************************************************
 _K4Search_FreeContext()
 ********************************************************************/

void _K4Search_FreeContext(void *pContext)
{
     K4SearchContext *context = (K4SearchContext *) pContext;

     _K4Search_ClearStructures(context);
     free(pContext);
}

/********************************************************************
 ********************************************************************/

void _K4Search_InitEdgeRec(graphP theGraph, int J)
{
    K4SearchContext *context = NULL;
    gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        context->functions.fpInitEdgeRec(theGraph, J);
        _InitK4SearchEdgeRec(context, J);
    }
}

void _InitK4SearchEdgeRec(K4SearchContext *context, int J)
{
    context->E[J].pathConnector = NIL;
}

/********************************************************************
 * This function is called if the outerplanarity algorithm fails to
 * embed all back edges for a vertex I.  This means that an obstruction
 * to outerplanarity has occurred, so we determine if it is a subgraph
 * homeomorphic to K4.  If so, then NONEMBEDDABLE is returned.  If not,
 * then a reduction is performed that unobstructs outerplanarity and
 * OK is returned, which allows the outerplanarity algorithm to
 * proceed with iteration I-1 (or to stop if I==0).
 ********************************************************************/

int  _K4Search_HandleBlockedEmbedIteration(graphP theGraph, int I)
{
    if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK4)
    {
    	// If the fwdArcList is empty, then the K4 was already isolated
    	// by _K4Search_HandleBlockedDescendantBicomp(), and we just
    	// return the NONEMBEDDABLE result in order to stop the embedding
    	// iteration loop.
		if (gp_GetVertexFwdArcList(theGraph, I) == NIL)
			return NONEMBEDDABLE;

        return _SearchForK4InBicomps(theGraph, I);
    }
    else
    {
        K4SearchContext *context = NULL;
        gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpHandleBlockedEmbedIteration(theGraph, I);
        }
    }

    return NOTOK;
}

/********************************************************************
 This function is called when outerplanarity obstruction minor A is
 encountered by the WalkDown.  In the implementation for the core
 planarity/outerplanarity algorithm, this method simply pushes the
 blocked bicomp root onto the stack and returns NONEMBEDDABLE, which
 causes the WalkDown to terminate.  The embed postprocessing would
 then go on to isolate the obstruction.

 However, outerplanarity obstruction minor A corresponds to a K_{2,3}
 homeomorph.  This method invokes a search for a K_4 homeomorph that
 may be entangled with the K_{2,3} homeomorph.  If an entangled K_4
 homeomorph is found, then _SearchForK4() returns NONEMBEDDABLE, which
 causes the WalkDown to terminate as above.  This is correct since a
 K_4 homeomorph has been found and isolated, and the K4Search overload
 of EmbedPostprocess() does no additional work.

 On the other hand, if minor A is found but there is no entangled K_4
 homeomorph, then the blocked descendant was reduced to a single edge
 so that it no longer obstructs outerplanarity. Then, OK was returned
 to indicate that the WalkDown should proceed.  This function then
 sets the vertex W and directional information that must be returned
 so that WalkDown can proceed.

 Returns OK to proceed with WalkDown at W,
         NONEMBEDDABLE to terminate WalkDown of Root Vertex
         NOTOK for internal error
 ********************************************************************/

int  _K4Search_HandleBlockedDescendantBicomp(graphP theGraph, int I, int RootVertex, int R, int *pRout, int *pW, int *pWPrevLink)
{
    if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK4)
    {
    	int RetVal = _SearchForK4InBicomp(theGraph, NULL, I, R);

    	// On internal error (NOTOK) or K4 found (NONEMBEDDABLE), we return.
    	if (RetVal != OK)
    		return RetVal;

    	// Since the bicomp rooted by R is now a singleton edge, either direction
    	// out of R and into W can be selected, as long as they are consistent
    	// We just choose the settings associated with selecting W as the next
    	// vertex from R on the external face.
    	*pRout = 0;
    	*pWPrevLink = 1;
    	*pW = _GetNextVertexOnExternalFace(theGraph, R, pWPrevLink);

        // Now return OK so the Walkdown can continue at W (i.e. *pW)
        return OK;
    }
    else
    {
        K4SearchContext *context = NULL;
        gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpHandleBlockedDescendantBicomp(theGraph, I, RootVertex, R, pRout, pW, pWPrevLink);
        }
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K4Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult)
{
     // For K4 search, we just return the edge embedding result because the
     // search result has been obtained already.
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK4)
     {
         return edgeEmbeddingResult;
     }

     // When not searching for K4, we let the superclass do the work
     else
     {
        K4SearchContext *context = NULL;
        gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpEmbedPostprocess(theGraph, I, edgeEmbeddingResult);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K4Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph)
{
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK4)
     {
         return OK;
     }

     // When not searching for K4, we let the superclass do the work
     else
     {
        K4SearchContext *context = NULL;
        gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckEmbeddingIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K4Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph)
{
     // When searching for K4, we ensure that theGraph is a subgraph of
     // the original graph and that it contains a K4 homeomorph
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK4)
     {
		int  degrees[4], imageVerts[4];

        if (_TestSubgraph(theGraph, origGraph) != TRUE)
            return NOTOK;

		if (_getImageVertices(theGraph, degrees, 3, imageVerts, 4) != OK)
			return NOTOK;

		if (_TestForCompleteGraphObstruction(theGraph, 4, degrees, imageVerts) == TRUE)
		{
			return OK;
		}

		return NOTOK;
     }

     // When not searching for K4, we let the superclass do the work
     else
     {
        K4SearchContext *context = NULL;
        gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckObstructionIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}
