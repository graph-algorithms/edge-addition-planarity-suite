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

#include "graphK33Search.private.h"
#include "graphK33Search.h"

extern int  _SearchForMergeBlocker(graphP theGraph, K33SearchContext *context, int I, int *pMergeBlocker);
extern int  _SearchForK33(graphP theGraph, int I);

extern int  _TestForK33GraphObstruction(graphP theGraph, int *degrees, int *imageVerts);
extern int  _getImageVertices(graphP theGraph, int *degrees, int maxDegree,
                              int *imageVerts, int maxNumImageVerts);
extern int  _TestSubgraph(graphP theSubgraph, graphP theGraph);

/* Forward declarations of local functions */

void _K33Search_ClearStructures(K33SearchContext *context);
int  _K33Search_CreateStructures(K33SearchContext *context);
int  _K33Search_InitStructures(K33SearchContext *context);

/* Forward declarations of overloading functions */

int  _K33Search_CreateFwdArcLists(graphP theGraph);
void _K33Search_CreateDFSTreeEmbedding(graphP theGraph);
void _K33Search_EmbedBackEdgeToDescendant(graphP theGraph, int RootSide, int RootVertex, int W, int WPrevLink);
int  _K33Search_MergeBicomps(graphP theGraph, int I, int RootVertex, int W, int WPrevLink);
int  _K33Search_MarkDFSPath(graphP theGraph, int ancestor, int descendant);
int  _K33Search_HandleBlockedEmbedIteration(graphP theGraph, int I);
int  _K33Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult);
int  _K33Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph);
int  _K33Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph);

void _K33Search_InitEdgeRec(graphP theGraph, int J);
void _InitK33SearchEdgeRec(K33SearchContext *context, int J);
void _K33Search_InitVertexInfo(graphP theGraph, int I);
void _InitK33SearchVertexInfo(K33SearchContext *context, int I);

int  _K33Search_InitGraph(graphP theGraph, int N);
void _K33Search_ReinitializeGraph(graphP theGraph);
int  _K33Search_EnsureArcCapacity(graphP theGraph, int requiredArcCapacity);

/* Forward declarations of functions used by the extension system */

void *_K33Search_DupContext(void *pContext, void *theGraph);
void _K33Search_FreeContext(void *);

/****************************************************************************
 * K33SEARCH_ID - the variable used to hold the integer identifier for this
 * extension, enabling this feature's extension context to be distinguished
 * from other features' extension contexts that may be attached to a graph.
 ****************************************************************************/

int K33SEARCH_ID = 0;

/****************************************************************************
 gp_AttachK33Search()

 This function adjusts the graph data structure to attach the K3,3 search
 feature.
 ****************************************************************************/

int  gp_AttachK33Search(graphP theGraph)
{
     K33SearchContext *context = NULL;

     // If the K3,3 search feature has already been attached to the graph,
     // then there is no need to attach it again
     gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
     if (context != NULL)
     {
         return OK;
     }

     // Allocate a new extension context
     context = (K33SearchContext *) malloc(sizeof(K33SearchContext));
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

     context->functions.fpCreateFwdArcLists = _K33Search_CreateFwdArcLists;
     context->functions.fpCreateDFSTreeEmbedding = _K33Search_CreateDFSTreeEmbedding;
     context->functions.fpEmbedBackEdgeToDescendant = _K33Search_EmbedBackEdgeToDescendant;
     context->functions.fpMergeBicomps = _K33Search_MergeBicomps;
     context->functions.fpMarkDFSPath = _K33Search_MarkDFSPath;
     context->functions.fpHandleBlockedEmbedIteration = _K33Search_HandleBlockedEmbedIteration;
     context->functions.fpEmbedPostprocess = _K33Search_EmbedPostprocess;
     context->functions.fpCheckEmbeddingIntegrity = _K33Search_CheckEmbeddingIntegrity;
     context->functions.fpCheckObstructionIntegrity = _K33Search_CheckObstructionIntegrity;

     context->functions.fpInitVertexInfo = _K33Search_InitVertexInfo;
     context->functions.fpInitEdgeRec = _K33Search_InitEdgeRec;

     context->functions.fpInitGraph = _K33Search_InitGraph;
     context->functions.fpReinitializeGraph = _K33Search_ReinitializeGraph;
     context->functions.fpEnsureArcCapacity = _K33Search_EnsureArcCapacity;

     _K33Search_ClearStructures(context);

     // Store the K33 search context, including the data structure and the
     // function pointers, as an extension of the graph
     if (gp_AddExtension(theGraph, &K33SEARCH_ID, (void *) context,
                         _K33Search_DupContext, _K33Search_FreeContext,
                         &context->functions) != OK)
     {
         _K33Search_FreeContext(context);
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
         if (_K33Search_CreateStructures(context) != OK ||
             _K33Search_InitStructures(context) != OK)
         {
             _K33Search_FreeContext(context);
             return NOTOK;
         }
     }

     return OK;
}

/********************************************************************
 gp_DetachK33Search()
 ********************************************************************/

int gp_DetachK33Search(graphP theGraph)
{
    return gp_RemoveExtension(theGraph, K33SEARCH_ID);
}

/********************************************************************
 _K33Search_ClearStructures()
 ********************************************************************/

void _K33Search_ClearStructures(K33SearchContext *context)
{
    if (!context->initialized)
    {
        // Before initialization, the pointers are stray, not NULL
        // Once NULL or allocated, free() or LCFree() can do the job
        context->sortedDFSChildLists = NULL;
        context->E = NULL;
        context->VI = NULL;

        context->initialized = 1;
    }
    else
    {
        LCFree(&context->sortedDFSChildLists);
        if (context->E != NULL)
        {
            free(context->E);
            context->E = NULL;
        }
        if (context->VI != NULL)
        {
            free(context->VI);
            context->VI = NULL;
        }
    }
}

/********************************************************************
 _K33Search_CreateStructures()
 Create uninitialized structures for the vertex and edge
 levels, and initialized structures for the graph level
 ********************************************************************/
int  _K33Search_CreateStructures(K33SearchContext *context)
{
     int N = context->theGraph->N;
     int Esize = context->theGraph->arcCapacity;

     if (N <= 0)
         return NOTOK;

     if ((context->sortedDFSChildLists = LCNew(N)) == NULL ||
         (context->E = (K33Search_EdgeRecP) malloc(Esize*sizeof(K33Search_EdgeRec))) == NULL ||
         (context->VI = (K33Search_VertexInfoP) malloc(N*sizeof(K33Search_VertexInfo))) == NULL
        )
     {
         return NOTOK;
     }

     return OK;
}

/********************************************************************
 _K33Search_InitStructures()
 ********************************************************************/
int  _K33Search_InitStructures(K33SearchContext *context)
{
     int I, J, N = context->theGraph->N;
     int Esize = context->theGraph->arcCapacity;

     if (N <= 0)
         return OK;

     for (I = 0; I < N; I++)
          _InitK33SearchVertexInfo(context, I);

     for (J = 0; J < Esize; J++)
          _InitK33SearchEdgeRec(context, J);

     return OK;
}

/********************************************************************
 ********************************************************************/

int  _K33Search_InitGraph(graphP theGraph, int N)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context == NULL)
        return NOTOK;
    {
        theGraph->N = N;
        theGraph->NV = N;
        if (theGraph->arcCapacity == 0)
        	theGraph->arcCapacity = 2*DEFAULT_EDGE_LIMIT*N;

        if (_K33Search_CreateStructures(context) != OK)
            return NOTOK;

        // This call initializes the base graph structures, but it also
        // initializes the custom edge and vertex level structures
        // due to the overloads of InitEdgeRec and InitVertexInfo
        context->functions.fpInitGraph(theGraph, N);
    }

    return OK;
}

/********************************************************************
 ********************************************************************/

void _K33Search_ReinitializeGraph(graphP theGraph)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        // Reinitialization can go much faster if the underlying
        // init functions for edges and vertices are called,
        // rather than the overloads of this module, because it
        // avoids lots of unnecessary gp_FindExtension() calls.
        if (theGraph->functions.fpInitEdgeRec == _K33Search_InitEdgeRec &&
            theGraph->functions.fpInitVertexInfo == _K33Search_InitVertexInfo)
        {
            // Restore the graph function pointers
            theGraph->functions.fpInitEdgeRec = context->functions.fpInitEdgeRec;
            theGraph->functions.fpInitVertexInfo = context->functions.fpInitVertexInfo;

            // Reinitialize the graph
            context->functions.fpReinitializeGraph(theGraph);

            // Restore the function pointers that attach this feature
            theGraph->functions.fpInitEdgeRec = _K33Search_InitEdgeRec;
            theGraph->functions.fpInitVertexInfo = _K33Search_InitVertexInfo;

            // Do the reinitialization that is specific to this module
            _K33Search_InitStructures(context);
            LCReset(context->sortedDFSChildLists);
        }

        // If optimization is not possible, then just stick with what works.
        // Reinitialize the graph-level structure and then invoke the
        // reinitialize function.
        else
        {
            LCReset(context->sortedDFSChildLists);

            // The underlying function fpReinitializeGraph() implicitly initializes the K33
            // structures due to the overloads of fpInitEdgeRec() and fpInitVertexInfo().
            // It just does so less efficiently because each invocation of InitEdgeRec
            // and InitVertexInfo has to look up the extension again.
            //// _K33Search_InitStructures(context);
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

int  _K33Search_EnsureArcCapacity(graphP theGraph, int requiredArcCapacity)
{
	return NOTOK;
}

/********************************************************************
 _K33Search_DupContext()
 ********************************************************************/

void *_K33Search_DupContext(void *pContext, void *theGraph)
{
     K33SearchContext *context = (K33SearchContext *) pContext;
     K33SearchContext *newContext = (K33SearchContext *) malloc(sizeof(K33SearchContext));

     if (newContext != NULL)
     {
         int N = ((graphP) theGraph)->N;
         int Esize = ((graphP) theGraph)->arcCapacity;

         *newContext = *context;

         newContext->theGraph = (graphP) theGraph;

         newContext->initialized = 0;
         _K33Search_ClearStructures(newContext);
         if (N > 0)
         {
             if (_K33Search_CreateStructures(newContext) != OK)
             {
                 _K33Search_FreeContext(newContext);
                 return NULL;
             }

             LCCopy(newContext->sortedDFSChildLists, context->sortedDFSChildLists);
             memcpy(newContext->E, context->E, Esize*sizeof(K33Search_EdgeRec));
             memcpy(newContext->VI, context->VI, N*sizeof(K33Search_VertexInfo));
         }
     }

     return newContext;
}

/********************************************************************
 _K33Search_FreeContext()
 ********************************************************************/

void _K33Search_FreeContext(void *pContext)
{
     K33SearchContext *context = (K33SearchContext *) pContext;

     _K33Search_ClearStructures(context);
     free(pContext);
}

/********************************************************************
 _K33Search_CreateFwdArcLists()

 Puts the forward arcs (back edges from a vertex to its descendants)
 into a circular list indicated by the fwdArcList member, a task
 simplified by the fact that they have already been placed in
 succession at the end of the adjacency list.

 For K3,3 search, the forward edges must be sorted.  The sort is linear
 time, but it is a little slower, so we avoid this cost for the other
 planarity-related algorithms.

  Returns OK on success, NOTOK on internal code failure
 ********************************************************************/

int _K33Search_CreateFwdArcLists(graphP theGraph)
{
    K33SearchContext *context = NULL;

    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // For isolating a K_{3,3} homeomorph, we need the forward edges
    // of each vertex to be in sorted order by DFI of descendants.
    // Otherwise we just drop through to the normal processing...

    if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
    {
    int I, Jcur, Jnext, ancestor;

        // for each vertex v in order, we follow each of its back edges
        // to the twin forward edge in an ancestor u, then we move
        // the forward edge to the fwdArcList of u.  Since this loop
        // processes vertices in order, the fwdArcList of each vertex
        // will be in order by the neighbor indicated by the forward edges.

        for (I=0; I < theGraph->N; I++)
        {
        	// Skip this vertex if it has no edges
        	Jnext = gp_GetLastArc(theGraph, I);
        	if (!gp_IsArc(theGraph, Jnext))
        		continue;

            // Skip the forward edges, which are in succession at the
        	// end of the arc list (last and its predecessors)
            while (gp_GetEdgeType(theGraph, Jnext) == EDGE_TYPE_FORWARD)
                Jnext = gp_GetPrevArc(theGraph, Jnext);

            // Now we want to put all the back arcs in a backArcList, too.
            // Since we've already skipped past the forward arcs, we continue
            // with the predecessor arcs until we either run out of arcs or
            // we find a DFS child arc (the DFS child arcs are in succession
            // at the beginning of the arc list, so when a child arc is
            // encountered in the predecessor direction, then there won't be
            // any more back arcs.
            while (gp_IsArc(theGraph, Jnext) &&
                   gp_GetEdgeType(theGraph, Jnext) != EDGE_TYPE_CHILD)
            {
                Jcur = Jnext;
                Jnext = gp_GetPrevArc(theGraph, Jnext);

                if (gp_GetEdgeType(theGraph, Jcur) == EDGE_TYPE_BACK)
                {
                    // Remove the back arc from I's adjacency list
                	gp_DetachArc(theGraph, Jcur);

                    // Put the back arc in the backArcList
                    if (context->VI[I].backArcList == NIL)
                    {
                        context->VI[I].backArcList = Jcur;
                        gp_SetPrevArc(theGraph, Jcur, Jcur);
                        gp_SetNextArc(theGraph, Jcur, Jcur);
                    }
                    else
                    {
                    	gp_AttachArc(theGraph, NIL, context->VI[I].backArcList, 1, Jcur);
                    }

                    // Determine the ancestor of vertex I to which Jcur connects
                    ancestor = gp_GetNeighbor(theGraph, Jcur);

                    // Go to the forward arc in the ancestor
                    Jcur = gp_GetTwinArc(theGraph, Jcur);

                    // Remove the forward arc from the ancestor's adjacency list
                	gp_DetachArc(theGraph, Jcur);

                    // Add the forward arc to the end of the fwdArcList.
                    if (gp_GetVertexFwdArcList(theGraph, ancestor) == NIL)
                    {
                        gp_SetVertexFwdArcList(theGraph, ancestor, Jcur);
                        gp_SetPrevArc(theGraph, Jcur, Jcur);
                        gp_SetNextArc(theGraph, Jcur, Jcur);
                    }
                    else
                    {
                    	gp_AttachArc(theGraph, NIL, gp_GetVertexFwdArcList(theGraph, ancestor), 1, Jcur);
                    }
                }
            }
        }

        // Since the fwdArcLists have been created, we do not fall through
        // to run the superclass implementation
        return OK;
    }

    // If we're not actually running a K3,3 search, then we just run the
    // superclass implementation
    return context->functions.fpCreateFwdArcLists(theGraph);
}

/********************************************************************
 _K33Search_CreateDFSTreeEmbedding()

 This function overloads the basic planarity version in the manner
 explained by the comments below.  Once the extra behavior is
 performed, the basic planarity version is invoked.
 ********************************************************************/

void _K33Search_CreateDFSTreeEmbedding(graphP theGraph)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        // When searching for K_{3,3} homeomorphs, we need the
        // list of DFS children for each vertex, which gets lost
        // during the initial tree embedding (each DFS tree child
        // arc is moved to the root copy of the vertex)

        if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
        {
            int I, J, N;

            N = theGraph->N;

            for (I=0; I<N; I++)
            {
                J = gp_GetFirstArc(theGraph, I);

                // If a vertex has any DFS children, the edges
                // to them are stored in descending order of
                // the DFI's along the successor arc pointers, so
                // we traverse them and prepend each to the
                // ascending order sortedDFSChildList

                while (gp_IsArc(theGraph, J) && gp_GetEdgeType(theGraph, J) == EDGE_TYPE_CHILD)
                {
                    context->VI[I].sortedDFSChildList =
                        LCPrepend(context->sortedDFSChildLists,
                                    context->VI[I].sortedDFSChildList,
                                    gp_GetNeighbor(theGraph, J));

                    J = gp_GetNextArc(theGraph, J);
                }
            }
        }

        // Invoke the superclass version of the function
        context->functions.fpCreateDFSTreeEmbedding(theGraph);
    }
}

/********************************************************************
 _K33Search_EmbedBackEdgeToDescendant()

 The forward and back arcs of the cycle edge are embedded by the planarity
 version of this function.
 However, for K_{3,3} subgraph homeomorphism, we also maintain the
 list of unembedded back arcs, so we need to remove the back arc from
 that list since it is now being put back into the adjacency list.
 ********************************************************************/

void _K33Search_EmbedBackEdgeToDescendant(graphP theGraph, int RootSide, int RootVertex, int W, int WPrevLink)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        // K33 search may have been attached, but not enabled
        if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
        {
        	// Get the fwdArc from the adjacentTo field, and use it to get the backArc
            int backArc = gp_GetTwinArc(theGraph, gp_GetVertexPertinentAdjacencyInfo(theGraph, W));

            // Remove the backArc from the backArcList
            if (context->VI[W].backArcList == backArc)
            {
                if (gp_GetNextArc(theGraph, backArc) == backArc)
                     context->VI[W].backArcList = NIL;
                else context->VI[W].backArcList = gp_GetNextArc(theGraph, backArc);
            }

            gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, backArc), gp_GetNextArc(theGraph, backArc));
            gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, backArc), gp_GetPrevArc(theGraph, backArc));
        }

        // Invoke the superclass version of the function
        context->functions.fpEmbedBackEdgeToDescendant(theGraph, RootSide, RootVertex, W, WPrevLink);
    }
}

/********************************************************************

  This override of _MergeBicomps() detects a special merge block
  that indicates a K3,3 can be found.  The merge blocker is an
  optimization needed for one case for which detecting a K3,3
  could not be done in linear time.

  Returns OK for a successful merge, NOTOK on an internal failure,
          or NONEMBEDDABLE if the merge is blocked
 ********************************************************************/

int  _K33Search_MergeBicomps(graphP theGraph, int I, int RootVertex, int W, int WPrevLink)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        /* If the merge is blocked, then the Walkdown is terminated so a
                K3,3 can be isolated by _SearchForK33() */

        if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
        {
        int mergeBlocker;

            // We want to test all merge points on the stack
            // as well as W, since the connection will go
            // from W.  So we push W as a 'degenerate' merge point.
            sp_Push2(theGraph->theStack, W, WPrevLink);
            sp_Push2(theGraph->theStack, NIL, NIL);

            _SearchForMergeBlocker(theGraph, context, I, &mergeBlocker);

            // If we find a merge blocker, then we return with
            // the stack intact including W so that the merge
            // blocked vertex can be easily found.
            if (mergeBlocker != NIL)
                return NONEMBEDDABLE;

            // If no merge blocker was found, then remove
            // W from the stack.
            sp_Pop2(theGraph->theStack, W, WPrevLink);
            sp_Pop2(theGraph->theStack, W, WPrevLink);
        }

        // If the merge was not blocked, then we perform the merge
        // When not doing a K3,3 search, then the merge is not
        // blocked as far as the K3,3 search method is concerned
        // Another algorithms could overload MergeBicomps and block
        // merges under certain conditions, but those would be based
        // on data maintained by the extension that implements the
        // other algorithm-- if *that* algorithm is the one being run
        return context->functions.fpMergeBicomps(theGraph, I, RootVertex, W, WPrevLink);
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

void _K33Search_InitEdgeRec(graphP theGraph, int J)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        context->functions.fpInitEdgeRec(theGraph, J);
        _InitK33SearchEdgeRec(context, J);
    }
}

void _InitK33SearchEdgeRec(K33SearchContext *context, int J)
{
    context->E[J].noStraddle = NIL;
    context->E[J].pathConnector = NIL;
}

/********************************************************************
 ********************************************************************/

void _K33Search_InitVertexInfo(graphP theGraph, int I)
{
    K33SearchContext *context = NULL;
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

    if (context != NULL)
    {
        context->functions.fpInitVertexInfo(theGraph, I);
        _InitK33SearchVertexInfo(context, I);
    }
}

void _InitK33SearchVertexInfo(K33SearchContext *context, int I)
{
    context->VI[I].sortedDFSChildList = NIL;
    context->VI[I].backArcList = NIL;
    context->VI[I].externalConnectionAncestor = NIL;
    context->VI[I].mergeBlocker = NIL;
}

/********************************************************************
// This function used to be implemented by going to each
// vertex, asking for its DFS parent, then finding the
// edge that lead to that DFS parent and marking it.
// However, the K3,3 search performs a certain bicomp
// reduction that is required to preserve the essential
// structure of the DFS tree.  As a result, sometimes a
// DFSParent has been reduced out of the graph, but the
// tree edge leads to the nearest ancestor still in the
// graph.  So, when we want to leave a vertex, we search
// for the DFS tree edge to the "parent" (nearest ancestor).
// then we mark the edge and use it to go up to the "parent".
 ********************************************************************/

int  _K33Search_MarkDFSPath(graphP theGraph, int ancestor, int descendant)
{
int  J, parent, N;

     N = theGraph->N;

     /* If we are marking from a root vertex upward, then go up to the parent
        copy before starting the loop */

     if (descendant >= N)
         descendant = gp_GetVertexParent(theGraph, descendant-N);

     /* Mark the lowest vertex (the one with the highest number). */

     gp_SetVertexVisited(theGraph, descendant);

     /* Mark all ancestors of the lowest vertex, and the edges used to reach
        them, up to the given ancestor vertex. */

     while (descendant != ancestor)
     {
          if (descendant == NIL)
              return NOTOK;

          /* If we are at a bicomp root, then ascend to its parent copy and
                mark it as visited. */

          if (descendant >= N)
          {
              parent = gp_GetVertexParent(theGraph, descendant-N);
          }

          /* If we are on a regular, non-virtual vertex then get the edge to
                the parent, mark the edge, then fall through to the code
                that marks the parent vertex. */
          else
          {

              /* Scan the edges for the one marked as the DFS parent */

              parent = NIL;
              J = gp_GetFirstArc(theGraph, descendant);
              while (gp_IsArc(theGraph, J))
              {
                  if (gp_GetEdgeType(theGraph, J) == EDGE_TYPE_PARENT)
                  {
                      parent = gp_GetNeighbor(theGraph, J);
                      break;
                  }
                  J = gp_GetNextArc(theGraph, J);
              }

              /* If the desired edge was not found, then the data structure is
                    corrupt, so bail out. */

              if (parent == NIL)
                  return NOTOK;

              /* Mark the edge */

              gp_SetEdgeVisited(theGraph, J);
              gp_SetEdgeVisited(theGraph, gp_GetTwinArc(theGraph, J));
          }

          /* Mark the parent, then hop to the parent and reiterate */

          gp_SetVertexVisited(theGraph, parent);
          descendant = parent;
     }

     return OK;
}

/********************************************************************
 ********************************************************************/

int  _K33Search_HandleBlockedEmbedIteration(graphP theGraph, int I)
{
    if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
        return _SearchForK33(theGraph, I);

    else
    {
        K33SearchContext *context = NULL;
        gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpHandleBlockedEmbedIteration(theGraph, I);
        }
    }

    return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K33Search_EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult)
{
     // For K3,3 search, we just return the edge embedding result because the
     // search result has been obtained already.
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
     {
         return edgeEmbeddingResult;
     }

     // When not searching for K3,3, we let the superclass do the work
     else
     {
        K33SearchContext *context = NULL;
        gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpEmbedPostprocess(theGraph, I, edgeEmbeddingResult);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K33Search_CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph)
{
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
     {
         return OK;
     }

     // When not searching for K3,3, we let the superclass do the work
     else
     {
        K33SearchContext *context = NULL;
        gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckEmbeddingIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}

/********************************************************************
 ********************************************************************/

int  _K33Search_CheckObstructionIntegrity(graphP theGraph, graphP origGraph)
{
     // When searching for K3,3, we ensure that theGraph is a subgraph of
     // the original graph and that it contains a K3,3 homeomorph
     if (theGraph->embedFlags == EMBEDFLAGS_SEARCHFORK33)
     {
         int  degrees[5], imageVerts[6];

         if (_TestSubgraph(theGraph, origGraph) != TRUE)
         {
             return NOTOK;
         }

         if (_getImageVertices(theGraph, degrees, 4, imageVerts, 6) != OK)
         {
             return NOTOK;
         }

         if (_TestForK33GraphObstruction(theGraph, degrees, imageVerts) == TRUE)
         {
             return OK;
         }

         return NOTOK;
     }

     // When not searching for K3,3, we let the superclass do the work
     else
     {
        K33SearchContext *context = NULL;
        gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);

        if (context != NULL)
        {
            return context->functions.fpCheckObstructionIntegrity(theGraph, origGraph);
        }
     }

     return NOTOK;
}

