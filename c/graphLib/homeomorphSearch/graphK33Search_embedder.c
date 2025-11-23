/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

// Needed due to use of malloc() and free()
#include <stdlib.h>

#include "graphK33Search.h"
#include "graphK33Search.private.h"

// The embedding obstruction tree implementation must be able to get the
// K3,3 Search extension context from the subgraph of an EONode
extern int K33SEARCH_ID;

#include "../graph.h"

#ifdef INCLUDE_K33SEARCH_EMBEDDER

/* Imported functions */

extern int _ClearVisitedFlagsInBicomp(graphP theGraph, int BicompRoot);
extern int _MarkHighestXYPath(graphP theGraph);
extern int _MarkLowestXYPath(graphP theGraph);
extern int _CheckEmbeddingFacialIntegrity(graphP theGraph);
extern int _getImageVertices(graphP theGraph, int *degrees, int maxDegree,
                             int *imageVerts, int maxNumImageVerts);
extern int _TestForCompleteGraphObstruction(graphP theGraph, int numVerts,
                                            int *degrees, int *imageVerts);
extern int _TestSubgraph(graphP theSubgraph, graphP theGraph);
extern void _ClearVertexVisitedFlags(graphP theGraph, int includeVirtualVertices);

/* Private functions for K_{3,3}-free graph embedding. */

K33Search_EONodeP _K33Search_EONode_New(int theEOType, graphP theSubgraph, int theSubgraphOwner);
void _K33Search_EONode_Free(K33Search_EONodeP *pEONode);
int _K33Search_TestForEOTreeChildren(K33Search_EONodeP EOTreeNode);

int _K33Search_EONode_NewONode(graphP theGraph, K33Search_EONodeP *pNewONode);

int _K33Search_ExtractEmbeddingSubgraphs(graphP theGraph, int R, K33Search_EONodeP newONode);

int _K33Search_ExtractExternaFaceBridgeSet(graphP theGraph, int R, int poleVertex, int equatorVertex, K33Search_EONodeP newONode);
int _K33Search_ExtractXYBridgeSet(graphP theGraph, int R, K33Search_EONodeP newONode);
int _K33Search_ExtractVWBridgeSet(graphP theGraph, int R, K33Search_EONodeP newONode);

int _K33Search_ExtractEmbeddedBridgeSet(graphP theGraph, int R, int cutv1, int cutv2, K33Search_EONodeP newONode,
                                        int firstStartingEdge, int linkDir, int lastStartingEdge);
int _K33Search_MarkBridgeSetToExtract(graphP theGraph, int R, int cutv1, int cutv2,
                                      int firstStartingEdge, int linkDir, int lastStartingEdge);
int _K33Search_ExtractBridgeSet(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet);
int _K33Search_MakeGraphSubgraphVertexMaps(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet);
int _K33Search_CopyEdgesToNewSubgraph(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet);
int _K33Search_PlanarizeNewSubgraph(graphP theNewSubgraph);

int _K33Search_AttachONodeAsChildOfRoot(graphP theGraph, K33Search_EONodeP newONode);
int _K33Search_AttachENodeAsChildOfONode(K33Search_EONodeP theENode, int cutv1, int cutv2, K33Search_EONodeP theONode);

// When these methods are promoted to graphUtils.c, then add extern to front of these header declarations.

int _CountVerticesAndEdgesInBicomp(graphP theGraph, int BicompRoot, int visitedOnly, int *pNumVisitedVertices, int *pNumVisitedEdges);

int _DeactivateBicomp(graphP theGraph, int R);
int _DeactivatePertinentOnlySubtrees(graphP theGraph, int w);

int _CountUnembeddedEdgesInPertinentOnlySubtrees(graphP theGraph, int w, int *pNumEdgesInSubgraph);
int _CountVerticesAndEdgesInPertinentOnlySubtrees(graphP theGraph, int w,
                                                  int *pNumVerticesInSubgraph, int *pNumEdgesInSubgraph);

// End of methods to promote to graphUtils.c

int _K33Search_MapVerticesInPertinentOnlySubtrees(graphP theGraph, K33SearchContext *context, int w, int *pNextSubgraphVertexIndex);
int _K33Search_MapVerticesInBicomp(graphP theGraph, K33SearchContext *context, int R, int *pNextSubgraphVertexIndex);
int _K33Search_AddUnembeddedEdgesToSubgraph(graphP theGraph, K33SearchContext *context, int w, graphP newSubgraphForBridgeSet);
int _K33Search_AddNewEdgeToSubgraph(graphP theGraph, K33SearchContext *context, int v, int w, graphP newSubgraphForBridgeSet);
int _K33Search_CopyEdgesFromPertinentOnlySubtrees(graphP theGraph, K33SearchContext *context, int w, graphP newSubgraphForBridgeSet);
int _K33Search_CopyEdgesFromBicomp(graphP theGraph, K33SearchContext *context, int R, graphP newSubgraphForBridgeSet);
int _K33Search_CopyEdgeToNewSubgraph(graphP theGraph, K33SearchContext *context, int e, graphP newSubgraphForBridgeSet);

// For validation with the data structure
int _K33Search_AssembleMainPlanarEmbedding(K33Search_EONodeP EOTreeRoot);

int _K33Search_ValidateEmbeddingObstructionTree(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph);
int _K33Search_ValidateENodeSubtree(K33Search_EONodeP theRootENode);
int _K33Search_ValidateONodeSubtree(K33Search_EONodeP theRootONode);
int _K33Search_ValidateChildENodeConnection(graphP theK5, int e, K33Search_EONodeP theChildENode);

int _K33Search_ValidateEmbeddingObstructionTreeEdgeSet(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph);
int _K33Search_CopyEmbeddingEdgesToGraph(graphP theMainGraph, K33Search_EONodeP EONode, graphP graphOfEmbedding);

int _K33Search_ValidateEmbeddingObstructionTreeVertexSet(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph);
int _K33Search_ValidateVerticesInENodeSubtree(graphP theGraph, K33Search_EONodeP theRootENode, int ignoreCutVertices, graphP origGraph);
int _K33Search_ValidateVerticesInONodeSubtree(graphP theGraph, K33Search_EONodeP theRootENode, graphP origGraph);

/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/

/********************************************************************
 K33Search_EONode_New()
 ********************************************************************/
K33Search_EONodeP _K33Search_EONode_New(int theEOType, graphP theSubgraph, int theSubgraphOwner)
{
    K33Search_EONodeP theNewEONode = (K33Search_EONodeP)malloc(sizeof(K33Search_EONode));

    if (theNewEONode == NULL)
        return NULL;

    theNewEONode->EOType = theEOType;
    theNewEONode->subgraph = theSubgraph;
    theNewEONode->subgraphOwner = theSubgraphOwner;
    theNewEONode->visited = FALSE;

    return theNewEONode;
}

/********************************************************************
 K33_EONode_Free()
********************************************************************/
void _K33Search_EONode_Free(K33Search_EONodeP *pEONode)
{
    if (pEONode != NULL && *pEONode != NULL)
    {
        // Loop through the K33_EdgeRecs to find the edgerec pairs with non-NULL EONode pointers,
        // which are the children of the EONode being freed. Virtual edges aren't used to point
        // to the parent because we only ever need to traverse downward from root to descendants.
        // Call _K33_EONode_Free() on them (once per pair), which will recurse all the way down the tree
        if ((*pEONode)->subgraph != NULL)
        {
            graphP theGraph = (*pEONode)->subgraph;
            K33SearchContext *context = NULL;

            gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
            if (context != NULL)
            {
                int Esize, e;
                Esize = gp_EdgeIndexBound(theGraph);
                for (e = gp_GetFirstEdge(theGraph); e < Esize; e++)
                {
                    if (e & 1)
                    {
                        // Odd positions are after preceding even positions, which are freed below
                        context->E[e].EONode = NULL;
                    }
                    else
                    {
                        // Frees node (if it exists) and sets pointer to NULL
                        _K33Search_EONode_Free(&context->E[e].EONode);
                    }
                }
            }
        }

        if ((*pEONode)->subgraphOwner)
        {
            gp_Free(&(*pEONode)->subgraph);
            (*pEONode)->subgraphOwner = FALSE;
        }
        else
        {
            (*pEONode)->subgraph = NULL;
        }

        free(*pEONode);
        *pEONode = NULL;
    }
}

/********************************************************************
 _K33Search_TestForEOTreeChildren()

 Method to determine whether there are any embedding obstruction tree children of a given tree or subtree root
********************************************************************/
int _K33Search_TestForEOTreeChildren(K33Search_EONodeP EOTreeNode)
{
    graphP theSubgraph = EOTreeNode->subgraph;
    K33SearchContext *context = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&context);
    if (context != NULL)
    {
        int EsizeOccupied, e;

        EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
        for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
        {
            if (context->E[e].EONode != NULL)
                return TRUE;
        }
    }

    return FALSE;
}

/********************************************************************
 _K33Search_EONode_NewONode()

 Makes a new orphan O-node to represent the essential K5 of a
 K5 homeomorph that has been discovered and which does not have
 any straddling bridges along any of its pairs of fundamental paths.
 Subsequent code will attach subgraphs of the input graph, via E-nodes,
 to this O-node.
 ********************************************************************/
int _K33Search_EONode_NewONode(graphP theGraph, K33Search_EONodeP *pNewONode)
{
    K33Search_EONodeP theNewONode = NULL;
    graphP theNewK5Graph = NULL;
    isolatorContextP IC = &theGraph->IC;
    int v, w, e;

    // Clear the output variable
    *pNewONode = NULL;

    // We need a new graph to represent the K5, and we need to attach a  K33 extension instance
    // so the new graph can store the child E-nodes of the new O-node in its extended edges.
    if ((theNewK5Graph = gp_New()) == NULL)
        return NOTOK;

    if (gp_InitGraph(theNewK5Graph, 5) != OK ||
        gp_AttachK33Search(theNewK5Graph) != OK)
    {
        gp_Free(&theNewK5Graph);
        return NOTOK;
    }

    // We need to tell theNewK5Graph that its 5 vertices represent u_max, v, w, x and y
    // from the original graph.
    theNewK5Graph->V[gp_GetFirstVertex(theNewK5Graph)].index = MAX3(IC->ux, IC->uy, IC->uz);
    theNewK5Graph->V[gp_GetFirstVertex(theNewK5Graph) + 1].index = IC->v;
    theNewK5Graph->V[gp_GetFirstVertex(theNewK5Graph) + 2].index = IC->w;
    theNewK5Graph->V[gp_GetFirstVertex(theNewK5Graph) + 3].index = IC->x;
    theNewK5Graph->V[gp_GetFirstVertex(theNewK5Graph) + 4].index = IC->y;

    // Need to add all 10 edges to theNewK5Graph so that it stores an actual K5
    for (v = gp_GetFirstVertex(theNewK5Graph); gp_VertexInRange(theNewK5Graph, v); v++)
        for (w = v + 1; gp_VertexInRange(theNewK5Graph, w); w++)
            if (gp_AddEdge(theNewK5Graph, v, 0, w, 0) != OK ||
                gp_GetNeighbor(theNewK5Graph, e = theNewK5Graph->V[v].link[0]) != w)
            {
                gp_Free(&theNewK5Graph);
                return NOTOK;
            }
            else
            {
                // We _know_ that the edges of the K5 associated with an O-node are all virtual,
                // but we take the extra step of marking them virtual anyway.
                gp_SetEdgeVirtual(theNewK5Graph, e);
                gp_SetEdgeVirtual(theNewK5Graph, gp_GetTwinArc(theNewK5Graph, e));
            }

    // Now we can construct an embedding obstruction tree node to associate with
    // theNewK5Graph and then tell it to be an O-node that owns theNewK5Graph.
    if ((theNewONode = _K33Search_EONode_New(K33SEARCH_EOTYPE_ONODE, theNewK5Graph, TRUE)) == NULL)
    {
        gp_Free(&theNewK5Graph);
        return NOTOK;
    }

    // return the successfully created, orphan O-node
    *pNewONode = theNewONode;
    return OK;
}

/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/

/********************************************************************
 _K33Search_ExtractEmbeddingSubgraphs

 This method will extract the planar (K_{3,3}-free) subgraphs from
 the reducible bicomp rooted by virtual vertex R (a virtual copy of V).
 These subgraphs are bridge sets (denoted beta) that are detachable
 at the following 2-cuts: vx, vy, wx, wy, xy, and vw.

 We make a copy of the subgraph into a new graph instance, associate the
 new graph instance with an E-node. The E-node then becomes a child of
 the newONode using the EONode pointer in the K33 extended edge record
 of the proper edge in the K5 graph of the new O-node.

 While making a copy of the subgraph, it is important to also copy over
 any pathConnector and EONode pointer info from the original graph into
 the subgraph copy. This leads to the following considerations:

 1) Any edge in the subgraph copy that has a pathConnector or EONode
    pointer setting is known to be a virtual edge. Virtual edges are
    to be ignored when testing that the list of non-virtual edges in
    the EO-tree embedding exactly matches the set of edges in the
    original graph.

 2) The pathConnector information in a subgraph copy is there only as
    a flag of virtual-ness of the edge. All preserved paths needed
    for K_{3,3} homeomorph isolation are in the pathConnect edges of
    the main graph's embedding.

 3) Once an EONode pointer is copied to the virtual edge of a subgraph
    it must be set to NULL in the corresponding virtual edge of the
    main graph embedding to complete the reparenting of an EO-subtree
    from being a child of the root E-node to being a child of the E-node
    associated with the subgraph copy. On a practical level, this also
    ensures that we don't later do a double-free because both the main
    graph embedding and the subgraph copy point to an EONode as a child.

    Also, note that a virtual edge in the main graph embedding that has
    its EONode pointer set to NULL will not lose its virtual status
    because it will still have non-NIL pathConnector settings. Even
    once we delete the edges associated with pathConnector paths, the
    non-NIL pathConnector settings will still signal the virtual-ness
    of edges that are embedded as edges from the original graph.

 NOTE: This method does not and does not need to extract planar subgraphs
       for (u_{max}, v) nor the two of (u_{max}, w), (u_{max}, x) and
       (u_{max}, y) for the two of w, x, and y that have back edge
       connections only to u_{max}.

       In fact, the only planar subgraph that it is strictly necessary to
       extract and preserve as a child of the new O-node is the one for
       the beta_{vw} bridge set. This is needed because beta_{vw} must be
       saved while also preserving planarity of the main planar embedding.
       K5 minus an edge is planar, and this is why everything except for
       beta_{vw} is and can be simply left in the main planar embedding
       during a K_{3,3} search that does not certify K_{3,3}-free results.

       That being said, in this version, we do make E-nodes for not only
       beta_{vw} but also the other five beta bridge sets in the bicomp
       being reduced in step v because those other bridge sets are being
       reduced to single edges as part of maintaining worst-case O(n)
       performance. The single edges only preserve a path from their
       respective bridge sets, but if we end up needing to certify a
       K_{3,3}-free result, then we need the full subgraphs, so we make
       E-nodes to preserve them, too, and make those other five E-nodes
       be other children of the O-node.
 ********************************************************************/
int _K33Search_ExtractEmbeddingSubgraphs(graphP theGraph, int R, K33Search_EONodeP newONode)
{
    isolatorContextP IC = &theGraph->IC;

    // Extract the beta_{vx} bridge set into a subgraph associated with a new E-node
    // that is then made a child of the newONode.
    if (_K33Search_ExtractExternaFaceBridgeSet(theGraph, R, IC->v, IC->x, newONode) != OK)
        return NOTOK;

    // Likewise for beta_{vy}
    if (_K33Search_ExtractExternaFaceBridgeSet(theGraph, R, IC->v, IC->y, newONode) != OK)
        return NOTOK;

    // And beta_{wx}
    if (_K33Search_ExtractExternaFaceBridgeSet(theGraph, R, IC->w, IC->x, newONode) != OK)
        return NOTOK;

    // And beta_{wy}
    if (_K33Search_ExtractExternaFaceBridgeSet(theGraph, R, IC->w, IC->y, newONode) != OK)
        return NOTOK;

    // A specialized method is needed for beta_{xy} because it has no edges along the
    // external face of the bicomp being reduced.
    if (_K33Search_ExtractXYBridgeSet(theGraph, R, newONode) != OK)
        return NOTOK;

    // And for beta_{vw} because it has not been fully embedded
    if (_K33Search_ExtractVWBridgeSet(theGraph, R, newONode) != OK)
        return NOTOK;

    // The vertices and edges in the bicomp rooted by R are being represented in the
    // E-node subgraphs for beta_{vx}, beta-{vy}, beta_{wx}, beta_{wy}, and beta_{xy},
    // except for v, w, x, and y. Therefore, the edges and all the other vertices in
    // the bicomp rooted by R are made virtual.
    // NOTE: Although the bicomp's edges are subsequently deleted by _ReduceBicomp()
    //       some are saved via the pathConnector mechanism, so we are making sure
    //       those are interpreted as virtual by the K_{3,3}-free certifier
    if (_DeactivateBicomp(theGraph, R) != OK)
        return NOTOK;

    // Note that v is not marked by the call above, and bicomp roots like R don't
    // participate in K_{3,3}-free embedding integrity checks.
    // For w, x, and y, all three are cleared of defunct status because we are leaving the
    // embeddings of beta_w, beta_x, and beta_y in the main planar embedding.
    gp_ClearVertexDefunct(theGraph, R);
    gp_ClearVertexDefunct(theGraph, IC->w);
    gp_ClearVertexDefunct(theGraph, IC->x);
    gp_ClearVertexDefunct(theGraph, IC->y);

    // The edges and vertices in the pertinent-only descendant bicomps of w must be
    // marked in a way which ensures they don't impact the result of K_{3,3}-free
    // embedding integrity checks. The vertices and edges marked by this
    // operation are represented in the new E-node subgraph for beta_{vw}.
    if (_DeactivatePertinentOnlySubtrees(theGraph, IC->w) != OK)
        return NOTOK;

    // Success if all six bridge sets of the K4 homeomorph on v, w, x, and y were extracted
    return OK;
}

/********************************************************************
 _K33Search_ExtractExternaFaceBridgeSet()

 This method creates a new E-node and associates it, as owner, with
 a new subgraph that is created and used to make a copy of a bridge set
 in the bicomp rooted by R that is separable by the 2-cut consisting
 of the poleVertex (v or w) and the equatorVertex (x or y).

 The method assumes a consistent orientation of vertices in the bicomp.
 ********************************************************************/

int _K33Search_ExtractExternaFaceBridgeSet(graphP theGraph, int R, int poleVertex, int equatorVertex, K33Search_EONodeP newONode)
{
    isolatorContextP IC = &theGraph->IC;
    int linkDir, firstStartingEdge, lastStartingEdge, e;

    // This is a private function, but still making sure the two key parameters were correctly passed.
    // P.S. Parentheses added around && clauses not because needed but to appease -Wparentheses
    if ((poleVertex != IC->v && poleVertex != IC->w) ||
        (equatorVertex != IC->x && equatorVertex != IC->y))
        return NOTOK;

    // We mark the highest or lowest xy path, depending on parameterization, because
    // the marking will help find the boundary edges of the bridget set to extract.

    if (_ClearVisitedFlagsInBicomp(theGraph, R) != OK)
        return NOTOK;

    if ((poleVertex == IC->v ? _MarkHighestXYPath(theGraph) : _MarkLowestXYPath(theGraph)) != OK)
        return NOTOK;

    // Figure out the linkDir that indicates the external face edge of the equatorVertex that
    // leads directly to the poleVertex (i.e. not the direction that has to go around to the
    // oppositePoleVertex and oppositeEquatorVertex to get to the poleVertex).

    // Because the bicomp's vertices have been consistently oriented...
    // If the poleVertex is v, then R's link[0] edge leads to x, so x's link[1] edge goes back to R,
    // and R's link[1] edge leads to y, so y's link[0] edge goes back to R.
    // Likeewise, except in reverse if the poleVertex is w. Namely, w's link[1] edge goes toward x,
    // and its link[1] edge goes to y, so x's link[0] edge and y's link[1] edge both go toward w.
    if (poleVertex == IC->v)
        linkDir = equatorVertex == IC->x ? 1 : 0;
    else
        linkDir = equatorVertex == IC->x ? 0 : 1;

    firstStartingEdge = theGraph->V[equatorVertex].link[linkDir];

    // The linkDir not only indicates how to go around the external face. It also indicates the
    // direction to travel around the adjacency list of a vertex. We need to rotationally traverse
    // from the external face edge (i.e., the firstStartingEdge) toward the edge incident to the
    // equatorVertex that is part of the marked xy path. This last edge before the one in the
    // xy path will be the lastStartingEdge.

    lastStartingEdge = firstStartingEdge;
    while (gp_IsArc(lastStartingEdge))
    {
        e = theGraph->E[lastStartingEdge].link[linkDir];
        if (gp_GetEdgeVisited(theGraph, e))
            break;
        lastStartingEdge = e;
    }

    if (gp_IsNotArc(lastStartingEdge))
        return NOTOK;

    return _K33Search_ExtractEmbeddedBridgeSet(theGraph, R, poleVertex, equatorVertex, newONode,
                                               firstStartingEdge, linkDir, lastStartingEdge);
}

/********************************************************************
 _K33Search_ExtractXYBridgeSet()

    This method isolates the bridge set connecting the vertices x and y
    in the bicomp rooted by R. Because the bicomp is a planar embedding,
    this is all the vertices and edges that are between (inclusive of)
    the highest and lowest xy paths.
    ********************************************************************/

int _K33Search_ExtractXYBridgeSet(graphP theGraph, int R, K33Search_EONodeP newONode)
{
    int linkDir, firstStartingEdge, lastStartingEdge;

    // We will be computing the first and last edges emanating from y on which
    // to start the exploration of beta_{xy}. Because of the previously imposed
    // consistent vertex orientation, the link[0] edge record of y leads back to
    // the bicomp root R, and therefore also gives the link iteration direction
    // to traverse y's adjacency list in the direction from the external face
    // path to R, toward the highest xy path then the lowest xy path, then the
    // external face path leading to w.
    linkDir = 0;

    // We start with marking the highest xy path so we can find the firstStartingEdge
    if (_ClearVisitedFlagsInBicomp(theGraph, R) != OK || _MarkHighestXYPath(theGraph) != OK)
        return NOTOK;

    // Find the firstStartingEdge by iterating until the highest xy path edge
    // emanating from y is found
    firstStartingEdge = theGraph->V[theGraph->IC.y].link[linkDir];
    while (gp_IsArc(firstStartingEdge))
    {
        if (gp_GetEdgeVisited(theGraph, firstStartingEdge))
            break;
        firstStartingEdge = theGraph->E[firstStartingEdge].link[linkDir];
    }

    if (gp_IsNotArc(firstStartingEdge))
        return NOTOK;

    // Now we mark the lowest xy path so we can find the lastStartingEdge
    if (_ClearVisitedFlagsInBicomp(theGraph, R) != OK || _MarkLowestXYPath(theGraph) != OK)
        return NOTOK;

    // The lastStartingEdge is the lowest xy path edge emanating from y
    lastStartingEdge = firstStartingEdge;
    while (gp_IsArc(lastStartingEdge))
    {
        if (gp_GetEdgeVisited(theGraph, lastStartingEdge))
            break;
        lastStartingEdge = theGraph->E[lastStartingEdge].link[linkDir];
    }

    if (gp_IsNotArc(lastStartingEdge))
        return NOTOK;

    return _K33Search_ExtractEmbeddedBridgeSet(theGraph, R, theGraph->IC.x, theGraph->IC.y, newONode,
                                               firstStartingEdge, linkDir, lastStartingEdge);
}

/********************************************************************
 _K33Search_ExtractEmbeddedBridgeSet()

 The five bridge sets that are already fully embedded can be
 extracted by the same code sequence, including creating of
 the representative E-node and making that a child of the O-node.
 ********************************************************************/

int _K33Search_ExtractEmbeddedBridgeSet(graphP theGraph, int R, int cutv1, int cutv2, K33Search_EONodeP newONode,
                                        int firstStartingEdge, int linkDir, int lastStartingEdge)
{
    isolatorContextP IC = &theGraph->IC;
    int oppositeOfCutv1, oppositeOfCutv2, e;
    int numVerticesInSubgraph = 0, numEdgesInSubgraph = 0;
    graphP newSubgraphForBridgeSet = NULL;
    K33Search_EONodeP theNewENode = NULL;

    // The cutv2 vertex and the span of edges rotationally from firstStartingEdge to lastStartingEdge
    // indicate how to start exploring the beta bridge set being extracted. This call marks the
    // visited flags in all vertices and edges of the bicomp that need to be extracted.
    if (_K33Search_MarkBridgeSetToExtract(theGraph, R, cutv1, cutv2,
                                          firstStartingEdge, linkDir, lastStartingEdge) != OK)
        return NOTOK;

    // The bridge set is 2-cut separable at cutv1 and cutv2, so it is an error if the above
    // exploration reaches the opposite cutv1 or cutv2 (or virtual copy, if appropriate).
    if (cutv1 == IC->v || cutv1 == IC->w)
    {
        oppositeOfCutv1 = cutv1 == IC->v ? IC->w : R;
        oppositeOfCutv2 = cutv2 == IC->x ? IC->y : IC->x;
    }
    else if (cutv1 == IC->x)
    {
        oppositeOfCutv1 = R;
        oppositeOfCutv2 = IC->w;
    }
    else
        return NOTOK;

    if (gp_GetVertexVisited(theGraph, oppositeOfCutv1) || gp_GetVertexVisited(theGraph, oppositeOfCutv2))
        return NOTOK;

    // Get the order and size of the subgraph to be created
    if (_CountVerticesAndEdgesInBicomp(theGraph, R, TRUE, &numVerticesInSubgraph, &numEdgesInSubgraph) != OK)
        return NOTOK;

    // Make a new graph to hold the bridge set subgraph
    if ((newSubgraphForBridgeSet = gp_New()) == NULL)
        return NOTOK;

    if (gp_InitGraph(newSubgraphForBridgeSet, numVerticesInSubgraph) != OK ||
        gp_AttachK33Search(newSubgraphForBridgeSet) != OK)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Copy the vertices and edges marked visited from the bicomp to the new subgraph.
    // This will include transferring EONode pointer ownership to subgraph edges.
    // The bridge set subgraph is expected to be a planar embedding, including an extra
    // virtual edge between cutv1 and cutv2, if the edge does not already exist
    if (_K33Search_ExtractBridgeSet(theGraph, R, cutv1, cutv2, newSubgraphForBridgeSet) != OK)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // The new subgraph should have all edges of the bridge set plus one virtual edge
    // connecting the 2-cut (cutv1, cutv2), i.e., (first vertex, second vertex) in
    // the subgraph.
    // NOTE: We only add one to the edge count if the virtual edge between the 2-cut endpoints
    //       was added, which we only did if it was not going to be a duplicate edge.
    e = gp_GetNeighborEdgeRecord(newSubgraphForBridgeSet,
                                 gp_GetFirstVertex(newSubgraphForBridgeSet),
                                 gp_GetFirstVertex(newSubgraphForBridgeSet) + 1);

    if (newSubgraphForBridgeSet->M != numEdgesInSubgraph + (gp_GetEdgeVirtual(newSubgraphForBridgeSet, e) ? 1 : 0))
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Make an E-node and associate it with the new subgraph copy, making the E-node
    // the owner of the new subgraph.
    if ((theNewENode = _K33Search_EONode_New(K33SEARCH_EOTYPE_ENODE, newSubgraphForBridgeSet, TRUE)) == NULL)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Now we find the edge in the O-node's K5 that is associated with cutv1 and cutv2 and
    // point its EONode pointers at the newly created E-node
    if (_K33Search_AttachENodeAsChildOfONode(theNewENode, cutv1, cutv2, newONode) != OK)
    {
        _K33Search_EONode_Free(&theNewENode);
        return NOTOK;
    }

    // If all operations succeed, then we have successfully extracted the desired bridge set
    return OK;
}

/********************************************************************
 _K33Search_MarkBridgeSetToExtract()

 Starting with the edges in the bridge set that are incident to cutv2,
 we explore toward and including cutv1 to obtain the vertices and edges
 to be extracted to the bridge set subgraph.

 For bridge sets along the external face, it is expected that cutv1
 is a poleVertex and cutv2 is an equatorVertex.

 The first and last edge and the linkDir are references to edges
 incident to cutv2. They define the subset of edges to be used to
 start the exploration of the bridget set being extracted.
 ********************************************************************/

int _K33Search_MarkBridgeSetToExtract(graphP theGraph, int R, int cutv1, int cutv2,
                                      int firstStartingEdge, int linkDir, int lastStartingEdge)
{
    int v, e, ePrev;

    // Clear away markings such as the marked xy path
    if (_ClearVisitedFlagsInBicomp(theGraph, R) != OK)
        return NOTOK;

    // The graph's stack will be used, so make sure it is empty first
    if (!sp_IsEmpty(theGraph->theStack))
        return NOTOK;

    // A DFS exploration will be performed, starting with cutv2, but constrained to
    // only the neighbors indicated by the firstStartingEdge to lastStartingEdge
    gp_SetVertexVisited(theGraph, cutv2);

    ePrev = NIL;
    e = firstStartingEdge;
    while (ePrev != lastStartingEdge)
    {
        // Mark the edge as visited
        gp_SetEdgeVisited(theGraph, e);
        gp_SetEdgeVisited(theGraph, gp_GetTwinArc(theGraph, e));

        // Push a next vertex to visit
        sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

        // Go to the next edge in the rotation, but keep record of the edge just
        // processed so we can deetect when we have finished processing the
        // lastStartingEdge
        ePrev = e;
        e = theGraph->E[e].link[linkDir];

        // The lastSartingEdge is always (supposed to be) an internal edge, not an
        // external face edge, so it is an implementation error to get to the end of
        // the adjacency list. Note that the start and end of the adjacency list indicate
        // the edge records that affix an external face vertex, such as the equatorVertex,
        // to the external face.
        if (e == NIL)
            return NOTOK;
    }

    // The DFS exploration must also be constrained to stop at the poleVertex, so we
    // mark it visited ahead of the main loop so that the DFS will not go beyond it.
    // If the poleVertex is v, then we need to use R because R is v's representative
    // virtual vertex in the bicomp being reduced.
    gp_SetVertexVisited(theGraph, (cutv1 == theGraph->IC.v ? R : cutv1));

    // Perform the constrained DFS on the bridge set
    while (!sp_IsEmpty(theGraph->theStack))
    {
        sp_Pop(theGraph->theStack, v);

        // If the vertex has not already been visited, then we can now mark it visited
        // and process its adjacency list. Note that this if test is the one that also
        // ensures that the DFS explores no farther than cutv1 nor any other edges
        // of cutv2.
        if (!gp_GetVertexVisited(theGraph, v))
        {
            gp_SetVertexVisited(theGraph, v);

            e = gp_GetFirstArc(theGraph, v);
            while (e != NIL)
            {
                if (!gp_GetEdgeVisited(theGraph, e))
                {
                    gp_SetEdgeVisited(theGraph, e);
                    gp_SetEdgeVisited(theGraph, gp_GetTwinArc(theGraph, e));

                    sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));
                }
                e = gp_GetNextArc(theGraph, e);
            }
        }
    }

    // Bridge set subgraph successfully marked for extraction.
    return OK;
}

/********************************************************************
 _K33Search_ExtractBridgeSet()

 For the beta_{vw} bridge set, the unembedded edges from v to vertices
 in the pertinent-only subtrees rooted by w are added, along with all
 edges embedded in those pertinent-only subtrees of w.

 For the other five bridge sets of that are already fully embedded in
 B_{vw}, we copy the edges marked visited from the bicomp to the new subgraph.

 In both cases, this includes marking as virtual in the subgraph those edges
 that are virtual in the main graph. This also includes transferring EONode
 pointer ownership to subgraph edges (and NULLing them out in the main graph,
 while leaving virtual such edges in the main graph since they must still
 carry pathConnector info).

 A virtual edge representing (cutv1, cutv2) is added to the subgraph, if
 the vertices representing cutv1 and cutv2 are not already adjacent.

 The bridge set being copied is a  planar embedding (given the assumption that
 it may contain virtual edges that point to K5's embedded in child O-nodes).
 While it's difficult to copy a planar embedding from a graph into an
 appropriately-sized subgraph, while preserving adjacency node order in all
 subgraph vertices, it is possible to simply copy all of the required edges
 and then let the planarity algorithm recover *an* appropriate rotation scheme
 for the subgraph, because that's what planarity algorithms are for.
 As long as the subgraph can be validated as *a* planar embedding, then it is
 still K3,3-free, even if it is not *the* planar embedding originally created.

 Finally, a mapping between the subgraph's vertices and the graph's
 vertices is placed in the index members of the subgraph's vertices.
 ********************************************************************/
int _K33Search_ExtractBridgeSet(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet)
{
    K33SearchContext *context = NULL, *subgraphContext = NULL;
    int v, e;

    // Assign subgraph vertex index locations for all vertices in the bridge set, explicitly placing
    // the cutv1 and cutv2 into the first and second positions.
    if (_K33Search_MakeGraphSubgraphVertexMaps(theGraph, R, cutv1, cutv2, newSubgraphForBridgeSet) != OK)
        return NOTOK;

    // Use the vertex index mappings to help copy the edges of the bridge set into the subgraph,
    // without trying to preserve the adjacency list order of the planar embedding of the bridge set
    // contained in theGraph
    if (_K33Search_CopyEdgesToNewSubgraph(theGraph, R, cutv1, cutv2, newSubgraphForBridgeSet) != OK)
        return NOTOK;

    // Now we add an extra edge to the subgraph to connect the two cut vertices, cutv1 and cutv2.
    // We add it after all the other edges and with link 0 parameters so that it is guaranteed to become
    // a DFS tree edge during planarization below. And because it is a tree edge, the embedder will
    // process the two cut vertices last, which ensures that the tree edge will be on the external face.
    // Rather than using the graph-to-subgraph map to convert cutv1 and cutv2 into vertices in the
    // subgraph, we rely on the fact that they have been placed into the first and second vertex
    // positions in the subgraph.
    // NOTE: The edge need only be added if the 2-cut endpoints are not already joined by an edge
    //       and it's advantageous in this implementation to omit the unneeded edge because we
    //       later have to planarize the subgraph, and the planarity implementation doesn't
    //       support duplicate edges nor loops.
    if (!gp_IsNeighbor(newSubgraphForBridgeSet,
                       gp_GetFirstVertex(newSubgraphForBridgeSet),
                       gp_GetFirstVertex(newSubgraphForBridgeSet) + 1))
    {
        if (gp_AddEdge(newSubgraphForBridgeSet,
                       gp_GetFirstVertex(newSubgraphForBridgeSet), 0,
                       gp_GetFirstVertex(newSubgraphForBridgeSet) + 1, 0) != OK)
            return NOTOK;

        // The new edge is marked virtual because it is added in addition to the edges that are actually
        // in the bridge set being extracted.
        e = gp_GetFirstArc(newSubgraphForBridgeSet, gp_GetFirstVertex(newSubgraphForBridgeSet));
        if (gp_GetNeighbor(newSubgraphForBridgeSet, e) != gp_GetFirstVertex(newSubgraphForBridgeSet) + 1)
            return NOTOK;
        gp_SetEdgeVirtual(newSubgraphForBridgeSet, e);
        gp_SetEdgeVirtual(newSubgraphForBridgeSet, gp_GetTwinArc(newSubgraphForBridgeSet, e));
    }

    // Now we call the planarity algorithm so that the new subgraph contains a planar embedding of
    // the extracted bridge set.
    if (_K33Search_PlanarizeNewSubgraph(newSubgraphForBridgeSet) != OK)
        return NOTOK;

    // Get the graph's K3,3 extension because that is where the subgraph-to-graph map is stored
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // Get the subgraph's K3,3 extension because that is where we want to store the subgraph-to-graph map
    gp_FindExtension(newSubgraphForBridgeSet, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    // Copy the subgraph-to-graph map into the index members of the vertices of the new subgraph
    for (v = gp_GetFirstVertex(newSubgraphForBridgeSet); gp_VertexInRange(newSubgraphForBridgeSet, v); v++)
        subgraphContext->VI[v].subgraphToGraphIndex = context->VI[v].subgraphToGraphIndex;

    // For cleanliness, we NIL out the graph-to-subgraph and subgraph-to-graph map locations used
    // in this bridge set extraction.
    for (v = gp_GetFirstVertex(newSubgraphForBridgeSet); gp_VertexInRange(newSubgraphForBridgeSet, v); v++)
    {
        context->VI[context->VI[v].subgraphToGraphIndex].graphToSubgraphIndex = NIL;
        context->VI[v].subgraphToGraphIndex = NIL;
    }

    // A planar embedding of the bridge set has been successfully extracted into
    // the subgraph
    return OK;
}

/********************************************************************
 _K33Search_MakeGraphSubgraphVertexMaps()

 The cutv1 and cutv2 vertices are made the first two vertices in the
 new subgraph. From there, all other vertices in the bicomp rooted by
 R that are marked visited are assigned to successive locations in
 the subgraphToGraphindex map, and the vertex identities in the
 graphToSubgraphIndex map are made to point to those successive index
 locations in the subgraph.

 For example, consider the first vertex visited after cutv1 and cutv2.
 It will have index 3 in the subgraph. Suppose that vertex has index
 27 in theGraph. Then the graph-to-subgraph array for location 27
 will be set to 3, and the subgraph-to-graph array location 3 will
 be set to 27. This mapping is needed so we can figure out what

 ********************************************************************/

int _K33Search_MakeGraphSubgraphVertexMaps(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet)
{
    K33SearchContext *context = NULL;
    int nextSubgraphVertexIndex, v, e;

    // Get the graph's K3,3 extension because that is where the subgraph-to-graph map is stored
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // Associate cutv1 and cutv2 with the first and second vertex locations in the new subgraph
    context->VI[cutv1].graphToSubgraphIndex = gp_GetFirstVertex(newSubgraphForBridgeSet);
    context->VI[cutv2].graphToSubgraphIndex = gp_GetFirstVertex(newSubgraphForBridgeSet) + 1;
    context->VI[gp_GetFirstVertex(newSubgraphForBridgeSet)].subgraphToGraphIndex = cutv1;
    context->VI[gp_GetFirstVertex(newSubgraphForBridgeSet) + 1].subgraphToGraphIndex = cutv2;

    // Now we explore the bridge set previously marked in the bicomp rooted by R, and we set up a
    // similar mapping for all other vertices encountered. Note that cutv1 may be the non-virtual
    // vertex associated with R, so we ignore R too because it is equivalent to ignoring cutv1 in
    // the two bridge sets that contain R (beta_{vx} and beta_{vy}), and ignoring R is otherwise
    // harmless as it is not in the other three bridge sets (beta_{wx}, beta_{wy}, and beta_{xy}).
    nextSubgraphVertexIndex = gp_GetFirstVertex(newSubgraphForBridgeSet) + 2;

    // No point making this routine immune to preexisting stack content
    if (!sp_IsEmpty(theGraph->theStack))
        return NOTOK;

    // If we're making the graph-subgraph vertex maps for beta_{vw}, we have
    // to call a special subroutine to traverse the pertinent-only subtrees
    // rooted by w as the way to find all vertices other than v and w
    if (cutv1 == theGraph->IC.v && cutv2 == theGraph->IC.w)
    {
        if (_K33Search_MapVerticesInPertinentOnlySubtrees(theGraph, context, theGraph->IC.w, &nextSubgraphVertexIndex) != OK)
            return NOTOK;

        return OK;
    }

    // Otherwise, we are processing a bridge set other than beta_{vw}, and whichever of
    // those we are doing has its vertices and edges marked visited in the bicomp rooted
    // by R, so we traverse the bicomp and...

    // Start at the first cut vertex looking for all visited vertices to add to the mapping
    sp_Push(theGraph->theStack, cutv1 == theGraph->IC.v ? R : cutv1);
    while (!sp_IsEmpty(theGraph->theStack))
    {
        sp_Pop(theGraph->theStack, v);
        if (gp_GetVertexVisited(theGraph, v))
        {
            // Every visited vertex will be temporarily marked unvisited here so that
            // we don't add vertices to the mapping multiple times.
            gp_ClearVertexVisited(theGraph, v);

            // Add to the mapping only vertices other than cutv1 and cutv2 (and also
            // not R, which may be the representative of cutv1 in the bicomp)
            if (v != cutv1 && v != cutv2 && v != R)
            {
                // We expect no more vertices in the mapping than were previously counted
                // as being marked in the bicomp.
                if (nextSubgraphVertexIndex > newSubgraphForBridgeSet->N)
                    return NOTOK;

                // Create the mapping between v and the next vertex in the subgraph
                context->VI[v].graphToSubgraphIndex = nextSubgraphVertexIndex;
                context->VI[nextSubgraphVertexIndex].subgraphToGraphIndex = v;

                // Increment for the next mapping
                nextSubgraphVertexIndex++;
            }

            // For every edge in the adjacency list of v, we push its neighbors
            // that are marked visited. Note that this can push some vertices
            // multiple times, when they are neighbors of multiple vertices that
            // are processed before they are. However, such vertices will eventually
            // get popped for the first time, added to the mapping, and then their
            // remaining stack entries will be popped and ignored because they
            // will be changed to unvisited until after the outer loop ends.
            e = gp_GetFirstArc(theGraph, v);
            while (gp_IsArc(e))
            {
                if (gp_GetVertexVisited(theGraph, gp_GetNeighbor(theGraph, e)))
                    sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

                e = gp_GetNextArc(theGraph, e);
            }
        }
    }

    // We had to mark all the vertices of the subgraph as unvisited, so now
    // we restore their visited flags to raised. This has to be done specially
    // for the first vertex because cutv1 may be the non-virtual vertex of R,
    // but it is R that has to be marked visited again in that case.
    gp_SetVertexVisited(theGraph, cutv1 == theGraph->IC.v ? R : cutv1);
    for (v = gp_GetFirstVertex(newSubgraphForBridgeSet) + 1; gp_VertexInRange(newSubgraphForBridgeSet, v); v++)
        gp_SetVertexVisited(theGraph, context->VI[v].subgraphToGraphIndex);

    // The mapping has been successfully created.
    return OK;
}

/********************************************************************
 _K33Search_CopyEdgesToNewSubgraph()
 ********************************************************************/
int _K33Search_CopyEdgesToNewSubgraph(graphP theGraph, int R, int cutv1, int cutv2, graphP newSubgraphForBridgeSet)
{
    K33SearchContext *context = NULL;
    int v, e;

    // Get the graph's K3,3 extension because that is where the subgraph-to-graph map is stored
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // Seems to be no point in immunizing this subroutine from pre-existing stack content
    if (!sp_IsEmpty(theGraph->theStack))
        return NOTOK;

    // If we're copying edges to the subgraph for beta_{vw}, we have
    // to call a special subroutine to traverse the pertinent-only subtrees
    // rooted by w as the way to find all of the bicomps that contain all
    // the edges that need to be copied.
    if (cutv1 == theGraph->IC.v && cutv2 == theGraph->IC.w)
    {
        if (_K33Search_AddUnembeddedEdgesToSubgraph(theGraph, context, theGraph->IC.w, newSubgraphForBridgeSet) != OK)
            return NOTOK;

        if (_K33Search_CopyEdgesFromPertinentOnlySubtrees(theGraph, context, theGraph->IC.w, newSubgraphForBridgeSet) != OK)
            return NOTOK;

        return OK;
    }

    // Otherwise, we are processing a bridge set other than beta_{vw}, and whichever of
    // those we are doing has its vertices and edges marked visited in the bicomp rooted
    // by R, so we traverse only the one bicomp to copy the edges marked visited, by ...

    // Starting at the first vertex in the 2-cut, we seek all visited vertices and edges
    sp_Push(theGraph->theStack, cutv1 == theGraph->IC.v ? R : cutv1);
    while (!sp_IsEmpty(theGraph->theStack))
    {
        sp_Pop(theGraph->theStack, v);
        if (gp_GetVertexVisited(theGraph, v))
        {
            // Need to avoid visiting visited vertices more than once
            gp_ClearVertexVisited(theGraph, v);

            e = gp_GetFirstArc(theGraph, v);
            while (gp_IsArc(e))
            {
                // If the edge is visited, then it has to be copied to the subgraph
                if (gp_GetEdgeVisited(theGraph, e))
                {
                    // First make sure we won't copy the edge twice
                    gp_ClearEdgeVisited(theGraph, e);
                    gp_ClearEdgeVisited(theGraph, gp_GetTwinArc(theGraph, e));

                    if (_K33Search_CopyEdgeToNewSubgraph(theGraph, context, e, newSubgraphForBridgeSet) != OK)
                        return NOTOK;

                    // The neighbor incident to v by the visited edge must be pushed so that
                    // the neighbor's other incident edges can eventually be processed.
                    sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));
                }

                e = gp_GetNextArc(theGraph, e);
            }
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_PlanarizeNewSubgraph()
 ********************************************************************/
int _K33Search_PlanarizeNewSubgraph(graphP theNewSubgraph)
{
    // The result should be planar (OK), so if it is NONEMBEDDABLE or NOTOK, we exit this routine with an error
    if (gp_Embed(theNewSubgraph, EMBEDFLAGS_PLANAR) != OK)
        return NOTOK;

    // The output of embedding remains sorted in DFI order, so we switch it back
    // to the original vertex order, which matches the graph-to-subgraph mapping
    if (gp_SortVertices(theNewSubgraph) != OK)
        return NOTOK;

    // Test the integrity of the planar embedding of the extracted bridget set subgraph
    // NOTE: We test this for now, but we don't need to because it will be tested later,
    //       as part of the K_{3,3}-free embedding integrity testing
    if (_CheckEmbeddingFacialIntegrity(theNewSubgraph) != OK)
        return NOTOK;

    return OK;
}

/********************************************************************
 _K33Search_ExtractVWBridgeSet()

 This extracts the edges and vertices of beta_vw into a subgraph,
 which is associated with a new E-node as owner. The E-node is
 then made a child of newONode via the edge for (v, w) in the
 newONode's K5 graph.

 As much as possible, this method mimics the method for extracting
 the other five bridge sets that are fully embedded in B_{vw},
 namely the method _K33Search_ExtractEmbeddedBridgeSet().
 Relevant subroutines of _K33Search_ExtractBridgeSet() have been
 generalized to achieve this symmetry and reuse code.
 ********************************************************************/

int _K33Search_ExtractVWBridgeSet(graphP theGraph, int R, K33Search_EONodeP newONode)
{
    isolatorContextP IC = &theGraph->IC;
    int numVerticesInSubgraph = 0, numEdgesInSubgraph = 0, e;
    graphP newSubgraphForBridgeSet = NULL;
    K33Search_EONodeP theNewENode = NULL;

    // We start by counting the edges in the unembedded 'claw' of v in beta_{vw}.
    if (_CountUnembeddedEdgesInPertinentOnlySubtrees(theGraph, IC->w, &numEdgesInSubgraph) != OK)
        return NOTOK;

    // We start out the number of vertices in the subgraph at 2 to account for v
    // and w.
    numVerticesInSubgraph = 2;

    // Now we add to those counts the number of vertices and edges in all of the
    // pertinent-only descendant bicomps of w, excluding bicomp roots because they are
    // duplicates of primary vertices used to represent them in their separate child bicomps
    if (_CountVerticesAndEdgesInPertinentOnlySubtrees(theGraph, IC->w, &numVerticesInSubgraph, &numEdgesInSubgraph) != OK)
        return NOTOK;

    // Make a new graph to hold the bridge set subgraph
    if ((newSubgraphForBridgeSet = gp_New()) == NULL)
        return NOTOK;

    if (gp_InitGraph(newSubgraphForBridgeSet, numVerticesInSubgraph) != OK ||
        gp_AttachK33Search(newSubgraphForBridgeSet) != OK)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Copy into the new subgraph all embedded edges in the pertinent-only
    // subtrees rooted by w, all unembedded edges from v to vertices in the
    // pertinent-only subtrees rooted by w, and add a virtual edge for (v, w) if
    // it is not included in the unembedded edges that are added. Then,
    // planarize the new subgraph and add the subgraph-to-graph mapping to the
    // index members of its vertices.
    // NOTE: Copying embedded edges includes transferring any EONode pointers
    //       from the original edges in theGraph to the newly added edges
    if (_K33Search_ExtractBridgeSet(theGraph, R, IC->v, IC->w, newSubgraphForBridgeSet) != OK)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // The new subgraph should have all edges of the bridge set plus one virtual edge
    // connecting the 2-cut (v, equatorVertex), i.e., (first vertex, second vertex)
    // in the subgraph.
    // NOTE: We only add one to the edge count if the virtual edge between the 2-cut endpoints
    //       was added, which we only do if it is not going to be a duplicate edge.
    e = gp_GetNeighborEdgeRecord(newSubgraphForBridgeSet,
                                 gp_GetFirstVertex(newSubgraphForBridgeSet),
                                 gp_GetFirstVertex(newSubgraphForBridgeSet) + 1);

    if (newSubgraphForBridgeSet->M != numEdgesInSubgraph + (gp_GetEdgeVirtual(newSubgraphForBridgeSet, e) ? 1 : 0))
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Make an E-node and associate it with the new subgraph copy, making the E-node
    // the owner of the new subgraph.
    if ((theNewENode = _K33Search_EONode_New(K33SEARCH_EOTYPE_ENODE, newSubgraphForBridgeSet, TRUE)) == NULL)
    {
        gp_Free(&newSubgraphForBridgeSet);
        return NOTOK;
    }

    // Now we find the edge in the O-node's K5 that is associated with cutv1 and cutv2 and
    // point its EONode pointers at the newly created E-node
    if (_K33Search_AttachENodeAsChildOfONode(theNewENode, IC->v, IC->w, newONode) != OK)
    {
        _K33Search_EONode_Free(&theNewENode);
        return NOTOK;
    }

    // If all operations succeed, then we have successfully extracted the desired bridge set
    return OK;
}

/********************************************************************
 _K33Search_AttachONodeAsChildOfRoot()

 The new O-node must be made a child of the root E-node by pointing
 the EONode pointers of a main planar embedding edge at the new O-node.

 While this could be an edge such as (u_{max}, w) or (u_{max}, x) or
 (u_{max}, y), those edges will not be able to be added until a
 future step u_max. The ReduceBicomp() invocation is occurring during
 the step v processing, so we use one of the edges created by the
 ReduceBicomp() process instead.
 ********************************************************************/
int _K33Search_AttachONodeAsChildOfRoot(graphP theGraph, K33Search_EONodeP newONode)
{
    int e;
    K33SearchContext *context = NULL;

    // Get the second edge in the adjacency list of x (the first one should be an
    // external face edge leading to  the bicomp root or to w).
    e = gp_GetFirstArc(theGraph, theGraph->IC.x);
    e = gp_GetNextArc(theGraph, e);

    // Ensure that the second edge in the adjacency list of x does in fact lead to y
    // (in other words, that it is the stand-in for the xy path)
    if (gp_GetNeighbor(theGraph, e) != theGraph->IC.y)
        return NOTOK;

    // Get the graph's K_{3,3} extension so we can modify the extended edge records
    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // Make the xy path edge point to the new O-node, thereby making it a child
    // of the root E-node associated with the main planar embedding that contains
    // the xy path edge.
    context->E[e].EONode = context->E[gp_GetTwinArc(theGraph, e)].EONode = newONode;
    return OK;
}

/********************************************************************
 _K33Search_AttachENodeAsChildOfONode()
 The E-node contains a subgraph of the input graph that is separable
 by the 2-cut (cutv1, cutv2) from the K5 homeomorph in the input graph
 that is represented by the O-node.
 ********************************************************************/
int _K33Search_AttachENodeAsChildOfONode(K33Search_EONodeP theENode, int cutv1, int cutv2, K33Search_EONodeP theONode)
{
    graphP theK5 = theONode->subgraph;
    int e, EsizeOccupied, neighborIndex, neighborIndexTwin;
    K33SearchContext *context = NULL;

    // There are only 10 edges in the K5, which is a constant, so we just sequentially
    // search through them to find the edge that should take the E-node pointer.
    EsizeOccupied = gp_EdgeInUseIndexBound(theK5);
    for (e = gp_GetFirstEdge(theK5); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theK5, e))
        {
            // The index field in each vertex of theK5 gives us the identity of the vertex
            // to which the K5 vertex maps in the originating graph...
            neighborIndex = theK5->V[gp_GetNeighbor(theK5, e)].index;
            neighborIndexTwin = theK5->V[gp_GetNeighbor(theK5, gp_GetTwinArc(theK5, e))].index;

            // ... which we can then compare to the cut vertex indices in the original graph
            // from which the E-node's subgraph has been extracted.
            if ((neighborIndex == cutv1 && neighborIndexTwin == cutv2) ||
                (neighborIndex == cutv2 && neighborIndexTwin == cutv1))
            {
                // And assign the E-node pointer to the EONode pointer in the extended
                // edge records for the edge whose endpoint vertices have index values of
                // cutv1 and cutv2.
                gp_FindExtension(theK5, K33SEARCH_ID, (void *)&context);
                if (context == NULL)
                    return NOTOK;
                context->E[e].EONode = context->E[gp_GetTwinArc(theK5, e)].EONode = theENode;
                break;
            }
        }
    }

    // Return OK unless we somehow failed to find the desired K5 edge
    return context != NULL ? OK : NOTOK;
}

/********************************************************************
 _CountVerticesAndEdgesInBicomp()

 This should be promoted to graphUtils.c

 Counts the vertices and edges in the given bicomp, tallying
 only those marked visited if visitedOnly is TRUE.

 Returns one or both results, depending on which of the output
 parameters is non-NULL.

 Returns OK on success, NOTOK on failure.
 ********************************************************************/
int _CountVerticesAndEdgesInBicomp(graphP theGraph, int BicompRoot, int visitedOnly,
                                   int *pNumVisitedVertices, int *pNumVisitedEdges)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int v, e;
    int numVisitedVertices = 0, numVisitedEdges = 0;

    sp_Push(theGraph->theStack, BicompRoot);
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, v);
        if (!visitedOnly || gp_GetVertexVisited(theGraph, v))
            numVisitedVertices++;

        e = gp_GetFirstArc(theGraph, v);
        while (gp_IsArc(e))
        {
            if (!visitedOnly || gp_GetEdgeVisited(theGraph, e))
                numVisitedEdges++;

            if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_CHILD)
                sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

            e = gp_GetNextArc(theGraph, e);
        }
    }

    if (pNumVisitedVertices != NULL)
        *pNumVisitedVertices = numVisitedVertices;

    if (pNumVisitedEdges != NULL)
        *pNumVisitedEdges = numVisitedEdges >> 1;

    return OK;
}

/********************************************************************
 _DeactivateBicomp()
 ********************************************************************/
int _DeactivateBicomp(graphP theGraph, int R)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int v, e;

    sp_Push(theGraph->theStack, R);
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, v);
        gp_SetVertexDefunct(theGraph, v);

        e = gp_GetFirstArc(theGraph, v);
        while (gp_IsArc(e))
        {
            gp_SetEdgeVirtual(theGraph, e);

            if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_CHILD)
                sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

            e = gp_GetNextArc(theGraph, e);
        }
    }

    return OK;
}

/********************************************************************
 _DeactivatePertinentOnlySubtrees()
 ********************************************************************/
int _DeactivatePertinentOnlySubtrees(graphP theGraph, int w)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root, deactivate it, and then run its external face to find
    // all vertices with pertinent child bicomp roots to push.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        if (_DeactivateBicomp(theGraph, pertinentRoot) != OK)
            return NOTOK;

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    return OK;
}

/********************************************************************
 _CountUnembeddedEdgesInPertinentOnlySubtrees()

 Goes through the pertinent subtrees the same way as in
 _DeactivatePertinentOnlySubtrees(), except just counts the
 descendants on the external faces that were marked by _Walkup()
 as being the descendant endpoint of an as-yet unembedded back edge.
 ********************************************************************/
int _CountUnembeddedEdgesInPertinentOnlySubtrees(graphP theGraph, int w, int *pNumEdgesInSubgraph)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack), edgeCount = 0;
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root and then run its external face to find the descendants
    // of w that are endpoints of unembedded back edges to v, and to find more pertinent
    // child bicomps to which we must descend.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            // Test for an unembedded back edge to v
            if (gp_IsArc(gp_GetVertexPertinentEdge(theGraph, W)))
                edgeCount++;

            // Push all the pertinent child bicomps of W
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    // If w is also an unembedded back edge endpoint, then increment the counter
    if (gp_IsArc(gp_GetVertexPertinentEdge(theGraph, w)))
        edgeCount++;

    // Increment the value of the output parameter and return successfully
    *pNumEdgesInSubgraph += edgeCount;

    return OK;
}

/********************************************************************
 _CountVerticesAndEdgesInPertinentOnlySubtrees()

 Goes through the pertinent subtrees the same way as in
 _DeactivatePertinentOnlySubtrees(), except just counts the
 vertices and edges along the way. The vertex count excludes
 w and all bicomp roots encountered along the way.
 ********************************************************************/
int _CountVerticesAndEdgesInPertinentOnlySubtrees(graphP theGraph, int w,
                                                  int *pNumVerticesInSubgraph, int *pNumEdgesInSubgraph)
{
    int totalVertexCount = 0, totalEdgeCount = 0;
    int numVerticesInBicomp, numEdgesInBicomp;
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root and then run its external face to find the descendants
    // of w that are endpoints of unembedded back edges to v, and to find more pertinent
    // child bicomps to which we must descend.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        // Count the number of vertices and edges in the bicomp, in total (3rd param FALSE)
        numVerticesInBicomp = numEdgesInBicomp = 0;
        if (_CountVerticesAndEdgesInBicomp(theGraph, pertinentRoot, FALSE, &numVerticesInBicomp, &numEdgesInBicomp) != OK)
            return NOTOK;

        // Contribute the bicomp numbers to the totals
        // (excluding the bicomp root from the vertex total by subtracting 1)
        totalVertexCount += numVerticesInBicomp - 1;
        totalEdgeCount += numEdgesInBicomp;

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            // Push all the pertinent child bicomps of W
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    // Provide output into the parameters and return successfully
    if (pNumVerticesInSubgraph != NULL)
        *pNumVerticesInSubgraph += totalVertexCount;
    if (pNumEdgesInSubgraph != NULL)
        *pNumEdgesInSubgraph += totalEdgeCount;

    return OK;
}

/********************************************************************
 _K33Search_MapVerticesInPertinentOnlySubtrees()

 Goes through the pertinent subtrees the same way as in
 _DeactivatePertinentOnlySubtrees(), except just maps
 each vertex encountered to the next vertex of the subgraph
 ********************************************************************/
int _K33Search_MapVerticesInPertinentOnlySubtrees(graphP theGraph, K33SearchContext *context, int w, int *pNextSubgraphVertexIndex)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root and then run its external face to find the descendants
    // of w that are endpoints of unembedded back edges to v, and to find more pertinent
    // child bicomps to which we must descend.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        // Map the vertices in the bicomp rooted by pertinentRoot
        if (_K33Search_MapVerticesInBicomp(theGraph, context, pertinentRoot, pNextSubgraphVertexIndex) != OK)
            return NOTOK;

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            // Push all the pertinent child bicomps of W
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_MapVerticesInBicomp()
 ********************************************************************/
int _K33Search_MapVerticesInBicomp(graphP theGraph, K33SearchContext *context, int R, int *pNextSubgraphVertexIndex)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int v, e;

    sp_Push(theGraph->theStack, R);
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, v);

        // Map all vertices in the bicomp except for the bicomp root
        if (v != R)
        {
            // Create the mapping between v and the next vertex in the subgraph
            context->VI[v].graphToSubgraphIndex = *pNextSubgraphVertexIndex;
            context->VI[*pNextSubgraphVertexIndex].subgraphToGraphIndex = v;

            // Increment for the next mapping
            (*pNextSubgraphVertexIndex)++;
        }

        // Push the direct "child" nodes of v (The quotes are around the word child
        // because an edge marked EDGE_TYPE_CHILD may in fact lead to a descendant
        // and not a direct child if there has been a ReduceBicomp).
        e = gp_GetFirstArc(theGraph, v);
        while (gp_IsArc(e))
        {
            if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_CHILD)
                sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

            e = gp_GetNextArc(theGraph, e);
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_AddUnembeddedEdgesToSubgraph()

 Check w and all of its pertinent-only descendant bicomps for vertices
 marked as needing to have an edge embedded to v, then embed those
 edges in the new subgraph (since they cannot be put in the main
 planar embedding without violating its planarity).
 Similar code to _CountUnembeddedEdgesInPertinentOnlySubtrees()
 ********************************************************************/
int _K33Search_AddUnembeddedEdgesToSubgraph(graphP theGraph, K33SearchContext *context, int w, graphP newSubgraphForBridgeSet)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext, e;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root and then run its external face to find the descendants
    // of w that are endpoints of unembedded back edges to v, and to find more pertinent
    // child bicomps to which we must descend.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            // Test for an unembedded back edge to v, and add the edge if needed
            e = gp_GetVertexPertinentEdge(theGraph, W);
            if (gp_IsArc(e))
            {
                if (_K33Search_AddNewEdgeToSubgraph(theGraph, context, theGraph->IC.v, W, newSubgraphForBridgeSet) != OK)
                    return NOTOK;

                // Once the edge has been added to the subgraph, we need to mark it as virtual in the main graph
                // so that it will be ignored when assembling the main planar embedding
                gp_SetEdgeVirtual(theGraph, e);
                gp_SetEdgeVirtual(theGraph, gp_GetTwinArc(theGraph, e));
            }

            // Push all the pertinent child bicomps of W
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    // If w is also an unembedded back edge endpoint, then vertex map (v, w)
    // to the new subgraph and add the resulting edge
    e = gp_GetVertexPertinentEdge(theGraph, w);
    if (gp_IsArc(e))
    {
        if (_K33Search_AddNewEdgeToSubgraph(theGraph, context, theGraph->IC.v, w, newSubgraphForBridgeSet) != OK)
            return NOTOK;

        // Once the edge has been added to the subgraph, we need to mark it as virtual in the main graph
        // so that it will be ignored when assembling the main planar embedding
        gp_SetEdgeVirtual(theGraph, e);
        gp_SetEdgeVirtual(theGraph, gp_GetTwinArc(theGraph, e));
    }

    return OK;
}

/********************************************************************
 _K33Search_AddNewEdgeToSubgraph()
 ********************************************************************/
int _K33Search_AddNewEdgeToSubgraph(graphP theGraph, K33SearchContext *context, int v, int w, graphP newSubgraphForBridgeSet)
{
    int vInSubgraph, wInSubgraph, eInSubgraph;

    // Use the mapping to generate the edge in the subgraph
    vInSubgraph = context->VI[v].graphToSubgraphIndex;
    wInSubgraph = context->VI[w].graphToSubgraphIndex;

    if (gp_AddEdge(newSubgraphForBridgeSet, vInSubgraph, 0, wInSubgraph, 0) != OK)
        return NOTOK;

    eInSubgraph = newSubgraphForBridgeSet->V[vInSubgraph].link[0];
    if (gp_GetNeighbor(newSubgraphForBridgeSet, eInSubgraph) != wInSubgraph)
        return NOTOK;

    return OK;
}

/********************************************************************
 _K33Search_CopyEdgesFromPertinentOnlySubtrees()

 Go through the pertinent subtrees the same way as in
 _DeactivatePertinentOnlySubtrees(), except call
 _K33Search_CopyEdgesFromBicomp() on each bicomp
 encountered to copy the edges to the new subgraph
 ********************************************************************/
int _K33Search_CopyEdgesFromPertinentOnlySubtrees(graphP theGraph, K33SearchContext *context, int w, graphP newSubgraphForBridgeSet)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int pertinentRootListElem, pertinentRoot, W, WPrevLink, Wnext;

    // For each pertinent-only child bicomp of w (in pertinentRoots),
    //    Push the root on the stack to initialize the loop
    // NOTE: We push all pertinent bicomp roots because we're in a pertinent-only subtree
    //       (so no need to test pertinent but not future pertinent here)
    pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, w);
    while (gp_IsVertex(pertinentRootListElem))
    {
        pertinentRoot = gp_GetRootFromDFSChild(theGraph, pertinentRootListElem);
        sp_Push(theGraph->theStack, pertinentRoot);
        pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, w), pertinentRootListElem);
    }

    // While the stack has pertinent roots left (before reaching the caller's stackBottom)
    // then we pop a bicomp root, invoke _K33Search_CopyEdgesFromBicomp() on it, and then
    // run its external face to find the child bicomps to which we must descend to fully
    // explore the pertinent only subtrees.
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, pertinentRoot);

        if (_K33Search_CopyEdgesFromBicomp(theGraph, context, pertinentRoot, newSubgraphForBridgeSet) != OK)
            return NOTOK;

        // Get a first non-root vertex on the link[0] side of the pertinentRoot
        W = gp_GetExtFaceVertex(theGraph, pertinentRoot, 0);
        WPrevLink = gp_GetExtFaceVertex(theGraph, W, 1) == pertinentRoot ? 1 : 0;
        while (W != pertinentRoot)
        {
            // Push all the pertinent child bicomps of W
            pertinentRootListElem = gp_GetVertexFirstPertinentRootChild(theGraph, W);
            while (gp_IsVertex(pertinentRootListElem))
            {
                sp_Push(theGraph->theStack, gp_GetRootFromDFSChild(theGraph, pertinentRootListElem));
                pertinentRootListElem = LCGetNext(theGraph->BicompRootLists, gp_GetVertexPertinentRootsList(theGraph, W), pertinentRootListElem);
            }

            // We get the successor on the external face of the current
            Wnext = gp_GetExtFaceVertex(theGraph, W, 1 ^ WPrevLink);
            WPrevLink = gp_GetExtFaceVertex(theGraph, Wnext, 1) == W ? 1 : 0;
            W = Wnext;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_CopyEdgesFromBicomp()
 ********************************************************************/
int _K33Search_CopyEdgesFromBicomp(graphP theGraph, K33SearchContext *context, int R, graphP newSubgraphForBridgeSet)
{
    int stackBottom = sp_GetCurrentSize(theGraph->theStack);
    int v, e;

    sp_Push(theGraph->theStack, R);
    while (sp_GetCurrentSize(theGraph->theStack) > stackBottom)
    {
        sp_Pop(theGraph->theStack, v);

        e = gp_GetFirstArc(theGraph, v);
        while (gp_IsArc(e))
        {
            // Every edge will be visited twice by this loop construct,
            // but we only want to add the edge once...
            if (gp_GetNeighbor(theGraph, e) < gp_GetNeighbor(theGraph, gp_GetTwinArc(theGraph, e)))
            {
                if (_K33Search_CopyEdgeToNewSubgraph(theGraph, context, e, newSubgraphForBridgeSet) != OK)
                    return NOTOK;
            }

            if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_CHILD)
                sp_Push(theGraph->theStack, gp_GetNeighbor(theGraph, e));

            e = gp_GetNextArc(theGraph, e);
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_CopyEdgeToNewSubgraph()

 Adds edge e into the new subgraph, translating to the vertex
 indices for the subgraph using the graph to subgraph mapping.
 Copies virtualness to the subgraph's edge, and transfers an
 EONodeP pointer from theGraph's edge to the subgraph's new edge.
 ********************************************************************/

int _K33Search_CopyEdgeToNewSubgraph(graphP theGraph, K33SearchContext *context, int e, graphP newSubgraphForBridgeSet)
{
    K33SearchContext *contextSubgraph = NULL;
    int v, w, vInSubgraph, wInSubgraph, eInSubgraph;

    // Get the two vertices associated with edge e
    v = gp_GetNeighbor(theGraph, gp_GetTwinArc(theGraph, e));
    w = gp_GetNeighbor(theGraph, e);
    v = gp_IsBicompRoot(theGraph, v) ? gp_GetPrimaryVertexFromRoot(theGraph, v) : v;
    w = gp_IsBicompRoot(theGraph, w) ? gp_GetPrimaryVertexFromRoot(theGraph, w) : w;

    // Use the mapping to generate the edge in the subgraph
    vInSubgraph = context->VI[v].graphToSubgraphIndex;
    wInSubgraph = context->VI[w].graphToSubgraphIndex;
    if (gp_AddEdge(newSubgraphForBridgeSet, vInSubgraph, 0, wInSubgraph, 0) != OK)
        return NOTOK;
    eInSubgraph = newSubgraphForBridgeSet->V[vInSubgraph].link[0];
    if (gp_GetNeighbor(newSubgraphForBridgeSet, eInSubgraph) != wInSubgraph)
        return NOTOK;

    // Mark the new edge virtual, if the original edge is virtual
    // NOTE that we don't have to transfer pathConnector info, just virtualness, because if
    // an edge has pathConnector info, then a nearby edge already has an EONode pointer to
    // an O-node whose child subtrees contain all the pathConnector edges in their subgraphs
    if (gp_GetEdgeVirtual(theGraph, e))
    {
        gp_SetEdgeVirtual(newSubgraphForBridgeSet, eInSubgraph);
        gp_SetEdgeVirtual(newSubgraphForBridgeSet, gp_GetTwinArc(newSubgraphForBridgeSet, eInSubgraph));
    }

    // Transfer ownership of an EONode pointer, if one is there
    if (context->E[e].EONode != NULL)
    {
        gp_FindExtension(newSubgraphForBridgeSet, K33SEARCH_ID, (void *)&contextSubgraph);
        if (contextSubgraph == NULL)
            return NOTOK;

        contextSubgraph->E[eInSubgraph].EONode = context->E[e].EONode;
        contextSubgraph->E[gp_GetTwinArc(newSubgraphForBridgeSet, eInSubgraph)].EONode = context->E[e].EONode;

        context->E[e].EONode = context->E[gp_GetTwinArc(theGraph, e)].EONode = NULL;
    }

    return OK;
}

/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/
/********************************************************************/

/********************************************************************
 _K33Search_AssembleMainPlanarEmbedding()

 This method makes a new subgraph consisting of all the non-virtual
 edges only, plus those edges needed to preserve the structure of
 the EO tree. The new subgraph is made to resemble the descendant
 subgraphs, such as by setting the subgraph to graph mapping.
 ********************************************************************/
int _K33Search_AssembleMainPlanarEmbedding(K33Search_EONodeP EOTreeRoot)
{
    int v, e, EsizeOccupied;
    graphP theGraph = EOTreeRoot->subgraph, rootSubgraph = NULL;
    K33SearchContext *context = NULL, *subgraphContext = NULL;

    gp_FindExtension(theGraph, K33SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

    // Make a new graph to hold the root subgraph in the main planar embedding (i.e., theGraph)
    if ((rootSubgraph = gp_New()) == NULL)
        return NOTOK;

    // Transfer ownership of the root subgraph to the EOTreeRoot, in lieu of having it point to theGraph
    EOTreeRoot->subgraph = rootSubgraph;
    EOTreeRoot->subgraphOwner = TRUE;

    // Fully initialize the root subgraph and get its K_{3,3} search extension object
    if (gp_InitGraph(rootSubgraph, theGraph->N) != OK || gp_AttachK33Search(rootSubgraph) != OK)
        return NOTOK;

    gp_FindExtension(rootSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    // Set up the graph-to-subgraph mapping
    for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
    {
        context->VI[v].graphToSubgraphIndex = v;
        context->VI[v].subgraphToGraphIndex = v;
        subgraphContext->VI[v].subgraphToGraphIndex = v;
    }

    // Preserve in the rootSubgraph the knowledge of the vertices that were
    // made defunct in the main planar embedding during the embedding process
    // (due to ReduceBicomp() operations).
    for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
    {
        if (gp_GetVertexDefunct(theGraph, v))
            gp_SetVertexDefunct(rootSubgraph, v);
    }

    // Copy the non-virtual edges and any edges containing pointers to child O-nodes
    EsizeOccupied = gp_EdgeInUseIndexBound(theGraph);
    for (e = gp_GetFirstEdge(theGraph); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theGraph, e))
        {
            if (!gp_GetEdgeVirtual(theGraph, e) || context->E[e].EONode != NULL)
            {
                if (_K33Search_CopyEdgeToNewSubgraph(theGraph, context, e, rootSubgraph) != OK)
                    return NOTOK;
            }
        }
    }

    // Now planarize the rootSubgraph
    if (_K33Search_PlanarizeNewSubgraph(rootSubgraph) != OK)
        return NOTOK;

    return OK;
}

/********************************************************************
 _K33Search_ValidateEmbeddingObstructionTree()
 ********************************************************************/
int _K33Search_ValidateEmbeddingObstructionTree(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph)
{
    // 1. Validate the EO-tree's alternating levels of E-nodes and O-nodes
    // 2. Validate the structure of the O-nodes as K5s whose edges connecting to
    //    child E-nodes have the matching edge endpoints as their identified 2-cut
    //    (i.e., in first and second positions of the subgraph in this implementation)
    // 3. Validate that each E-node's subgraph is a planar embedding
    if (_K33Search_ValidateENodeSubtree(EOTreeRoot) != OK)
        return NOTOK;

    // 4. Test the bijection of the non-virtual edges of the embedding with the
    //    edges in the original graph (in linear time).
    if (_K33Search_ValidateEmbeddingObstructionTreeEdgeSet(theGraph, EOTreeRoot, origGraph) != OK)
        return NOTOK;

    // 5. Test the bijection between the vertices of the original graph and the
    //    vertices in the embedding that are (a) in E-nodes, (b) not the 2-cut
    //    vertices in the descendant E-node subgraphs, and (c) not defunct vertices.
    if (_K33Search_ValidateEmbeddingObstructionTreeVertexSet(theGraph, EOTreeRoot, origGraph) != OK)
        return NOTOK;

    return OK;
}

/********************************************************************
 _K33Search_ValidateENodeSubtree()

 Ensures that the node passed in as theRootENode is an E-node that
 has not been previously been visited and that its subgraph passes
 the planar graph facial integrity check.

 Then, the method to validate a subtree rooted by an O-node is called
 for each edge having an EONode pointer in the E-node's subgraph.

 The E-node type check combined with calling for O-node validation on
 all children helps validate that the embedding obstruction tree has
 alternating levels of E-nodes and O-nodes (because the O-node validation
 performs the analogous operations).

 The visitation test ensures that embedding-obstruction linked structure
 is indeed a tree (acyclic).
 ********************************************************************/
int _K33Search_ValidateENodeSubtree(K33Search_EONodeP theRootENode)
{
    int e, EsizeOccupied;
    graphP theSubgraph = theRootENode->subgraph;
    K33SearchContext *subgraphContext = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    if (theRootENode->EOType != K33SEARCH_EOTYPE_ENODE)
        return NOTOK;

    if (theRootENode->visited)
        return NOTOK;
    theRootENode->visited = TRUE;

    if (_CheckEmbeddingFacialIntegrity(theSubgraph) != OK)
        return NOTOK;

    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theSubgraph, e) && subgraphContext->E[e].EONode != NULL)
        {
            if (_K33Search_ValidateONodeSubtree(subgraphContext->E[e].EONode) != OK)
                return NOTOK;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_ValidateONodeSubtree()

 Ensures that the node passed in as theRootONode is an O-node that
 has not been previously been visited and that its subgraph passes
 the test of being a K5.

 Then, for each edge of the K5 having a non-NULL EONode pointer, two
 validations are performed. First, we validate that the subgraph of
 the child E-node pointed to by the edge's EONode pointer has its
 first two vertices (i.e., its 2-cut) representing the same two
 vertices as the edge in the K5 that points to it.  Second we
 invoke the E-node subtree validation on the non-NULL EONode.

 The O-node type check combined with calling for E-node validation on
 all children helps validate that the embedding obstruction tree has
 alternating levels of E-nodes and O-nodes (because the E-node validation
 performs the analogous operations).

 The visitation test ensures that embedding-obstruction linked structure
 is indeed a tree (acyclic).
 ********************************************************************/
int _K33Search_ValidateONodeSubtree(K33Search_EONodeP theRootONode)
{
    int e, EsizeOccupied;
    int degrees[5], imageVerts[6];
    graphP theSubgraph = theRootONode->subgraph;
    K33SearchContext *subgraphContext = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    if (theRootONode->EOType != K33SEARCH_EOTYPE_ONODE)
        return NOTOK;

    if (theRootONode->visited)
        return NOTOK;
    theRootONode->visited = TRUE;

    if (theSubgraph->N != 5 || theSubgraph->M != 10)
        return NOTOK;

    if (_getImageVertices(theSubgraph, degrees, 4, imageVerts, 6) != OK)
        return NOTOK;

    if (_TestForCompleteGraphObstruction(theSubgraph, 5, degrees, imageVerts) != TRUE)
        return NOTOK;

    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theSubgraph, e) && subgraphContext->E[e].EONode != NULL)
        {
            if (_K33Search_ValidateChildENodeConnection(theSubgraph, e, subgraphContext->E[e].EONode) != OK)
                return NOTOK;

            if (_K33Search_ValidateENodeSubtree(subgraphContext->E[e].EONode) != OK)
                return NOTOK;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_ValidateChildENodeConnection()
 ********************************************************************/
int _K33Search_ValidateChildENodeConnection(graphP theK5, int e, K33Search_EONodeP theChildENode)
{
    K33SearchContext *childSubgraphContext = NULL;
    int v, w, vInChild, wInChild, temp;

    // Reality check that theChildENode is an E-node
    if (theChildENode->EOType != K33SEARCH_EOTYPE_ENODE)
        return NOTOK;

    gp_FindExtension(theChildENode->subgraph, K33SEARCH_ID, (void *)&childSubgraphContext);
    if (childSubgraphContext == NULL)
        return NOTOK;

    // Get the subgraph to graph mappings of the first two vertices in the child E-node's subgraph,
    // as these are the vertices of the 2-cut that the E-node subgraph shares with the edge e
    // in the O-node's K5.
    vInChild = childSubgraphContext->VI[gp_GetFirstVertex(theChildENode->subgraph)].subgraphToGraphIndex;
    wInChild = childSubgraphContext->VI[gp_GetFirstVertex(theChildENode->subgraph) + 1].subgraphToGraphIndex;
    if (vInChild > wInChild)
    {
        temp = vInChild;
        vInChild = wInChild;
        wInChild = temp;
    }

    // Now get the two vertex endpoints for the edge in the O-node's K5,
    // and then get their subgraph to graph mappings
    v = gp_GetNeighbor(theK5, e);
    w = gp_GetNeighbor(theK5, gp_GetTwinArc(theK5, e));
    v = gp_GetVertexIndex(theK5, v);
    w = gp_GetVertexIndex(theK5, w);
    if (v > w)
    {
        temp = v;
        v = w;
        w = temp;
    }

    // Make sure the 2-cut matches in the O-node and E-node subgraphs
    if (v != vInChild || w != wInChild)
        return NOTOK;

    return OK;
}

/********************************************************************
 _K33Search_ValidateEmbeddingObstructionTreeEdgeSet()
 ********************************************************************/
int _K33Search_ValidateEmbeddingObstructionTreeEdgeSet(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph)
{
    int v;
    graphP graphOfEmbedding = gp_New();

    if (graphOfEmbedding == NULL || gp_InitGraph(graphOfEmbedding, origGraph->N) != OK)
    {
        gp_Free(&graphOfEmbedding);
        return NOTOK;
    }

    // Obtain all the non-virtual edges from the E-node subgraphs in the EO-tree
    // (all O-node edges are virtual with respect to the original graph)
    if (_K33Search_CopyEmbeddingEdgesToGraph(theGraph, EOTreeRoot, graphOfEmbedding) != OK)
    {
        gp_Free(&graphOfEmbedding);
        return NOTOK;
    }

    // Now the vertices must be reordered from a DFI interpretation in theGraph associated with the
    // EOTreeRoot to the original indexing in origGraph.

    // First, we copy the original non-DFI numbering from theGraph into the graphOfEmbedding
    for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
    {
        gp_SetVertexIndex(graphOfEmbedding, v, gp_GetVertexIndex(theGraph, v));
    }
    // Then we tell the graphOfEmbedding that it is DFSNUMBERED and SORTEDBYDFI so that the
    // vertex index values will be interpreted as the original non-DFI numbering by gp_SortVertices()
    graphOfEmbedding->internalFlags |= (FLAGS_DFSNUMBERED | FLAGS_SORTEDBYDFI);

    // Sort the graph of the embedding back to non-DFI order
    if (gp_SortVertices(graphOfEmbedding) != OK)
    {
        gp_Free(&graphOfEmbedding);
        return NOTOK;
    }

    // If the embedding and the original graph are each subgraphs of the
    // other, then they contain the same edges, so error if not both true
    if (_TestSubgraph(graphOfEmbedding, origGraph) != TRUE)
    {
        gp_Free(&graphOfEmbedding);
        return NOTOK;
    }

    if (_TestSubgraph(origGraph, graphOfEmbedding) != TRUE)
    {
        gp_Free(&graphOfEmbedding);
        return NOTOK;
    }

    gp_Free(&graphOfEmbedding);
    return OK;
}

/********************************************************************
 ********************************************************************/
int _K33Search_CopyEmbeddingEdgesToGraph(graphP theMainGraph, K33Search_EONodeP EONode, graphP graphOfEmbedding)
{
    int EsizeOccupied, e, v, w, vInGraph, wInGraph, eInGraph;
    graphP theSubgraph = EONode->subgraph;
    K33SearchContext *subgraphContext = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    // Now we run the edge array of the subgraph associated with the EONode to add all of the
    // in-use, non-virtual edges into the graph embedding.
    // NOTE: We could add a condition to only run this loop on E-node subgraphs because
    //       all edges in O-node subgraphs are marked virtual, but running this loop anyway
    //       does not affect the performance bound and makes sure the flags are set correctly
    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        // In the main embedding, edges are sometimes deleted, so we only process
        // the non-virtual edges that are in-use (not deleted)
        if (gp_EdgeInUse(theSubgraph, e) && !gp_GetEdgeVirtual(theSubgraph, e))
        {
            v = gp_GetNeighbor(theSubgraph, e);
            w = gp_GetNeighbor(theSubgraph, gp_GetTwinArc(theSubgraph, e));
            vInGraph = subgraphContext->VI[v].subgraphToGraphIndex;
            wInGraph = subgraphContext->VI[w].subgraphToGraphIndex;

            if (gp_AddEdge(graphOfEmbedding, vInGraph, 0, wInGraph, 0) != OK)
                return NOTOK;

            eInGraph = graphOfEmbedding->V[vInGraph].link[0];
            if (gp_GetNeighbor(graphOfEmbedding, eInGraph) != wInGraph)
                return NOTOK;
        }
    }

    // Now we run the edge array of the subgraph again to find the nodes to which we must
    // recursively descend, i.e., all in-use virtual and non-virtual edges that have an
    // EONode pointer are pointing to child EONodes to which we descend.
    // NOTE: In this implementation, it is not necessary and therefore not done to have
    //       an edge whose EONode points to the EONode parent of the current EONode.
    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        // In the main embedding, edges are sometimes deleted, so we only process
        // the non-virtual edges that are in-use (not deleted)
        if (gp_EdgeInUse(theSubgraph, e) && subgraphContext->E[e].EONode != NULL)
        {
            if (_K33Search_CopyEmbeddingEdgesToGraph(theMainGraph, subgraphContext->E[e].EONode, graphOfEmbedding) != OK)
                return NOTOK;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_ValidateEmbeddingObstructionTreeVertexSet()
 ********************************************************************/
int _K33Search_ValidateEmbeddingObstructionTreeVertexSet(graphP theGraph, K33Search_EONodeP EOTreeRoot, graphP origGraph)
{
    int v;

    _ClearVertexVisitedFlags(origGraph, FALSE);

    // Recursively descend into the EO-tree to mark visited the appropriate vertices in origGraph,
    // returning NOTOK if any is ever marked visited more than once. The FALSE parameter is used
    // to indicate that the main planar embedding (the root of the whole tree) is not separated
    // from an ancestor by a 2-cut, so there are no vertices to ignore in the main planar embedding.
    if (_K33Search_ValidateVerticesInENodeSubtree(theGraph, EOTreeRoot, FALSE, origGraph) != OK)
        return NOTOK;

    // Test that every vertex in origGraph was represented in the EO-tree embedding
    for (v = gp_GetFirstVertex(origraph); gp_VertexInRange(origGraph, v); v++)
        if (!gp_GetVertexVisited(origGraph, v))
            return NOTOK;

    // On success, put the origGraph vertices back to not visited and return a success result
    _ClearVertexVisitedFlags(origGraph, FALSE);
    return OK;
}

/********************************************************************
 _K33Search_ValidateVerticesInENodeSubtree()
 ********************************************************************/
int _K33Search_ValidateVerticesInENodeSubtree(graphP theGraph, K33Search_EONodeP theRootENode, int ignoreCutVertices, graphP origGraph)
{
    int v, v_offset, vInGraph;
    int e, EsizeOccupied;
    graphP theSubgraph = theRootENode->subgraph;
    K33SearchContext *subgraphContext = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    // In this implementation, the cut vertices of a descendant E-node subgraph
    // are stored  in the first two positions in the vertex array
    v_offset = ignoreCutVertices ? 2 : 0;
    for (v = gp_GetFirstVertex(theSubgraph) + v_offset; gp_VertexInRange(theSubgraph, v); v++)
    {
        // In the main planar embedding, vertices are marked defunct as they are sent off to
        // be represented by a descendant E-node's subgraph; those are excluded from the
        // visitation test-then-set logic
        if (!gp_GetVertexDefunct(theSubgraph, v))
        {
            vInGraph = subgraphContext->VI[v].subgraphToGraphIndex;
            if (gp_GetVertexVisited(origGraph, vInGraph))
                return NOTOK;
            gp_SetVertexVisited(origGraph, vInGraph);
        }
    }

    // Find all the O-nodes and recursively descend to them
    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theSubgraph, e) && subgraphContext->E[e].EONode != NULL)
        {
            if (_K33Search_ValidateVerticesInONodeSubtree(theGraph, subgraphContext->E[e].EONode, origGraph) != OK)
                return NOTOK;
        }
    }

    return OK;
}

/********************************************************************
 _K33Search_ValidateVerticesInONodeSubtree()
 ********************************************************************/
int _K33Search_ValidateVerticesInONodeSubtree(graphP theGraph, K33Search_EONodeP theRootONode, graphP origGraph)
{
    int e, EsizeOccupied;
    graphP theSubgraph = theRootONode->subgraph;
    K33SearchContext *subgraphContext = NULL;

    gp_FindExtension(theSubgraph, K33SEARCH_ID, (void *)&subgraphContext);
    if (subgraphContext == NULL)
        return NOTOK;

    // Find all the E-nodes and recursively descend to them
    EsizeOccupied = gp_EdgeInUseIndexBound(theSubgraph);
    for (e = gp_GetFirstEdge(theSubgraph); e < EsizeOccupied; e += 2)
    {
        if (gp_EdgeInUse(theSubgraph, e) && subgraphContext->E[e].EONode != NULL)
        {
            if (_K33Search_ValidateVerticesInENodeSubtree(theGraph, subgraphContext->E[e].EONode, TRUE, origGraph) != OK)
                return NOTOK;
        }
    }

    return OK;
}

#endif
