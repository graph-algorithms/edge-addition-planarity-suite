/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved. Includes a reference implementation of the following:
John M. Boyer and Wendy J. Myrvold, "On the Cutting Edge: Simplified O(n)
Planarity by Edge Addition,"  Journal of Graph Algorithms and Applications,
Vol. 8, No. 3, pp. 241-273, 2004.

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

#define GRAPHSTRUCTURE_C

#include <stdlib.h>

#include "graphStructures.h"
#include "graph.h"

/* Imported functions for FUNCTION POINTERS */

extern int  _CreateFwdArcLists(graphP);
extern void _CreateDFSTreeEmbedding(graphP theGraph);
extern int  _SortVertices(graphP theGraph);
extern void _EmbedBackEdgeToDescendant(graphP theGraph, int RootSide, int RootVertex, int W, int WPrevLink);
extern int  _MergeBicomps(graphP theGraph, int I, int RootVertex, int W, int WPrevLink);
extern int  _HandleInactiveVertex(graphP theGraph, int BicompRoot, int *pW, int *pWPrevLink);
extern int  _MarkDFSPath(graphP theGraph, int ancestor, int descendant);
extern int  _EmbedIterationPostprocess(graphP theGraph, int I);
extern int  _EmbedPostprocess(graphP theGraph, int I, int edgeEmbeddingResult);
extern int  _CheckEmbeddingIntegrity(graphP theGraph, graphP origGraph);
extern int  _CheckObstructionIntegrity(graphP theGraph, graphP origGraph);
extern int  _ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize);
extern int  _WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize);

/********************************************************************
 Private functions, except exported within library
 ********************************************************************/

void _ClearIsolatorContext(graphP theGraph);
void _FillVisitedFlags(graphP theGraph, int FillValue);
void _FillVisitedFlagsInBicomp(graphP theGraph, int BicompRoot, int FillValue);
void _FillVisitedFlagsInOtherBicomps(graphP theGraph, int BicompRoot, int FillValue);
void _FillVisitedFlagsInUnembeddedEdges(graphP theGraph, int FillValue);
void _SetVertexTypeInBicomp(graphP theGraph, int BicompRoot, int theType);

void _HideInternalEdges(graphP theGraph, int vertex);
void _RestoreInternalEdges(graphP theGraph);
int  _GetBicompSize(graphP theGraph, int BicompRoot);
void _DeleteUnmarkedEdgesInBicomp(graphP theGraph, int BicompRoot);
void _ClearInvertedFlagsInBicomp(graphP theGraph, int BicompRoot);

void _AddArc(graphP theGraph, int u, int v, int arcPos, int link);

void _InitFunctionTable(graphP theGraph);

/********************************************************************
 Private functions.
 ********************************************************************/

void _ClearGraph(graphP theGraph);

int  _GetRandomNumber(int NMin, int NMax);

void _HideArc(graphP theGraph, int arcPos);

/* Private functions for which there are FUNCTION POINTERS */

void _InitGraphNode(graphP theGraph, int I);
void _InitVertexRec(graphP theGraph, int I);

int  _InitGraph(graphP theGraph, int N);
void _ReinitializeGraph(graphP theGraph);

/********************************************************************
 gp_New()
 Constructor for graph object.
 Can create two graphs if restricted to no dynamic memory.
 ********************************************************************/

graphP gp_New()
{
graphP theGraph = (graphP) malloc(sizeof(baseGraphStructure));

     if (theGraph != NULL)
     {
         theGraph->G = NULL;
         theGraph->V = NULL;

         theGraph->BicompLists = NULL;
         theGraph->DFSChildLists = NULL;
         theGraph->theStack = NULL;

         theGraph->buckets = NULL;
         theGraph->bin = NULL;

         theGraph->extFace = NULL;

         theGraph->edgeHoles = NULL;

         theGraph->extensions = NULL;

         _InitFunctionTable(theGraph);

         _ClearGraph(theGraph);
     }

     return theGraph;
}

/********************************************************************
 _InitFunctionTable()

 If you add functions to the function table, then they must be
 initialized here, but you must also add the new function pointer
 to the definition of the graphFunctionTable in graphFunctionTable.h

 Function headers for the functions used to initialize the table are
 classified at the top of this file as either imported from other
 compilation units (extern) or private to this compilation unit.
 Search for FUNCTION POINTERS in this file to see where to add the
 function header.
 ********************************************************************/

void _InitFunctionTable(graphP theGraph)
{
     theGraph->functions.fpCreateFwdArcLists = _CreateFwdArcLists;
     theGraph->functions.fpCreateDFSTreeEmbedding = _CreateDFSTreeEmbedding;
     theGraph->functions.fpEmbedBackEdgeToDescendant = _EmbedBackEdgeToDescendant;
     theGraph->functions.fpMergeBicomps = _MergeBicomps;
     theGraph->functions.fpHandleInactiveVertex = _HandleInactiveVertex;
     theGraph->functions.fpMarkDFSPath = _MarkDFSPath;
     theGraph->functions.fpEmbedIterationPostprocess = _EmbedIterationPostprocess;
     theGraph->functions.fpEmbedPostprocess = _EmbedPostprocess;
     theGraph->functions.fpCheckEmbeddingIntegrity = _CheckEmbeddingIntegrity;
     theGraph->functions.fpCheckObstructionIntegrity = _CheckObstructionIntegrity;

     theGraph->functions.fpInitGraphNode = _InitGraphNode;
     theGraph->functions.fpInitVertexRec = _InitVertexRec;

     theGraph->functions.fpInitGraph = _InitGraph;
     theGraph->functions.fpReinitializeGraph = _ReinitializeGraph;
     theGraph->functions.fpSortVertices = _SortVertices;

     theGraph->functions.fpReadPostprocess = _ReadPostprocess;
     theGraph->functions.fpWritePostprocess = _WritePostprocess;
}

/********************************************************************
 gp_InitGraph()
 Allocates memory for graph and vertex records now that N is known.
 For G, we need N vertex nodes, N more vertex nodes for root copies,
        (2 * EDGE_LIMIT * N) edge records.
 For V, we need N vertex records.
 The BicompLists and DFSChildLists are of size N and start out empty.
 The stack, initially empty, is made big enough for a pair of integers per
        edge record, or 2 * 2 * EDGE_LIMIT * N.
 buckets and bin are both O(n) in size.  They are used by
        CreateSortedSeparatedDFSChildLists()

  Returns OK on success, NOTOK on all failures.
          On NOTOK, graph extensions are freed so that the graph is
          returned to the post-condition of gp_New().
 ********************************************************************/

int  gp_InitGraph(graphP theGraph, int N)
{
     return theGraph->functions.fpInitGraph(theGraph, N);
}

int  _InitGraph(graphP theGraph, int N)
{
int I, edgeOffset, Gsize;

/* Compute the number of graph nodes for vertices (and root copies) and
   the total number of graph nodes */

     edgeOffset = 2*N;
     Gsize = edgeOffset + 2*EDGE_LIMIT*N;

/* Allocate memory as described above */

     if ((theGraph->G = (graphNodeP) malloc(Gsize*sizeof(graphNode))) == NULL ||
         (theGraph->V = (vertexRecP) malloc(N*sizeof(vertexRec))) == NULL ||
         (theGraph->BicompLists = LCNew(N)) == NULL ||
         (theGraph->DFSChildLists = LCNew(N)) == NULL ||
         (theGraph->theStack = sp_New(2 * 2 * EDGE_LIMIT * N)) == NULL ||
         (theGraph->buckets = (int *) malloc(N * sizeof(int))) == NULL ||
         (theGraph->bin = LCNew(N)) == NULL ||
         (theGraph->extFace = (extFaceLinkRecP) malloc(edgeOffset*sizeof(extFaceLinkRec))) == NULL ||
         (theGraph->edgeHoles = sp_New(EDGE_LIMIT * N)) == NULL ||
         0)
     {
         _ClearGraph(theGraph);
         return NOTOK;
     }

/* Initialize memory */

     theGraph->N = N;
     theGraph->edgeOffset = edgeOffset;

     for (I = 0; I < Gsize; I++)
          theGraph->functions.fpInitGraphNode(theGraph, I);

     for (I = 0; I < N; I++)
          theGraph->functions.fpInitVertexRec(theGraph, I);

     for (I = 0; I < edgeOffset; I++)
     {
         theGraph->extFace[I].vertex[0] = theGraph->extFace[I].vertex[1] = NIL;
         theGraph->extFace[I].inversionFlag = 0;
     }

     _ClearIsolatorContext(theGraph);

     return OK;
}

/********************************************************************
 gp_ReinitializeGraph()
 Reinitializes a graph, restoring it to the state it was in immediately
 gp_InitGraph() processed it.
 ********************************************************************/

void gp_ReinitializeGraph(graphP theGraph)
{
    theGraph->functions.fpReinitializeGraph(theGraph);
}

void _ReinitializeGraph(graphP theGraph)
{
int  I, N = theGraph->N, edgeOffset = theGraph->edgeOffset;
int  Gsize = edgeOffset + 2*EDGE_LIMIT*N;

     theGraph->M = 0;
     theGraph->internalFlags = theGraph->embedFlags = 0;

     for (I = 0; I < Gsize; I++)
          theGraph->functions.fpInitGraphNode(theGraph, I);

     for (I = 0; I < N; I++)
          theGraph->functions.fpInitVertexRec(theGraph, I);

     for (I = 0; I < edgeOffset; I++)
     {
         theGraph->extFace[I].vertex[0] = theGraph->extFace[I].vertex[1] = NIL;
         theGraph->extFace[I].inversionFlag = 0;
     }

     _ClearIsolatorContext(theGraph);

     LCReset(theGraph->BicompLists);
     LCReset(theGraph->DFSChildLists);
     sp_ClearStack(theGraph->theStack);
     LCReset(theGraph->bin);
     sp_ClearStack(theGraph->edgeHoles);
}

/********************************************************************
 _InitGraphNode()
 Sets the fields in a single graph node structure to initial values
 ********************************************************************/

void _InitGraphNode(graphP theGraph, int I)
{
     theGraph->G[I].v =
     theGraph->G[I].link[0] =
     theGraph->G[I].link[1] = NIL;
     theGraph->G[I].visited = 0;
     theGraph->G[I].type = TYPE_UNKNOWN;
     theGraph->G[I].flags = 0;
}

/********************************************************************
 _InitVertexRec()
 Sets the fields in a single vertex record to initial values
 ********************************************************************/

void _InitVertexRec(graphP theGraph, int I)
{
     theGraph->V[I].leastAncestor =
     theGraph->V[I].Lowpoint = I;
     theGraph->V[I].DFSParent = NIL;
     theGraph->V[I].adjacentTo = NIL;
     theGraph->V[I].pertinentBicompList = NIL;
     theGraph->V[I].separatedDFSChildList = NIL;
     theGraph->V[I].fwdArcList = NIL;
}

/********************************************************************
 _ClearIsolatorContext()
 ********************************************************************/

void _ClearIsolatorContext(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     IC->minorType = 0;
     IC->v = IC->r = IC->x = IC->y = IC->w = IC->px = IC->py = IC->z =
     IC->ux = IC->dx = IC->uy = IC->dy = IC->dw = IC->uz = IC->dz = NIL;
}

/********************************************************************
 _FillVisitedFlags()
 ********************************************************************/

void _FillVisitedFlags(graphP theGraph, int FillValue)
{
int  i;
int  limit = theGraph->edgeOffset + 2*(theGraph->M + sp_GetCurrentSize(theGraph->edgeHoles));

     for (i=0; i < limit; i++)
          theGraph->G[i].visited = FillValue;
}

/********************************************************************
 _FillVisitedFlagsInBicomp()
 ********************************************************************/

void _FillVisitedFlagsInBicomp(graphP theGraph, int BicompRoot, int FillValue)
{
int  V, J;

     sp_ClearStack(theGraph->theStack);
     sp_Push(theGraph->theStack, BicompRoot);
     while (sp_NonEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, V);
          theGraph->G[V].visited = FillValue;

          J = gp_GetFirstArc(theGraph, V);
          while (gp_IsArc(theGraph, J))
          {
             theGraph->G[J].visited = FillValue;

             if (theGraph->G[J].type == EDGE_DFSCHILD)
                 sp_Push(theGraph->theStack, theGraph->G[J].v);

             J = gp_GetNextArc(theGraph, J);
          }
     }
}

/********************************************************************
 _FillVisitedFlagsInOtherBicomps()
 Typically, we want to clear or set all visited flags in the graph
 (see _FillVisitedFlags).  However, in some algorithms this would be
 too costly, so it is necessary to clear or set the visited flags only
 in one bicomp (see _FillVisitedFlagsInBicomp), then do some processing
 that sets some of the flags then performs some tests.  If the tests
 are positive, then we can clear or set all the visited flags in the
 other bicomps (the processing may have set the visited flags in the
 one bicomp in a particular way that we want to retain, so we skip
 the given bicomp).
 ********************************************************************/

void _FillVisitedFlagsInOtherBicomps(graphP theGraph, int BicompRoot, int FillValue)
{
int  R, edgeOffset = theGraph->edgeOffset;

     for (R = theGraph->N; R < edgeOffset; R++)
          if (R != BicompRoot &&
        	  gp_IsArc(theGraph, gp_GetFirstArc(theGraph, R)) )
              _FillVisitedFlagsInBicomp(theGraph, R, FillValue);
}

/********************************************************************
 _FillVisitedFlagsInUnembeddedEdges()
 Unembedded edges aren't part of any bicomp yet, but it may be
 necessary to fill their visited flags, typically with zero.
 ********************************************************************/

void _FillVisitedFlagsInUnembeddedEdges(graphP theGraph, int FillValue)
{
int I, J;

    for (I = 0; I < theGraph->N; I++)
    {
        J = theGraph->V[I].fwdArcList;
        while (gp_IsArc(theGraph, J))
        {
            theGraph->G[J].visited =
            theGraph->G[gp_GetTwinArc(theGraph, J)].visited = FillValue;

            J = gp_GetNextArc(theGraph, J);
            if (J == theGraph->V[I].fwdArcList)
                J = NIL;
        }
    }
}

/********************************************************************
 _SetVertexTypeInBicomp()
 ********************************************************************/

void _SetVertexTypeInBicomp(graphP theGraph, int BicompRoot, int theType)
{
int  V, J;

     sp_ClearStack(theGraph->theStack);
     sp_Push(theGraph->theStack, BicompRoot);
     while (sp_NonEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, V);
          theGraph->G[V].type = theType;

          J = gp_GetFirstArc(theGraph, V);
          while (gp_IsArc(theGraph, J))
          {
             if (theGraph->G[J].type == EDGE_DFSCHILD)
                 sp_Push(theGraph->theStack, theGraph->G[J].v);

             J = gp_GetNextArc(theGraph, J);
          }
     }
}

/********************************************************************
 _ClearGraph()
 Clears all memory used by the graph, restoring it to the state it
 was in immediately after gp_New() created it.
 ********************************************************************/

void _ClearGraph(graphP theGraph)
{
     if (theGraph->G != NULL)
     {
          free(theGraph->G);
          theGraph->G = NULL;
     }
     if (theGraph->V != NULL)
     {
          free(theGraph->V);
          theGraph->V = NULL;
     }

     theGraph->N = theGraph->M = theGraph->edgeOffset = 0;
     theGraph->internalFlags = theGraph->embedFlags = 0;

     _ClearIsolatorContext(theGraph);

     LCFree(&theGraph->BicompLists);
     LCFree(&theGraph->DFSChildLists);

     sp_Free(&theGraph->theStack);

     if (theGraph->buckets != NULL)
     {
         free(theGraph->buckets);
         theGraph->buckets = NULL;
     }

     LCFree(&theGraph->bin);

     if (theGraph->extFace != NULL)
     {
         free(theGraph->extFace);
         theGraph->extFace = NULL;
     }

     sp_Free(&theGraph->edgeHoles);

     gp_FreeExtensions(theGraph);
}

/********************************************************************
 gp_Free()
 Frees G and V, then the graph record.  Then sets your pointer to NULL
 (so you must pass the address of your pointer).
 ********************************************************************/

void gp_Free(graphP *pGraph)
{
     if (pGraph == NULL) return;
     if (*pGraph == NULL) return;

     _ClearGraph(*pGraph);

     free(*pGraph);
     *pGraph = NULL;
}

/********************************************************************
 gp_CopyGraph()
 Copies the content of the srcGraph into the dstGraph.  The dstGraph
 must have been previously initialized with the same number of
 vertices as the srcGraph (e.g. gp_InitGraph(dstGraph, srcGraph->N).

 Returns OK for success, NOTOK for failure.
 ********************************************************************/

int  gp_CopyGraph(graphP dstGraph, graphP srcGraph)
{
int  I, N = srcGraph->N, edgeOffset = srcGraph->edgeOffset;
int  Gsize = edgeOffset + 2*EDGE_LIMIT*N;

     /* Parameter checks */
     if (dstGraph == NULL || srcGraph == NULL)
         return NOTOK;

     if (dstGraph->N != srcGraph->N)
         return NOTOK;

     // Copy the basic GraphNode structures.  Augmentations to
     // the graph node structure created by extensions are copied
     // below by gp_CopyExtensions()
     for (I = 0; I < Gsize; I++)
          dstGraph->G[I] = srcGraph->G[I];

     // Copy the basic VertexRec structures.  Augmentations to
     // the vertex structure created by extensions are copied
     // below by gp_CopyExtensions()
     for (I = 0; I < N; I++)
          dstGraph->V[I] = srcGraph->V[I];

     // Copy the external face array
     for (I = 0; I < edgeOffset; I++)
     {
         dstGraph->extFace[I].vertex[0] = srcGraph->extFace[I].vertex[0];
         dstGraph->extFace[I].vertex[1] = srcGraph->extFace[I].vertex[1];
         dstGraph->extFace[I].inversionFlag = srcGraph->extFace[I].inversionFlag;
     }

     // Give the dstGraph the same size and instrinsic properties
     dstGraph->N = srcGraph->N;
     dstGraph->M = srcGraph->M;
     dstGraph->edgeOffset = srcGraph->edgeOffset;
     dstGraph->internalFlags = srcGraph->internalFlags;
     dstGraph->embedFlags = srcGraph->embedFlags;

     dstGraph->IC = srcGraph->IC;

     LCCopy(dstGraph->BicompLists, srcGraph->BicompLists);
     LCCopy(dstGraph->DFSChildLists, srcGraph->DFSChildLists);
     sp_Copy(dstGraph->theStack, srcGraph->theStack);
     sp_Copy(dstGraph->edgeHoles, srcGraph->edgeHoles);

     // Copy the graph's function table, which only has pointers to
     // the most recent extension overloads of each function (or
     // the original function pointer if a particular function has
     // not been overloaded).
     dstGraph->functions = srcGraph->functions;

     // Copy the set of extensions, which includes copying the
     // extension data as well as the function overload tables
     return gp_CopyExtensions(dstGraph, srcGraph);
}

/********************************************************************
 gp_DupGraph()
 ********************************************************************/

graphP gp_DupGraph(graphP theGraph)
{
graphP result;

     if ((result = gp_New()) == NULL) return NULL;

     if (gp_InitGraph(result, theGraph->N) != OK ||
         gp_CopyGraph(result, theGraph) != OK)
     {
         gp_Free(&result);
         return NULL;
     }

     return result;
}

/********************************************************************
 gp_CreateRandomGraph()

 Creates a randomly generated graph.  First a tree is created by
 connecting each vertex to some successor.  Then a random number of
 additional random edges are added.  If an edge already exists, then
 we retry until a non-existent edge is picked.

 This function assumes the caller has already called srand().
 ********************************************************************/

int  gp_CreateRandomGraph(graphP theGraph)
{
int N, I, M, u, v;

     N = theGraph->N;

/* Generate a random tree; note that this method virtually guarantees
        that the graph will be renumbered, but it is linear time.
        Also, we are not generating the DFS tree but rather a tree
        that simply ensures the resulting random graph is connected. */

     for (I=1; I < N; I++)
          if (gp_AddEdge(theGraph, _GetRandomNumber(0, I-1), 0, I, 0) != OK)
                return NOTOK;

/* Generate a random number of additional edges
        (actually, leave open a small chance that no
        additional edges will be added). */

     M = _GetRandomNumber(7*N/8, EDGE_LIMIT*N);

     if (M > N*(N-1)/2) M = N*(N-1)/2;

     for (I=N-1; I<M; I++)
     {
          u = _GetRandomNumber(0, N-2);
          v = _GetRandomNumber(u+1, N-1);

          if (gp_IsNeighbor(theGraph, u, v))
              I--;
          else
          {
              if (gp_AddEdge(theGraph, u, 0, v, 0) != OK)
                  return NOTOK;
          }
     }

     return OK;
}

/********************************************************************
 _GetRandomNumber()
 This function generates a random number between NMin and NMax
 inclusive.  It assumes that the caller has called srand().
 It calls rand(), but before truncating to the proper range,
 it adds the high bits of the rand() result into the low bits.
 The result of this is that the randomness appearing in the
 truncated bits also has an affect on the non-truncated bits.
 I used the same technique to improve the spread of hashing functions
 in my Jan.98 Dr. Dobb's Journal article  "Resizable Data Structures".
 ********************************************************************/

int  _GetRandomNumber(int NMin, int NMax)
{
int  N = rand();

     if (NMax < NMin) return NMin;

     N += ((N&0xFFFF0000)>>16);
     N += ((N&0x0000FF00)>>8);
     N %= (NMax-NMin+1);
     return N+NMin;
}

/********************************************************************
 _getUnprocessedChild()
 Support routine for gp_Create RandomGraphEx(), this function
 obtains a child of the given vertex in the randomly generated
 tree that has not yet been processed.  NIL is returned if the
 given vertex has no unprocessed children

 ********************************************************************/

int _getUnprocessedChild(graphP theGraph, int parent)
{
int J = gp_GetFirstArc(theGraph, parent);
int JTwin = gp_GetTwinArc(theGraph, J);
int child = theGraph->G[J].v;

    /* The tree edges were added to the beginning of the adjacency list,
        and we move processed tree edge records to the end of the list,
        so if the immediate next arc (edge record) is not a tree edge
        then we return NIL because the vertex has no remaining
        unprocessed children */

    if (theGraph->G[J].type == TYPE_UNKNOWN)
        return NIL;

    /* If the child has already been processed, then all children
        have been pushed to the end of the list, and we have just
        encountered the first child we processed, so there are no
        remaining unprocessed children */

    if (theGraph->G[J].visited)
        return NIL;

    /* We have found an edge leading to an unprocessed child, so
        we mark it as processed so that it doesn't get returned
        again in future iterations. */

    theGraph->G[J].visited = 1;
    theGraph->G[JTwin].visited = 1;

    /* Now we move the edge record in the parent vertex to the end
        of the adjacency list of that vertex. Of course, we need do
        nothing if the arc J is alone in the adjacency list, which
        is the case only when its next and previous arcs are equal */

    if (gp_GetNextArc(theGraph, J) != gp_GetPrevArc(theGraph, J))
    {
        theGraph->G[parent].link[0] = theGraph->G[J].link[0];
        theGraph->G[theGraph->G[J].link[0]].link[1] = parent;
        theGraph->G[J].link[0] = parent;
        theGraph->G[J].link[1] = theGraph->G[parent].link[1];
        theGraph->G[theGraph->G[parent].link[1]].link[0] = J;
        theGraph->G[parent].link[1] = J;
    }

    /* Now we move the edge record in the child vertex to the
        end of the adjacency list of the child. */

    if (gp_GetNextArc(theGraph, JTwin) != gp_GetPrevArc(theGraph, JTwin))
    {
        theGraph->G[theGraph->G[JTwin].link[0]].link[1] = theGraph->G[JTwin].link[1];
        theGraph->G[theGraph->G[JTwin].link[1]].link[0] = theGraph->G[JTwin].link[0];
        theGraph->G[JTwin].link[0] = child;
        theGraph->G[JTwin].link[1] = theGraph->G[child].link[1];
        theGraph->G[theGraph->G[child].link[1]].link[0] = JTwin;
        theGraph->G[child].link[1] = JTwin;
    }

    /* Now we set the child's parent and return the child. */

    theGraph->V[child].DFSParent = parent;

    return child;
}

/********************************************************************
 _hasUnprocessedChild()
 Support routine for gp_Create RandomGraphEx(), this function
 obtains a child of the given vertex in the randomly generated
 tree that has not yet been processed.  False (0) is returned
 unless the given vertex has an unprocessed child.
 ********************************************************************/

int _hasUnprocessedChild(graphP theGraph, int parent)
{
int J = gp_GetFirstArc(theGraph, parent);

    if (theGraph->G[J].type == TYPE_UNKNOWN)
        return 0;

    if (theGraph->G[J].visited)
        return 0;

    return 1;
}

/********************************************************************
 gp_CreateRandomGraphEx()
 Given a graph structure with a pre-specified number of vertices N,
 this function creates a graph with the specified number of edges.

 If numEdges <= 3N-6, then the graph generated is planar.  If
 numEdges is larger, then a maximal planar graph is generated, then
 (numEdges - 3N + 6) additional random edges are added.

 This function assumes the caller has already called srand().
 ********************************************************************/

int  gp_CreateRandomGraphEx(graphP theGraph, int numEdges)
{
#define EDGE_TREE_RANDOMGEN (TYPE_UNKNOWN+1)

int N, I, arc, M, root, v, c, p, last, u, J, e;

     N = theGraph->N;

     if (numEdges > EDGE_LIMIT * N)
         numEdges = EDGE_LIMIT * N;

/* Generate a random tree. */

    for (I=1; I < N; I++)
    {
        v = _GetRandomNumber(0, I-1);
        if (gp_AddEdge(theGraph, v, 0, I, 0) != OK)
            return NOTOK;

        else
	    {
            arc = theGraph->edgeOffset + 2*theGraph->M - 2;
		    theGraph->G[arc].type = EDGE_TREE_RANDOMGEN;
		    theGraph->G[gp_GetTwinArc(theGraph, arc)].type = EDGE_TREE_RANDOMGEN;
		    theGraph->G[arc].visited = 0;
		    theGraph->G[gp_GetTwinArc(theGraph, arc)].visited = 0;
	    }
    }

/* Add edges up to the limit or until the graph is maximal planar. */

    M = numEdges <= 3*N - 6 ? numEdges : 3*N - 6;

    root = 0;
    v = last = _getUnprocessedChild(theGraph, root);

    while (v != root && theGraph->M < M)
    {
	     c = _getUnprocessedChild(theGraph, v);

	     if (c != NIL)
	     {
             if (last != v)
             {
		         if (gp_AddEdge(theGraph, last, 1, c, 1) != OK)
			         return NOTOK;
             }

		     if (gp_AddEdge(theGraph, root, 1, c, 1) != OK)
			     return NOTOK;

		     v = last = c;
	     }

	     else
	     {
		     p = theGraph->V[v].DFSParent;
		     while (p != NIL && (c = _getUnprocessedChild(theGraph, p)) == NIL)
		     {
			     v = p;
			     p = theGraph->V[v].DFSParent;
			     if (p != NIL && p != root)
			     {
				     if (gp_AddEdge(theGraph, last, 1, p, 1) != OK)
					     return NOTOK;
			     }
		     }

		     if (p != NIL)
		     {
                 if (p == root)
                 {
                     if (gp_AddEdge(theGraph, v, 1, c, 1) != OK)
				         return NOTOK;

                     if (v != last)
                     {
			             if (gp_AddEdge(theGraph, last, 1, c, 1) != OK)
				             return NOTOK;
                     }
                 }
                 else
                 {
			         if (gp_AddEdge(theGraph, last, 1, c, 1) != OK)
				         return NOTOK;
                 }

                 if (p != root)
                 {
			        if (gp_AddEdge(theGraph, root, 1, c, 1) != OK)
				         return NOTOK;
                    last = c;
                 }

			     v = c;
		     }
	     }
    }

/* Add additional edges if the limit has not yet been reached. */

    while (theGraph->M < numEdges)
    {
        u = _GetRandomNumber(0, N-1);
        v = _GetRandomNumber(0, N-1);

        if (u != v && !gp_IsNeighbor(theGraph, u, v))
            if (gp_AddEdge(theGraph, u, 0, v, 0) != OK)
                return NOTOK;
    }

/* Clear the edge types back to 'unknown' */

    for (e = 0; e < numEdges; e++)
    {
        J = theGraph->edgeOffset + 2*e;
        theGraph->G[J].type = TYPE_UNKNOWN;
        theGraph->G[gp_GetTwinArc(theGraph, J)].type = TYPE_UNKNOWN;
        theGraph->G[J].visited = 0;
        theGraph->G[gp_GetTwinArc(theGraph, J)].visited = 0;
    }

/* Put all DFSParent indicators back to NIL */

    for (I = 0; I < N; I++)
        theGraph->V[I].DFSParent = NIL;

    return OK;

#undef EDGE_TREE_RANDOMGEN
}

/********************************************************************
 gp_SetDirection()
 Behavior depends on edgeFlag_Direction (EDGEFLAG_DIRECTION_INONLY,
 EDGEFLAG_DIRECTION_OUTONLY, or 0).
 A direction of 0 clears directedness. Otherwise, edge record e is set
 to edgeFlag_Direction and e's twin arc is set to the opposing setting.
 ********************************************************************/

void gp_SetDirection(graphP theGraph, int e, int edgeFlag_Direction)
{
	int eTwin = gp_GetTwinArc(theGraph, e);

	if (edgeFlag_Direction == EDGEFLAG_DIRECTION_INONLY)
	{
		theGraph->G[e].flags |= EDGEFLAG_DIRECTION_INONLY;
		theGraph->G[eTwin].flags |= EDGEFLAG_DIRECTION_OUTONLY;
	}
	else if (edgeFlag_Direction == EDGEFLAG_DIRECTION_OUTONLY)
	{
		theGraph->G[e].flags |= EDGEFLAG_DIRECTION_OUTONLY;
		theGraph->G[eTwin].flags |= EDGEFLAG_DIRECTION_INONLY;
	}
	else
	{
		theGraph->G[e].flags &= ~(EDGEFLAG_DIRECTION_INONLY|EDGEFLAG_DIRECTION_OUTONLY);
		theGraph->G[eTwin].flags &= ~(EDGEFLAG_DIRECTION_INONLY|EDGEFLAG_DIRECTION_OUTONLY);
	}
}

/********************************************************************
 gp_IsNeighbor()

 Checks whether v is already in u's adjacency list, i.e. does the arc
 u -> v exist.
 If there is an edge record for v in u's list, but it is marked INONLY,
 then it represents the arc v->u but not u->v, so it is ignored.

 Returns 1 for yes, 0 for no.
 ********************************************************************/

int  gp_IsNeighbor(graphP theGraph, int u, int v)
{
int  J;

     J = gp_GetFirstArc(theGraph, u);
     while (gp_IsArc(theGraph, J))
     {
          if (theGraph->G[J].v == v)
          {
              if (!gp_GetDirection(theGraph, J, EDGEFLAG_DIRECTION_INONLY))
            	  return 1;
          }
          J = gp_GetNextArc(theGraph, J);
     }
     return 0;
}

/********************************************************************
 gp_GetNeighborEdgeRecord()
 Searches the adjacency list of u to obtains the edge record for v.

 NOTE: The caller should check whether the edge record is INONLY;
       This method returns any edge record representing a connection
       between vertices u and v, so this method can return an
       edge record even if gp_IsNeighbor(theGraph, u, v) is false (0).
       To filter out INONLY edge records, use gp_GetDirection() on
       the edge record returned by this method.

 Returns NIL if there is no edge record indicating v in u's adjacency
         list, or the edge record location otherwise.
 ********************************************************************/

int  gp_GetNeighborEdgeRecord(graphP theGraph, int u, int v)
{
int  J;

     J = gp_GetFirstArc(theGraph, u);
     while (gp_IsArc(theGraph, J))
     {
          if (theGraph->G[J].v == v) return J;
          J = gp_GetNextArc(theGraph, J);
     }
     return NIL;
}

/********************************************************************
 gp_GetVertexDegree()

 Counts the number of edge records in the adjacency list of a given
 vertex V.  The while loop condition is 2N or higher because our
 data structure keeps records at locations 0 to N-1 for vertices
 AND N to 2N-1 for copies of vertices.  So edge records are stored
 at locations 2N and above.

 Note: For digraphs, this method returns the total degree of the
       vertex, including outward arcs (undirected and OUTONLY)
       as well as INONLY arcs.  Other functions are defined to get
       the in-degree or out-degree of the vertex.

 Note: This function determines the degree by counting.  An extension
       could cache the degree value of each vertex and update the
       cached value as edges are added and deleted.
 ********************************************************************/

int  gp_GetVertexDegree(graphP theGraph, int v)
{
int  J, degree;

     if (theGraph==NULL || v==NIL) return 0;

     degree = 0;

     J = gp_GetFirstArc(theGraph, v);
     while (gp_IsArc(theGraph, J))
     {
         degree++;
         J = gp_GetNextArc(theGraph, J);
     }

     return degree;
}

/********************************************************************
 gp_GetVertexInDegree()

 Counts the number of edge records in the adjacency list of a given
 vertex V that represent arcs from another vertex into V.
 This includes undirected edges and INONLY arcs, so it only excludes
 edges records that are marked as OUTONLY arcs.

 Note: This function determines the in-degree by counting.  An extension
       could cache the in-degree value of each vertex and update the
       cached value as edges are added and deleted.
 ********************************************************************/

int  gp_GetVertexInDegree(graphP theGraph, int v)
{
int  J, degree;

     if (theGraph==NULL || v==NIL) return 0;

     degree = 0;

     J = gp_GetFirstArc(theGraph, v);
     while (gp_IsArc(theGraph, J))
     {
         if (!gp_GetDirection(theGraph, J, EDGEFLAG_DIRECTION_OUTONLY))
             degree++;
         J = gp_GetNextArc(theGraph, J);
     }

     return degree;
}

/********************************************************************
 gp_GetVertexOutDegree()

 Counts the number of edge records in the adjacency list of a given
 vertex V that represent arcs from V to another vertex.
 This includes undirected edges and OUTONLY arcs, so it only excludes
 edges records that are marked as INONLY arcs.

 Note: This function determines the out-degree by counting.  An extension
       could cache the out-degree value of each vertex and update the
       cached value as edges are added and deleted.
 ********************************************************************/

int  gp_GetVertexOutDegree(graphP theGraph, int v)
{
int  J, degree;

     if (theGraph==NULL || v==NIL) return 0;

     degree = 0;

     J = gp_GetFirstArc(theGraph, v);
     while (gp_IsArc(theGraph, J))
     {
         if (!gp_GetDirection(theGraph, J, EDGEFLAG_DIRECTION_INONLY))
             degree++;
         J = gp_GetNextArc(theGraph, J);
     }

     return degree;
}

/********************************************************************
 _AddArc()
 This routine adds arc (u,v) to u's edge list, storing the record for
 v at position arcPos.  The record is added to the beginning of u's
 adjacency list if the link parameter is 0 and to the end if it is 1.
 The use of exclusive-or (i.e. 1^link) is simply to get
 the other link (if link is 0 then 1^link is 1, and vice versa).
 ********************************************************************/

void _AddArc(graphP theGraph, int u, int v, int arcPos, int link)
{
     theGraph->G[arcPos].v = v;

     if (gp_IsArc(theGraph, gp_GetFirstArc(theGraph, u)))
     {
    	 if (link == 0)
    	 {
    		 gp_SetNextArc(theGraph, arcPos, gp_GetFirstArc(theGraph, u));
    		 gp_SetPrevArc(theGraph, gp_GetFirstArc(theGraph, u), arcPos);
    		 gp_AttachFirstArc(theGraph, u, arcPos);
    	 }
    	 else
    	 {
    		 gp_SetPrevArc(theGraph, arcPos, gp_GetLastArc(theGraph, u));
    		 gp_SetNextArc(theGraph, gp_GetLastArc(theGraph, u), arcPos);
    		 gp_AttachLastArc(theGraph, u, arcPos);
    	 }
     }
     else
     {
		 gp_AttachLastArc(theGraph, u, arcPos);
		 gp_AttachFirstArc(theGraph, u, arcPos);
     }
}

/********************************************************************
 gp_AddEdge()
 Adds the undirected edge (u,v) to the graph by placing edge records
 representing u into v's circular edge record list and v into u's
 circular edge record list.

 upos receives the location in G where the u record in v's list will be
        placed, and vpos is the location in G of the v record we placed in
 u's list.  These are used to initialize the short circuit links.

 ulink (0|1) indicates whether the edge record to v in u's list should
        become adjacent to u by its 0 or 1 link, i.e. u[ulink] == vpos.
 vlink (0|1) indicates whether the edge record to u in v's list should
        become adjacent to v by its 0 or 1 link, i.e. v[vlink] == upos.

 ********************************************************************/

int  gp_AddEdge(graphP theGraph, int u, int ulink, int v, int vlink)
{
int  upos, vpos;

     if (theGraph==NULL || u<0 || v<0 ||
         u>=theGraph->edgeOffset || v>=theGraph->edgeOffset)
         return NOTOK;

     /* We enforce the edge limit */

     if (theGraph->M >= EDGE_LIMIT*theGraph->N)
         return NONEMBEDDABLE;

     if (sp_NonEmpty(theGraph->edgeHoles))
         sp_Pop(theGraph->edgeHoles, vpos);
     else
         vpos = theGraph->edgeOffset + 2*theGraph->M;

     upos = gp_GetTwinArc(theGraph, vpos);

     _AddArc(theGraph, u, v, vpos, ulink);
     _AddArc(theGraph, v, u, upos, vlink);

     theGraph->M++;
     return OK;
}

/********************************************************************
 _AddInternalArc()
 This routine adds arc (u,v) to u's edge list, storing the record for
 v at position arcPos.  The record is added between the edge records
 e_u0 and e_u1.
 ********************************************************************/

void _AddInternalArc(graphP theGraph, int u, int e_u0, int e_u1, int v, int arcPos)
{
int e_u0_link, e_u1_link;

     theGraph->G[arcPos].v = v;

     e_u0_link = theGraph->G[e_u0].link[0] == e_u1 ? 0 : 1;
     e_u1_link = 1^e_u0_link;

     theGraph->G[e_u0].link[e_u0_link] = arcPos;
     theGraph->G[e_u1].link[e_u1_link] = arcPos;

     theGraph->G[arcPos].link[1^e_u0_link] = e_u0;
     theGraph->G[arcPos].link[1^e_u1_link] = e_u1;
}

/********************************************************************
 gp_AddInternalEdge()

 This function adds the edge (u, v) such that the edge record added
 to the adjacency list of u is between records e_u0 and e_u1 and
 the edge record added to the adjacency list of v is between records
 e_v0 and e_v1.  Note that each of e_u0, e_u1, e_v0 and e_v1 could
 indicate an edge record or a vertex record.
 ********************************************************************/

int  gp_AddInternalEdge(graphP theGraph, int u, int e_u0, int e_u1,
                                         int v, int e_v0, int e_v1)
{
int vertMax = theGraph->edgeOffset - 1,
    edgeMax = vertMax + 2*theGraph->M,
    upos, vpos;

     edgeMax += 2*sp_GetCurrentSize(theGraph->edgeHoles);

     if (theGraph==NULL || u<0 || v<0 || u>vertMax || v>vertMax ||
         e_u0<0 || e_u0>edgeMax || e_u1<0 || e_u1>edgeMax ||
         e_v0<0 || e_v0>edgeMax || e_v1<0 || e_v1>edgeMax)
         return NOTOK;

     if (gp_GetNextArc(theGraph, e_u0) != e_u1 && gp_GetPrevArc(theGraph, e_u0) != e_u1)
         return NOTOK;

     if (gp_GetNextArc(theGraph, e_u1) != e_u0 && gp_GetPrevArc(theGraph, e_u1) != e_u0)
         return NOTOK;

     if (gp_GetNextArc(theGraph, e_v0) != e_v1 && gp_GetPrevArc(theGraph, e_v0) != e_v1)
         return NOTOK;

     if (gp_GetNextArc(theGraph, e_v1) != e_v0 && gp_GetPrevArc(theGraph, e_v1) != e_v0)
         return NOTOK;

     if (theGraph->M >= EDGE_LIMIT*theGraph->N)
         return NONEMBEDDABLE;

     if (sp_NonEmpty(theGraph->edgeHoles))
         sp_Pop(theGraph->edgeHoles, vpos);
     else
         vpos = theGraph->edgeOffset + 2*theGraph->M;

     upos = gp_GetTwinArc(theGraph, vpos);

     _AddInternalArc(theGraph, u, e_u0, e_u1, v, upos);
     _AddInternalArc(theGraph, v, e_v0, e_v1, u, vpos);

     theGraph->M++;

     return OK;
}

/********************************************************************
 _HideArc()
 This routine removes an arc from an edge list, but does not delete
 it from the data structure.  Many algorithms must temporarily remove
 an edge, perform some calculation, and eventually put the edge back.
 This routine supports that operation.

 The neighboring adjacency list nodes are cross-linked, but the two
 link members of the arc are retained so it can reinsert itself when
 _RestoreArc() is called.
 ********************************************************************/

void _HideArc(graphP theGraph, int arc)
{
int nextArc = gp_GetNextArc(theGraph, arc),
    prevArc = gp_GetPrevArc(theGraph, arc);

    if (nextArc != NIL)
        theGraph->G[nextArc].link[1] = prevArc;

    if (prevArc != NIL)
        theGraph->G[prevArc].link[0] = nextArc;
}

/********************************************************************
 _RestoreArc()
 This routine reinserts an arc into the edge list from which it
 was previously removed by _HideArc().

 The assumed processing model is that arcs will be restored in reverse
 of the order in which they were hidden, i.e. it is assumed that the
 hidden arcs will be pushed on a stack and the arcs will be popped
 from the stack for restoration.
 ********************************************************************/

void _RestoreArc(graphP theGraph, int arc)
{
int nextArc = gp_GetNextArc(theGraph, arc),
	prevArc = gp_GetPrevArc(theGraph, arc);

	if (nextArc != NIL)
		theGraph->G[nextArc].link[1] = arc;

	if (prevArc != NIL)
		theGraph->G[prevArc].link[0] = arc;
}

/********************************************************************
 gp_HideEdge()
 This routine removes an arc and its twin arc from its edge list,
 but does not delete them from the data structure.  Many algorithms must
 temporarily remove an edge, perform some calculation, and eventually
 put the edge back. This routine supports that operation.

 For each arc, the neighboring adjacency list nodes are cross-linked,
 but the links in the arc are retained because they indicate the
 neighbor arcs to which the arc can be reattached by gp_RestoreEdge().
 ********************************************************************/

void gp_HideEdge(graphP theGraph, int arcPos)
{
     _HideArc(theGraph, arcPos);
     _HideArc(theGraph, gp_GetTwinArc(theGraph, arcPos));
}

/********************************************************************
 gp_RestoreEdge()
 This routine reinserts an arc and its twin arc into the edge list
 from which it was previously removed by gp_HideEdge().

 The assumed processing model is that edges will be restored in
 reverse of the order in which they were hidden, i.e. it is assumed
 that the hidden edges will be pushed on a stack and the edges will
 be popped from the stack for restoration.

 Note: Since both arcs of an edge are restored, only one arc need
        be pushed on the stack for restoration.  This routine
        restores the two arcs in the opposite order from the order
        in which they are hidden by gp_HideEdge().
 ********************************************************************/

void gp_RestoreEdge(graphP theGraph, int arcPos)
{
     _RestoreArc(theGraph, gp_GetTwinArc(theGraph, arcPos));
     _RestoreArc(theGraph, arcPos);
}

/****************************************************************************
 gp_DeleteEdge()

 This function deletes the given edge record J and its twin, reducing the
 number of edges M in the graph.
 Before the Jth record is deleted, its 'nextLink' adjacency list neighbor
 is collected as the return result.  This is useful when iterating through
 an edge list and making deletions because the nextLink arc is the 'next'
 arc in the iteration, but it is hard to obtain *after* deleting arc J.
 ****************************************************************************/

int  gp_DeleteEdge(graphP theGraph, int J, int nextLink)
{
int  JTwin = gp_GetTwinArc(theGraph, J);
int  M = theGraph->M;
int  nextArc, JPos, MPos;

/* Calculate the nextArc after J so that, when J is deleted, the return result
        informs a calling loop of the next edge to be processed. */

     nextArc = gp_GetAdjacentArc(theGraph, J, nextLink);

/* Delete the edge records J and JTwin. */

     theGraph->G[theGraph->G[J].link[0]].link[1] = theGraph->G[J].link[1];
     theGraph->G[theGraph->G[J].link[1]].link[0] = theGraph->G[J].link[0];
     theGraph->G[theGraph->G[JTwin].link[0]].link[1] = theGraph->G[JTwin].link[1];
     theGraph->G[theGraph->G[JTwin].link[1]].link[0] = theGraph->G[JTwin].link[0];

/* Clear the edge record contents */

    theGraph->functions.fpInitGraphNode(theGraph, J);
    theGraph->functions.fpInitGraphNode(theGraph, JTwin);

/* If records J and JTwin are not the last in the edge record array, then
     we want to record a new hole in the edge array. */

     JPos = (J < JTwin ? J : JTwin);
     MPos = theGraph->edgeOffset + 2*(M-1) + 2*sp_GetCurrentSize(theGraph->edgeHoles);

     if (JPos < MPos)
     {
         sp_Push(theGraph->edgeHoles, JPos);
     }

/* Now we reduce the number of edges in the data structure, and then
        return the previously calculated successor of J. */

     theGraph->M--;
     return nextArc;
}

/********************************************************************
 _HideInternalEdges()
 Pushes onto the graph's stack and hides all arc nodes of the vertex
 except the first and last arcs in the adjacency list of the vertex.
 This method is typically called on a vertex that is on the external
 face of a biconnected component, because the first and last arcs are
 the ones that attach the vertex to the external face cycle, and any
 other arcs in the adjacency list are inside that cycle.
 ********************************************************************/

void _HideInternalEdges(graphP theGraph, int vertex)
{
int J = gp_GetFirstArc(theGraph, vertex);

    // If the vertex adjacency list is empty or if it contains
    // only one edge, then there are no *internal* edges to hide
    if (J == gp_GetLastArc(theGraph, vertex))
    	return;

    // Start with the first internal edge
    J = gp_GetNextArc(theGraph, J);

    sp_ClearStack(theGraph->theStack);

    while (J != gp_GetLastArc(theGraph, vertex))
    {
        sp_Push(theGraph->theStack, J);
        gp_HideEdge(theGraph, J);
        J = gp_GetNextArc(theGraph, J);
    }
}

/********************************************************************
 _RestoreInternalEdges()
 Reverses the effects of _HideInternalEdges()
 ********************************************************************/

void _RestoreInternalEdges(graphP theGraph)
{
int  e;

     while (!sp_IsEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, e);
          if (gp_IsArc(theGraph, e))
              gp_RestoreEdge(theGraph, e);
     }
}

/********************************************************************
 _DeleteUnmarkedEdgesInBicomp()

 This function deletes from a given biconnected component all edges
 whose visited member is zero.
 ********************************************************************/

void _DeleteUnmarkedEdgesInBicomp(graphP theGraph, int BicompRoot)
{
int  V, J;

     sp_ClearStack(theGraph->theStack);
     sp_Push(theGraph->theStack, BicompRoot);
     while (!sp_IsEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, V);

          J = gp_GetFirstArc(theGraph, V);
          while (gp_IsArc(theGraph, J))
          {
             if (theGraph->G[J].type == EDGE_DFSCHILD)
                 sp_Push(theGraph->theStack, theGraph->G[J].v);

             if (!theGraph->G[J].visited)
                  J = gp_DeleteEdge(theGraph, J, 0);
             else J = gp_GetNextArc(theGraph, J);
          }
     }
}

/********************************************************************
 _ClearInvertedFlagsInBicomp()

 This function clears the inverted flag markers on any edges in a
 given biconnected component.
 ********************************************************************/

void _ClearInvertedFlagsInBicomp(graphP theGraph, int BicompRoot)
{
int  V, J;

     sp_ClearStack(theGraph->theStack);
     sp_Push(theGraph->theStack, BicompRoot);
     while (!sp_IsEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, V);

          J = gp_GetFirstArc(theGraph, V);
          while (gp_IsArc(theGraph, J))
          {
             if (theGraph->G[J].type == EDGE_DFSCHILD)
             {
                 sp_Push(theGraph->theStack, theGraph->G[J].v);
                 CLEAR_EDGEFLAG_INVERTED(theGraph, J);
             }

             J = gp_GetNextArc(theGraph, J);
          }
     }
}

/********************************************************************
 _GetBicompSize()
 ********************************************************************/

int  _GetBicompSize(graphP theGraph, int BicompRoot)
{
int  V, J;
int  theSize = 0;

     sp_ClearStack(theGraph->theStack);
     sp_Push(theGraph->theStack, BicompRoot);
     while (!sp_IsEmpty(theGraph->theStack))
     {
          sp_Pop(theGraph->theStack, V);
          theSize++;
          J = gp_GetFirstArc(theGraph, V);
          while (gp_IsArc(theGraph, J))
          {
             if (theGraph->G[J].type == EDGE_DFSCHILD)
                 sp_Push(theGraph->theStack, theGraph->G[J].v);

             J = gp_GetNextArc(theGraph, J);
          }
     }
     return theSize;
}
