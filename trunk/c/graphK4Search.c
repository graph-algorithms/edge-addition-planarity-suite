/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
All rights reserved.
No part of this file can be copied or used for any reason without
the expressed written permission of the copyright holder.
*/

#include "graphK4Search.h"
#include "graphK4Search.private.h"

extern int K4SEARCH_ID;

#include "graph.h"

/* Imported functions */

extern void _FillVisitedFlags(graphP, int);
extern void _FillVisitedFlagsInBicomp(graphP theGraph, int BicompRoot, int FillValue);
extern void _FillVisitedFlagsInOtherBicomps(graphP theGraph, int BicompRoot, int FillValue);
extern void _FillVisitedFlagsInUnembeddedEdges(graphP theGraph, int FillValue);
extern int  _GetBicompSize(graphP theGraph, int BicompRoot);
extern void _HideInternalEdges(graphP theGraph, int vertex);
extern void _RestoreInternalEdges(graphP theGraph);
extern void _DeleteUnmarkedEdgesInBicomp(graphP theGraph, int BicompRoot);
extern void _ClearInvertedFlagsInBicomp(graphP theGraph, int BicompRoot);

extern int  _GetNextVertexOnExternalFace(graphP theGraph, int curVertex, int *pPrevLink);
extern int  _JoinBicomps(graphP theGraph);
extern void _OrientVerticesInBicomp(graphP theGraph, int BicompRoot, int PreserveSigns);
extern void _OrientVerticesInEmbedding(graphP theGraph);
extern void _InvertVertex(graphP theGraph, int V);

extern int  _MarkHighestXYPath(graphP theGraph);

extern int  _FindUnembeddedEdgeToCurVertex(graphP theGraph, int cutVertex, int *pDescendant);
extern int  _FindUnembeddedEdgeToSubtree(graphP theGraph, int ancestor, int SubtreeRoot, int *pDescendant);

extern int  _MarkPathAlongBicompExtFace(graphP theGraph, int startVert, int endVert);

extern int  _AddAndMarkEdge(graphP theGraph, int ancestor, int descendant);

extern int  _DeleteUnmarkedVerticesAndEdges(graphP theGraph);

extern int  _GetLeastAncestorConnection(graphP theGraph, int cutVertex);
extern int  _MarkDFSPathsToDescendants(graphP theGraph);
extern int  _AddAndMarkUnembeddedEdges(graphP theGraph);

extern int  _ChooseTypeOfNonOuterplanarityMinor(graphP theGraph, int I, int R);
extern int  _IsolateOuterplanarityObstructionE(graphP theGraph);

/* Private functions for K4 searching (exposed to the extension). */

int  _SearchForK4(graphP theGraph, int I);

int  _SearchForK4InBicomp(graphP theGraph, K4SearchContext *context, int I, int R);

/* Private functions for K4 searching. */

int  _K4_FindSecondActiveVertexOnLowExtFacePath(graphP theGraph);
int  _K4_ReduceBicompToEdge(graphP theGraph, int R);

int  _K4_FinishIsolatorContextInitialization(graphP theGraph, K4SearchContext *context);
int  _K4_ReduceExternalFacePathToEdge(graphP theGraph, K4SearchContext *context, int u, int x, int edgeType);
int  _K4_ReduceXYPathToEdge(graphP theGraph, K4SearchContext *context, int u, int x, int edgeType);
int  _K4_RestoreReducedPath(graphP theGraph, K4SearchContext *context, int J);
int  _K4_RestoreAndOrientReducedPaths(graphP theGraph, K4SearchContext *context);
int  _K4_SetEdgeType(graphP theGraph, int u, int v);
int  _K4_OrientPath(graphP theGraph, int u, int v, int w, int x);
void _K4_SetVisitedOnPath(graphP theGraph, int u, int v, int w, int x, int visited);
int  _K4_FindExternalConnectionDescendantEndpoint(graphP theGraph, int ancestor,
                                                  int cutVertex, int *pDescendant);

int  _K4_IsolateMinorA1(graphP theGraph);
int  _K4_IsolateMinorA2(graphP theGraph);
int  _K4_IsolateMinorB1(graphP theGraph);
int  _K4_IsolateMinorB2(graphP theGraph);


/****************************************************************************
 _SearchForK4()
 ****************************************************************************/

int  _SearchForK4(graphP theGraph, int I)
{
int  C1, C2, D, e, RetVal=OK, FoundOne;
K4SearchContext *context = NULL;

    gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);
    if (context == NULL)
        return NOTOK;

     /* Each DFS child is listed in DFI order in V[I].sortedDFSChildList.
        In V[I].fwdArcList, the forward arcs of all unembedded back edges are
        in order by DFI of the descendant endpoint of the edge.

        DFS descendants have a higher DFI than ancestors, so given two
        successive children C1 and C2, if any forward arcs lead to a
        vertex D such that DFI(C1) < DFI(D) < DFI(C2), then the Walkdown
        failed to embed a back edge from I to a descendant D of C1. */

     e = theGraph->V[I].fwdArcList;
     D = theGraph->G[e].v;

     C1 = context->V[I].sortedDFSChildList;

     FoundOne = FALSE;

     while (C1 != NIL && e != NIL)
     {
        C2 = LCGetNext(context->sortedDFSChildLists,
                       context->V[I].sortedDFSChildList, C1);

        // If the edge e leads from I to a descendant D of C1,
        // then D will be less than C2 (as explained above),
        // so we search for a K_4 in the bicomp rooted
        // by the root copy of I associated with C1.
        // (If C2 is NIL, then C1 is the last child)

        if (D < C2 || C2 == NIL)
        {
        	FoundOne = TRUE;
            RetVal = _SearchForK4InBicomp(theGraph, context, I, C1+theGraph->N);

            // If something went wrong, NOTOK was returned;
            // If a K_4 was found, NONEMBEDDABLE was returned;
            // If OK was returned, then only a K_{2,3} was found
            // (and reduced to a non-obstruction), so we continue
            // searching any other bicomps on which the Walkdown failed.

            if (RetVal != OK)
             break;
        }

        // Skip the edges that lead to descendants of C1 to get to those
        // edges that lead to descendants of C2.

        if (C2 != NIL)
        {
            while (D < C2 && gp_IsArc(theGraph, e))
            {
                e = gp_GetNextArc(theGraph, e);
                if (e == theGraph->V[I].fwdArcList)
                     e = NIL;
                else D = theGraph->G[e].v;
            }
        }

        // Move the DFS child context to C2
        C1 = C2;
     }

/* If we got through the loop with an OK value for each bicomp on
     which the Walkdown failed, then we return OK to indicate that only
     K_{2,3}'s were found. The OK return allows the embedder to continue.

     NOTE: The variable FoundOne helps ensure we detect at least one
        bicomp on which the Walkdown failed (this should always be
        the case in an error-free implementation like this one!). */

     return FoundOne ? RetVal : NOTOK;
}

/****************************************************************************
 _SearchForK4InBicomp()
 ****************************************************************************/

int  _SearchForK4InBicomp(graphP theGraph, K4SearchContext *context, int I, int R)
{
isolatorContextP IC = &theGraph->IC;

	if (context == NULL)
	{
		gp_FindExtension(theGraph, K4SEARCH_ID, (void *)&context);
		if (context == NULL)
			return NOTOK;
	}

	// TO DO
	//Can't really do this because orients the whole bicomp, but we
	//can't touch the whole bicomp in the B case

	// Begin by determining whether minor A, B or E is detected
	if (_ChooseTypeOfNonOuterplanarityMinor(theGraph, I, R) != OK)
		return NOTOK;

	// Minor E indicates the desired K4 homeomorph, so we isolate it and return NONEMBEDDABLE
    if (theGraph->IC.minorType & MINORTYPE_E)
    {
    	// Restore the orientations of the vertices in the bicomp, which were
    	// adjusted during the invocation of ChooseTypeOfNonOuterplanarityMinor()
        _OrientVerticesInBicomp(theGraph, R, 1);

        // Now impose consistent orientation on the embedding so we can then
        // restore the reduced paths.
        _OrientVerticesInEmbedding(theGraph);
        if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
            return NOTOK;

        // Set up to isolate minor E
        if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
            return NOTOK;

        if (_MarkHighestXYPath(theGraph) != TRUE)
             return NOTOK;

        // Mark the vertices and edges of the K4 homeomorph
        if (_IsolateOuterplanarityObstructionE(theGraph) != OK)
            return NOTOK;

        // Remove edges not in the K4 homeomorph
        if (_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
            return NOTOK;

        // Return indication that K4 homeomorph has been found
        return NONEMBEDDABLE;
    }

    // Minors A and B indicate the existence of K_{2,3} homeomorphs, but
    // we run additional tests to see whether we can either find an
    // entwined K4 homeomorph or reduce part of the graph so that the
    // outerplanarity method can resolve more of the pertinence in
    // the current iteration
    if (theGraph->IC.minorType & MINORTYPE_A)
    {
    	// Case A1: Test whether there is an active vertex Z other than W
    	// along the external face path [X, ..., W, ..., Y]
    	if (_K4_FindSecondActiveVertexOnLowExtFacePath(theGraph) == TRUE)
    	{
        	// Restore the orientations of the vertices in the bicomp, which were
        	// adjusted during the invocation of ChooseTypeOfNonOuterplanarityMinor()
            _OrientVerticesInBicomp(theGraph, R, 1);

            // Now impose consistent orientation on the embedding so we can then
            // restore the reduced paths.
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA1(theGraph) != OK)
    			return NOTOK;

            // Remove edges not in the K4 homeomorph
            if (_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
                return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
    	}

    	// Case A2: Test whether the bicomp has an XY path
        if (_MarkHighestXYPath(theGraph) == TRUE)
        {
        	// Restore the orientations of the vertices in the bicomp, which were
        	// adjusted during the invocation of ChooseTypeOfNonOuterplanarityMinor()
            _OrientVerticesInBicomp(theGraph, R, 1);

            // Now impose consistent orientation on the embedding so we can then
            // restore the reduced paths.
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA2(theGraph) != OK)
    			return NOTOK;

            // Remove edges not in the K4 homeomorph
            if (_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
                return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
        }

        // Since neither A1 nor A2 is found, then we reduce the bicomp
        // to the edge (R, W) then
    	if (_K4_ReduceBicompToEdge(theGraph, R) != OK)
    		return NOTOK;

        // Return OK so that the WalkDown can continue resolving the pertinence of I.
    	return OK;
    }
    else if (theGraph->IC.minorType & MINORTYPE_B)
    {
    	// TO DO: Patterns for finding K4;
    	// if found, isolate K4, return NONEMBEDDABLE
    	// else return OK;
    	// To DO: Returning OK is the way to get the embedder to proceed to the next
    	// iteration, but we actually need to continue the *WalkDown* on the bicomp
    	return NOTOK;
    }

    // You never get here in an error-free implementation like this one
    return NOTOK;
}

/****************************************************************************
 _FindSecondActiveVertexOnLowExtFacePath()
 This method determines whether there is an active vertex Z other than W on
 the path [X, ..., W, ..., Y].  By active, we mean a vertex that connects
 by an unembedded edge to either I or an ancestor of I.  That is, a vertext
 that is pertinent or future pertinent (would be pertinent in a future step
 of the embedder).
 In the core planarity embedder, future pertinence and external activity are
 the same, so we temporarily flip to the planar embedder flags as a quick way
 to get the right behavior out of _VertexActiveStatus().
 ****************************************************************************/
int _K4_FindSecondActiveVertexOnLowExtFacePath(graphP theGraph)
{
    int Z = theGraph->IC.x, ZPrevLink=1;

	// First we test X for future pertinence only (if it were pertinent, then
	// we wouldn't have been blocked up on this bicomp)
    if (FUTUREPERTINENT(theGraph, Z, theGraph->IC.v))
	{
		theGraph->IC.z = Z;
		theGraph->IC.uz = _GetLeastAncestorConnection(theGraph, Z);
		return TRUE;
	}

	// Now we move on to test all the vertices strictly between X and Y on
	// the lower external face path, except W, for either pertinence or
	// future pertinence.
	Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);

	while (Z != theGraph->IC.py)
	{
		if (Z != theGraph->IC.w)
		{
		    if (FUTUREPERTINENT(theGraph, Z, theGraph->IC.v))
			{
				theGraph->IC.z = Z;
				theGraph->IC.uz = _GetLeastAncestorConnection(theGraph, Z);
				return TRUE;
			}
			else if (PERTINENT(theGraph, Z))
			{
				theGraph->IC.z = Z;
				theGraph->IC.uz = theGraph->IC.v;
				return TRUE;
			}
		}

		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
	}

	// Now we test Y for future pertinence (same explanation as for X above)
    if (FUTUREPERTINENT(theGraph, Z, theGraph->IC.v))
	{
		theGraph->IC.z = Z;
		theGraph->IC.uz = _GetLeastAncestorConnection(theGraph, Z);
		return TRUE;
	}

	// We didn't find the desired second vertex, so report FALSE
	return FALSE;
}

/****************************************************************************
 ****************************************************************************/

int  _K4_ReduceBicompToEdge(graphP theGraph, int R)
{
	// Rather than reducing just a path, we need to exploit
	// the fact that (R, W) is a 2-cut.  Hook the adj list
	// of R into a circular list and the adj list of W into
	// a circular list.  The connectors of R and W can then
	// point to those lists.  Then we just need a new edge
	// for (R, W).
	// Only problem here is that ideally we would like to have
	// net zero growth of edges. It's not strictly necessary
	// for maintaining a linear bound, but from an implementation
	// standpoint, it avoids having to expand the arc capacity.
	// implementation standpoint.
	return NOTOK;
}

int  _K4_IsolateMinorA1(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

int  _K4_IsolateMinorA2(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

int  _K4_IsolateMinorB1(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

int  _K4_IsolateMinorB2(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

/****************************************************************************
 _K4_FinishIsolatorContextInitialization()
 Once it has been decided that a desired subgraph can be isolated, it
 becomes safe to finish the isolator context initialization.
 IMPLEMENTATION NOT CHECKED FOR CORRECTNESS
 ****************************************************************************/

int  _K4_FinishIsolatorContextInitialization(graphP theGraph, K4SearchContext *context)
{
isolatorContextP IC = &theGraph->IC;

/* Restore the orientation of the bicomp on which we're working, then
    perform orientation of all vertices in graph. (An unnecessary but
    polite step that simplifies the description of key states of the
    data structures). */

     _OrientVerticesInBicomp(theGraph, IC->r, 1);
     _OrientVerticesInEmbedding(theGraph);

/* Restore any paths that were reduced to single edges */

     if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
         return NOTOK;

/* We assume that the current bicomp has been marked appropriately,
     but we must now clear the visitation flags of all other bicomps. */

     _FillVisitedFlagsInOtherBicomps(theGraph, IC->r, 0);

/* To complete the normal behavior of _FillVisitedFlags() in the
    normal isolator context initialization, we also have to clear
    the visited flags on all edges that have not yet been embedded */

     _FillVisitedFlagsInUnembeddedEdges(theGraph, 0);

/* Now we can find the descendant ends of unembedded back edges based on
     the ancestor settings ux, uy and uz. */

     if (_K4_FindExternalConnectionDescendantEndpoint(theGraph, IC->ux, IC->x, &IC->dx) != OK ||
         _K4_FindExternalConnectionDescendantEndpoint(theGraph, IC->uy, IC->y, &IC->dy) != OK ||
         _K4_FindExternalConnectionDescendantEndpoint(theGraph, IC->uz, IC->z, &IC->dz) != OK)
         return NOTOK;

/* Finally, we obtain the descendant end of an unembedded back edge to
     the current vertex. */

     if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _K4_FindExternalConnectionDescendantEndpoint()
 Returns OK if it finds that either the given cutVertex or one of its
    descendants in a separated bicomp has an unembedded back edge
    connection to the given ancestor vertex.
 Returns NOTOK otherwise (it is an error to not find the descendant because
    this function is only called if _SearchForDescendantExternalConnection()
    has already determined the existence of the descendant).
  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_FindExternalConnectionDescendantEndpoint(graphP theGraph, int ancestor,
                                               int cutVertex, int *pDescendant)
{
int  listHead, child, J;

     // Check whether the cutVertex is directly adjacent to the ancestor
     // by an unembedded back edge.

     J = theGraph->V[ancestor].fwdArcList;
     while (gp_IsArc(theGraph, J))
     {
         if (theGraph->G[J].v == cutVertex)
         {
             *pDescendant = cutVertex;
             return OK;
         }

         J = gp_GetNextArc(theGraph, J);
         if (J == theGraph->V[ancestor].fwdArcList)
             J = NIL;
     }

     // Now check the descendants of the cut vertex to see if any make
     // a connection to the ancestor.
     listHead = child = theGraph->V[cutVertex].separatedDFSChildList;
     while (child != NIL)
     {
         if (theGraph->V[child].Lowpoint >= theGraph->IC.v)
             break;

         if (_FindUnembeddedEdgeToSubtree(theGraph, ancestor, child, pDescendant) == TRUE)
             return OK;

         child = LCGetNext(theGraph->DFSChildLists, listHead, child);
     }

     return NOTOK;
}

/****************************************************************************
 _K4_ComputeArcType()
 This is just a little helper function that automates a sequence of decisions
 that has to be made a number of times.
 An edge record is being added to the adjacency list of a; it indicates that
 b is a neighbor.  The edgeType can be either 'tree' (EDGE_DFSPARENT) or
 'cycle' (EDGE_BACK).  If a or b is a root copy, we translate to the
 non-virtual counterpart, then determine which has the lesser DFI.  If a
 has the lower DFI then the edge record is a tree edge to a child
 (EDGE_DFSCHILD) if edgeType indicates a tree edge.  If edgeType indicates a
 cycle edge, then it is a forward cycle edge (EDGE_FORWARD) to a descendant.
 Symmetric conditions define the types for a > b.

  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL

 ****************************************************************************/

int  _K4_ComputeArcType(graphP theGraph, int a, int b, int edgeType)
{
     if (a >= theGraph->N)
         a = theGraph->V[a - theGraph->N].DFSParent;
     if (b >= theGraph->N)
         b = theGraph->V[b - theGraph->N].DFSParent;

     if (a < b)
         return edgeType == EDGE_DFSPARENT ? EDGE_DFSCHILD : EDGE_FORWARD;

     return edgeType == EDGE_DFSPARENT ? EDGE_DFSPARENT : EDGE_BACK;
}

/****************************************************************************
 _K4_ReduceExternalFacePathToEdge()
  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_ReduceExternalFacePathToEdge(graphP theGraph, K4SearchContext *context, int u, int x, int edgeType)
{
int  prevLink, v, w, e;

     /* If the path is a single edge, then no need for a reduction */

     prevLink = 1;
     v = _GetNextVertexOnExternalFace(theGraph, u, &prevLink);
     if (v == x)
         return OK;

     /* We have the endpoints u and x of the path, and we just computed the
        first vertex internal to the path and a neighbor of u.  Now we
        compute the vertex internal to the path and a neighbor of x. */

     prevLink = 0;
     w = _GetNextVertexOnExternalFace(theGraph, x, &prevLink);

     /* Delete the two edges that connect the path to the bicomp.
        If either edge is a reduction edge, then we have to restore
        the path it represents. We can only afford to visit the
        endpoints of the path.
        Note that in the restored path, the edge incident to each
        endpoint of the original path is a newly added edge,
        not a reduction edge. */

     e = gp_GetFirstArc(theGraph, u);
     if (context->G[e].pathConnector != NIL)
     {
         if (_K4_RestoreReducedPath(theGraph, context, e) != OK)
             return NOTOK;
         e = gp_GetFirstArc(theGraph, u);
         v = theGraph->G[e].v;
     }
     gp_DeleteEdge(theGraph, e, 0);

     e = gp_GetLastArc(theGraph, x);
     if (context->G[e].pathConnector != NIL)
     {
         if (_K4_RestoreReducedPath(theGraph, context, e) != OK)
             return NOTOK;
         e = gp_GetLastArc(theGraph, x);
         w = theGraph->G[e].v;
     }
     gp_DeleteEdge(theGraph, e, 0);

     /* Add the reduction edge, then set its path connectors so the original
        path can be recovered and set the edge type so the essential structure
        of the DFS tree can be maintained (The 'Do X to Bicomp' functions
        and functions like MarkDFSPath(0 depend on this). */

     gp_AddEdge(theGraph, u, 0, x, 1);

     e = gp_GetFirstArc(theGraph, u);
     context->G[e].pathConnector = v;
     theGraph->G[e].type = _K4_ComputeArcType(theGraph, u, x, edgeType);

     e = gp_GetLastArc(theGraph, x);
     context->G[e].pathConnector = w;
     theGraph->G[e].type = _K4_ComputeArcType(theGraph, x, u, edgeType);

     /* Set the external face info */

     theGraph->extFace[u].vertex[0] = x;
     theGraph->extFace[x].vertex[1] = u;

     return OK;
}

/****************************************************************************
 _K4_ReduceXYPathToEdge()
  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_ReduceXYPathToEdge(graphP theGraph, K4SearchContext *context, int u, int x, int edgeType)
{
int  e, v, w;

     e = gp_GetFirstArc(theGraph, u);
     e = gp_GetNextArc(theGraph, e);
     v = theGraph->G[e].v;

     /* If the XY-path is a single edge, then no reduction is needed */

     if (v == x)
         return OK;

     /* Otherwise, remove the two edges that join the XY-path to the bicomp */

     if (context->G[e].pathConnector != NIL)
     {
         if (_K4_RestoreReducedPath(theGraph, context, e) != OK)
             return NOTOK;
         e = gp_GetFirstArc(theGraph, u);
         e = gp_GetNextArc(theGraph, e);
         v = theGraph->G[e].v;
     }
     gp_DeleteEdge(theGraph, e, 0);

     e = gp_GetFirstArc(theGraph, x);
     e = gp_GetNextArc(theGraph, e);
     w = theGraph->G[e].v;
     if (context->G[e].pathConnector != NIL)
     {
         if (_K4_RestoreReducedPath(theGraph, context, e) != OK)
             return NOTOK;
         e = gp_GetFirstArc(theGraph, x);
         e = gp_GetNextArc(theGraph, e);
         w = theGraph->G[e].v;
     }
     gp_DeleteEdge(theGraph, e, 0);

     /* Now add a single edge to represent the XY-path */
     gp_InsertEdge(theGraph, u, gp_GetFirstArc(theGraph, u), 0,
    		                 x, gp_GetFirstArc(theGraph, x), 0);

     /* Now set up the path connectors so the original XY-path can be recovered if needed.
        Also, set the reduction edge's type to preserve the DFS tree structure */

     e = gp_GetFirstArc(theGraph, u);
     e = gp_GetNextArc(theGraph, e);
     context->G[e].pathConnector = v;
     theGraph->G[e].type = _K4_ComputeArcType(theGraph, u, x, edgeType);

     e = gp_GetFirstArc(theGraph, x);
     e = gp_GetNextArc(theGraph, e);
     context->G[e].pathConnector = w;
     theGraph->G[e].type = _K4_ComputeArcType(theGraph, x, u, edgeType);

     return OK;
}

/****************************************************************************
 _K4_RestoreReducedPath()

 Given an edge record of an edge used to reduce a path, we want to restore
 the path in constant time.
 The path may contain more reduction edges internally, but we do not
 search for and process those since it would violate the constant time
 bound required of this function.
 return OK on success, NOTOK on failure
  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_RestoreReducedPath(graphP theGraph, K4SearchContext *context, int J)
{
int  JTwin, u, v, w, x;
int  J0, J1, JTwin0, JTwin1;

     if (context->G[J].pathConnector == NIL)
         return OK;

     JTwin = gp_GetTwinArc(theGraph, J);

     u = theGraph->G[JTwin].v;
     v = context->G[J].pathConnector;
     w = context->G[JTwin].pathConnector;
     x = theGraph->G[J].v;

     /* Get the locations of the graph nodes between which the new
        graph nodes must be added in order to reconnect the path
        parallel to the edge. */

     J0 = gp_GetNextArc(theGraph, J);
     J1 = gp_GetPrevArc(theGraph, J);
     JTwin0 = gp_GetNextArc(theGraph, JTwin);
     JTwin1 = gp_GetPrevArc(theGraph, JTwin);

     /* We first delete the edge represented by J and JTwin. We do so before
        restoring the path to ensure we do not exceed the maximum arc capacity. */

     gp_DeleteEdge(theGraph, J, 0);

     /* Now we add the two edges to reconnect the reduced path represented
        by the edge [J, JTwin].  The edge record in u is added between J0 and J1.
        Likewise, the new edge record in x is added between JTwin0 and JTwin1. */

     if (gp_IsArc(theGraph, J0))
     {
    	 if (gp_InsertEdge(theGraph, u, J0, 1, v, gp_AdjacencyListEndMark(v), 0) != OK)
    		 return NOTOK;
     }
     else
     {
    	 if (gp_InsertEdge(theGraph, u, J1, 0, v, gp_AdjacencyListEndMark(v), 0) != OK)
    		 return NOTOK;
     }

     if (gp_IsArc(theGraph, JTwin0))
     {
    	 if (gp_InsertEdge(theGraph, x, JTwin0, 1, w, gp_AdjacencyListEndMark(w), 0) != OK)
    		 return NOTOK;
     }
     else
     {
    	 if (gp_InsertEdge(theGraph, x, JTwin1, 0, w, gp_AdjacencyListEndMark(w), 0) != OK)
    		 return NOTOK;
     }

     /* Set the types of the newly added edges */

     if (_K4_SetEdgeType(theGraph, u, v) != OK ||
         _K4_SetEdgeType(theGraph, w, x) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _K4_RestoreAndOrientReducedPaths()

 This function searches the embedding for any edges that are specially marked
 as being representative of a path that was previously reduced to a
 single edge by _ReduceBicomp().  The edge is replaced by the path.
 Note that the new path may contain more reduction edges, and these will be
 iteratively expanded by the outer for loop.

 If the edge records of an edge being expanded are the first or last arcs
 of the edge's vertex endpoints, then the edge may be along the external face.
 If so, then the vertices along the path being restored must be given a
 consistent orientation with the endpoints.  It is expected that the embedding
 will have been oriented prior to this operation.
  NOT CHECKED FOR CORRECTNESS OR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_RestoreAndOrientReducedPaths(graphP theGraph, K4SearchContext *context)
{
int  e, J, JTwin, u, v, w, x, visited;
int  J0, JTwin0, J1, JTwin1;

     for (e = 0; e < theGraph->M + sp_GetCurrentSize(theGraph->edgeHoles);)
     {
         J = theGraph->edgeOffset + 2*e;
         if (context->G[J].pathConnector != NIL)
         {
             visited = theGraph->G[J].visited;

             JTwin = gp_GetTwinArc(theGraph, J);
             u = theGraph->G[JTwin].v;
             v = context->G[J].pathConnector;
             w = context->G[JTwin].pathConnector;
             x = theGraph->G[J].v;

             /* Now we need the predecessor and successor edge records
                of J and JTwin.  The edge (u, v) will be inserted so
                that the record in u's adjacency list that indicates v
                will be between J0 and J1.  Likewise, the edge record
                (x -> w) will be placed between JTwin0 and JTwin1. */

             J0 = gp_GetNextArc(theGraph, J);
             J1 = gp_GetPrevArc(theGraph, J);
             JTwin0 = gp_GetNextArc(theGraph, JTwin);
             JTwin1 = gp_GetPrevArc(theGraph, JTwin);

             /* We first delete the edge represented by J and JTwin. We do so before
                restoring the path to ensure we do not exceed the maximum arc capacity. */

             gp_DeleteEdge(theGraph, J, 0);

             /* Now we add the two edges to reconnect the reduced path represented
                by the edge [J, JTwin].  The edge record in u is added between J0 and J1.
                Likewise, the new edge record in x is added between JTwin0 and JTwin1. */

             if (gp_IsArc(theGraph, J0))
             {
            	 if (gp_InsertEdge(theGraph, u, J0, 1, v, gp_AdjacencyListEndMark(v), 0) != OK)
            		 return NOTOK;
             }
             else
             {
            	 if (gp_InsertEdge(theGraph, u, J1, 0, v, gp_AdjacencyListEndMark(v), 0) != OK)
            		 return NOTOK;
             }

             if (gp_IsArc(theGraph, JTwin0))
             {
            	 if (gp_InsertEdge(theGraph, x, JTwin0, 1, w, gp_AdjacencyListEndMark(w), 0) != OK)
            		 return NOTOK;
             }
             else
             {
            	 if (gp_InsertEdge(theGraph, x, JTwin1, 0, w, gp_AdjacencyListEndMark(w), 0) != OK)
            		 return NOTOK;
             }

             /* Set the types of the newly added edges */

             if (_K4_SetEdgeType(theGraph, u, v) != OK ||
                 _K4_SetEdgeType(theGraph, w, x) != OK)
                 return NOTOK;

             /* We determine whether the reduction edge may be on the external face,
                in which case we will need to ensure that the vertices on the path
                being restored are consistently oriented.  This will accommodate
                future invocations of MarkPathAlongBicompExtFace().
                Note: If J0, J1, JTwin0 or JTwin1 is not an edge, then it is
                      because we've walked off the end of the edge record list,
                      which happens when J and JTwin are either the first or
                      last edge of the containing vertex.  In turn, the first
                      and last edges of a vertex are the ones that hold it onto
                      the external face, if it is on the external face. */

             if ((!gp_IsArc(theGraph, J0) && !gp_IsArc(theGraph, JTwin1)) ||
                 (!gp_IsArc(theGraph, J1) && !gp_IsArc(theGraph, JTwin0)))
             {
                 if (_K4_OrientPath(theGraph, u, v, w, x) != OK)
                     return NOTOK;
             }

             /* The internal XY path was already marked as part of the decision logic
                that made us decide we could find a K3,3 and hence that we should
                reverse all of the reductions.  Subsequent code counts on the fact
                that the X-Y path is already marked, so if we replace a marked edge
                with a path, then we need to mark the path. Similarly, for an unmarked
                edge, the replacement path should be unmarked. */

             _K4_SetVisitedOnPath(theGraph, u, v, w, x, visited);
         }
         else e++;
     }

     return OK;
}

/****************************************************************************
 _K4_SetEdgeType()
 When we are restoring an edge, we must restore its type.  We can deduce
 what the type was based on other information in the graph.
 NOT CHECKED FOR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

int  _K4_SetEdgeType(graphP theGraph, int u, int v)
{
int  e, eTwin, u_orig, v_orig, N;

     // If u or v is a virtual vertex (a root copy), then get the non-virtual counterpart.

     N = theGraph->N;
     u_orig = u < N ? u : (theGraph->V[u - N].DFSParent);
     v_orig = v < N ? v : (theGraph->V[v - N].DFSParent);

     // Get the edge for which we will set the type

     e = gp_GetNeighborEdgeRecord(theGraph, u, v);
     eTwin = gp_GetTwinArc(theGraph, e);

     // If u_orig is the parent of v_orig, or vice versa, then the edge is a tree edge

     if (theGraph->V[v_orig].DFSParent == u_orig ||
         theGraph->V[u_orig].DFSParent == v_orig)
     {
         if (u_orig > v_orig)
         {
             theGraph->G[e].type = EDGE_DFSPARENT;
             theGraph->G[eTwin].type = EDGE_DFSCHILD;
         }
         else
         {
             theGraph->G[eTwin].type = EDGE_DFSPARENT;
             theGraph->G[e].type = EDGE_DFSCHILD;
         }
     }

     // Otherwise it is a back edge

     else
     {
         if (u_orig > v_orig)
         {
             theGraph->G[e].type = EDGE_BACK;
             theGraph->G[eTwin].type = EDGE_FORWARD;
         }
         else
         {
             theGraph->G[eTwin].type = EDGE_BACK;
             theGraph->G[e].type = EDGE_FORWARD;
         }
     }

     return OK;
}

/****************************************************************************
 _K4_OrientPath()
 NOT CHECKED FOR CORRECNTESS OR SUITABILITY
 ****************************************************************************/

int  _K4_OrientPath(graphP theGraph, int u, int v, int w, int x)
{
int  e_u = gp_GetNeighborEdgeRecord(theGraph, u, v);
int  e_v, e_ulink, e_vlink;

    do {
        // Get the external face link in vertex u that indicates the
        // edge e_u which connects to the next vertex v in the path
    	// As a sanity check, we determine whether e_u is an
    	// external face edge, because there would be an internal
    	// implementation error if not
    	if (gp_GetFirstArc(theGraph, u) == e_u)
    		e_ulink = 0;
    	else if (gp_GetLastArc(theGraph, u) == e_u)
    		e_ulink = 1;
    	else return NOTOK;

        v = theGraph->G[e_u].v;

        // Now get the external face link in vertex v that indicates the
        // edge e_v which connects back to the prior vertex u.
        e_v = gp_GetTwinArc(theGraph, e_u);

    	if (gp_GetFirstArc(theGraph, v) == e_v)
    		e_vlink = 0;
    	else if (gp_GetLastArc(theGraph, v) == e_v)
    		e_vlink = 1;
    	else return NOTOK;

        // The vertices u and v are inversely oriented if they
        // use the same link to indicate the edge [e_u, e_v].
        if (e_vlink == e_ulink)
        {
            _InvertVertex(theGraph, v);
            e_vlink = 1^e_vlink;
        }

        theGraph->extFace[u].vertex[e_ulink] = v;
        theGraph->extFace[v].vertex[e_vlink] = u;

        u = v;
        e_u = gp_GetArc(theGraph, v, 1^e_vlink);
    } while (u != x);

    return OK;
}

/****************************************************************************
 _K4_SetVisitedOnPath()
  NOT CHECKED FOR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
 ****************************************************************************/

void _K4_SetVisitedOnPath(graphP theGraph, int u, int v, int w, int x, int visited)
{
int  e = gp_GetNeighborEdgeRecord(theGraph, u, v);

     theGraph->G[v].visited = visited;

     do {
         v = theGraph->G[e].v;
         theGraph->G[v].visited = visited;
         theGraph->G[e].visited = visited;
         e = gp_GetTwinArc(theGraph, e);
         theGraph->G[e].visited = visited;

         e = gp_GetNextArcCircular(theGraph, e);
     } while (v != x);
}
