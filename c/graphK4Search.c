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
extern void _FindActiveVertices(graphP theGraph, int R, int *pX, int *pY);
extern int  _JoinBicomps(graphP theGraph);
extern void _OrientVerticesInBicomp(graphP theGraph, int BicompRoot, int PreserveSigns);
extern void _OrientVerticesInEmbedding(graphP theGraph);
extern void _InvertVertex(graphP theGraph, int V);

extern void _SetVertexTypesForMarkingXYPath(graphP theGraph);
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

int  _K4_ChooseTypeOfNonOuterplanarityMinor(graphP theGraph, int I, int R);

int  _K4_FindSecondActiveVertexOnLowExtFacePath(graphP theGraph);
int  _K4_ReduceBicompToEdge(graphP theGraph);
int  _K4_FindPlanarityActiveVertex(graphP theGraph, int I, int R, int prevLink, int *pW);
int  _K4_FindSeparatingInternalEdge(theGraph, int R, int W, int prevLink, int *pW, int *pX, int *pY);

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

            // TO DO: if a reduction occurs, we actually need to do more
            // walking down *on that bicomp*, which may remove some of the
            // edges from the fwdArcList on which we're iterating.
            // This routine is only getting called on B and E, not A
            // Rename to accommodate
            // Need to keep hammering on a given bicomp until the front of the
            // list is empty or has edges to another bicomp.  Then, the bicomp
            // pertinence is finished.  Only other reason to stop is an
            // identified K4.

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

	// Begin by determining whether minor A, B or E is detected
	if (_K4_ChooseTypeOfNonOuterplanarityMinor(theGraph, I, R) != OK)
		return NOTOK;

    // Minor A indicates the existence of K_{2,3} homeomorphs, but
    // we run additional tests to see whether we can either find an
    // entwined K4 homeomorph or reduce the bicomp so that the WalkDown
	// is enabled to continue to resolve pertinence
    if (theGraph->IC.minorType & MINORTYPE_A)
    {
    	// Now that we know we have minor A, we can afford to orient the
    	// bicomp because we will either find the desired K4 or we will
    	// reduce the bicomp to an edge. The tests for A1 and A2 are easier
    	// to implement on an oriented bicomp.
        _OrientVerticesInBicomp(theGraph, R, 1);

    	// Case A1: Test whether there is an active vertex Z other than W
    	// along the external face path [X, ..., W, ..., Y]
    	if (_K4_FindSecondActiveVertexOnLowExtFacePath(theGraph) == TRUE)
    	{
        	// Restore the orientations of the vertices in the bicomp, then orient
    		// the whole embedding, so we can restore and orient the reduced paths
            _OrientVerticesInBicomp(theGraph, R, 1);
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

            // TO DO: there needs to be some kind of "FinishIsolatorContextInitialization"
            // logic that does more work on the visited flags.  Maybe because we're catching
            // it before any markings at all, we could just fill all flags with 0?
            // What about the "find" calls in that function?

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA1(theGraph) != OK  ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
    	}

    	// Case A2: Test whether the bicomp has an XY path
    	_SetVertexTypesForMarkingXYPath(graphP theGraph);
        if (_MarkHighestXYPath(theGraph) == TRUE)
        {
        	// Restore the orientations of the vertices in the bicomp, then orient
    		// the whole embedding, so we can restore and orient the reduced paths
            _OrientVerticesInBicomp(theGraph, R, 1);
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

            // TO DO: there needs to be some kind of "FinishIsolatorContextInitialization"
            // logic that does more work on the visited flags.  Maybe because we're catching
            // it before any markings at all, we could just fill all flags with 0?
            // What about the "find" calls in that function?

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA2(theGraph) != OK ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
        }

        // Since neither A1 nor A2 is found, then we reduce the bicomp to the
        // tree edge (R, W).
    	if (_K4_ReduceBicompToEdge(theGraph, R, IC->w) != OK)
    		return NOTOK;

        // Return OK so that the WalkDown can continue resolving the pertinence of I.
    	return OK;
    }

    // Minor B also indicates the existence of K_{2,3} homeomorphs, but
    // we run additional tests to see whether we can either find an
    // entwined K4 homeomorph or reduce a portion of the bicomp so that
    // the WalkDown can be reinvoked on the bicomp
    else if (theGraph->IC.minorType & MINORTYPE_B)
    {
    	int a_x, a_y;

    	// Find the vertices a_x and a_y that are active (pertinent or future pertinent)
    	// and also first along the external face paths emanating from the bicomp root
    	if (_K4_FindPlanarityActiveVertex(theGraph, I, R, 1, &a_x) != OK ||
    		_K4_FindPlanarityActiveVertex(theGraph, I, R, 0, &a_y) != OK)
    		return NOTOK;

    	// Case B1: If both a_x and a_y are future pertinent, then we can stop and
    	// isolate a subgraph homeomorphic to K4.
    	if (FUTUREPERTINENT(theGraph, a_x, I) && FUTUREPERTINENT(theGraph, a_y, I))
    	{
    		// TO DO: finish init then isolate K4 from case B1, then return NONEMBEDDABLE
    		return NOTOK;
    	}

    	// Case B2: Determine whether there is an internal separating X-Y path for a_x or for a_y
    	// The method makes appropriate isolator context settings if the separator edge is found
    	if (_K4_FindSeparatingInternalEdge(theGraph, R, a_x, 1, &IC->w, &IC->px, &IC->py) == TRUE ||
    		_K4_FindSeparatingInternalEdge(theGraph, R, a_y, 0, &IC->w, &IC->py, &IC->px) == TRUE)
    	{
    		// TO DO: finish init then isolate K4 from case B2, then return NONEMBEDDABLE
    		return NOTOK;
    	}

    	// TO DO: If pattern not found, make reductions along a_x and a_y paths, then return OK

    	// TO DO: Returning OK is the way to get the embedder to proceed to the next
    	// iteration, but we actually need to continue the *WalkDown* on the bicomp
    	return NOTOK;
    }

	// Minor E indicates the desired K4 homeomorph, so we isolate it and return NONEMBEDDABLE
    else if (theGraph->IC.minorType & MINORTYPE_E)
    {
        // Impose consistent orientation on the embedding so we can then
        // restore the reduced paths.
        _OrientVerticesInEmbedding(theGraph);
        if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
            return NOTOK;

        // TO DO: there needs to be some kind of "FinishIsolatorContextInitialization"
        // logic that does more work on the visited flags.  Maybe because we're catching
        // it before any markings below, we could just fill all flags with 0?

        // Set up to isolate minor E
        if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
            return NOTOK;

        if (_MarkHighestXYPath(theGraph) != TRUE)
             return NOTOK;

        // Isolate the K4 homeomorph
        if (_IsolateOuterplanarityObstructionE(theGraph) != OK ||
        	_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
            return NOTOK;

        // Return indication that K4 homeomorph has been found
        return NONEMBEDDABLE;
    }

    // You never get here in an error-free implementation like this one
    return NOTOK;
}

/****************************************************************************
 _K4_ChooseTypeOfNonOuterplanarityMinor()
 This is an overload of the function _ChooseTypeOfNonOuterplanarityMinor()
 that avoids processing the whole bicomp rooted by R, e.g. to orient its
 vertices or label the vertices of its external face.
 This is necessary in particular because of the reduction processing on
 MINORTYPE_B.  When a K2,3 is found by minor B, we may not be able to find
 an entangled K4, so a reduction is performed, but it only eliminates
 part of the bicomp and the operations here need to avoid touching parts
 of the bicomp that won't be reduced, except by a constant amount of course.
 ****************************************************************************/
int  _K4_ChooseTypeOfNonOuterplanarityMinor(graphP theGraph, int I, int R)
{
    int  XPrevLink=1, YPrevLink=0;
    int  Wx, WxPrevLink, Wy, WyPrevLink;

    _ClearIsolatorContext(theGraph);

    theGraph->IC.v = I;
    theGraph->IC.r = R;

    // We are essentially doing a _FindActiveVertices() here, except two things:
    // 1) for outerplanarity we know the first vertices along the paths from R
    //    are the desired vertices because all vertices are "externally active"
    // 2) We have purposely not oriented the bicomp, so the XPrevLink result is
    //    needed to help find the pertinent vertex W
    theGraph->IC.x = _GetNextVertexOnExternalFace(theGraph, R, &XPrevLink);
    theGraph->IC.y = _GetNextVertexOnExternalFace(theGraph, R, &YPrevLink);

    // We are essentially doing a _FindPertinentVertex() here, except two things:
    // 1) It is not known whether the reduction of the path through X or the path
    //    through Y will enable the pertinence of W to be resolved, so it is
    //    necessary to perform parallel face traversal to find W with a cost no
    //    more than twice what it will take to resolve the W's pertinence
    //    (assuming we have to do a reduction rather than finding an entangled K4)
    // 2) In the normal _FindPertinentVertex(), the bicomp is already oriented, so
    //    the "prev link" is hard coded to traverse down the X side.  In this
    //    implementation, the  bicomp is purposely not oriented, so we need to know
    //    XPrevLink and YPrevLink in order to set off in the correct directions.
    Wx = theGraph->IC.x;
    WxPrevLink = XPrevLink;
    Wy = theGraph->IC.y;
    WyPrevLink = YPrevLink;
    theGraph->IC.w = NIL;

    while (Wx != theGraph->IC.y)
    {
        Wx = _GetNextVertexOnExternalFace(theGraph, Wx, &WxPrevLink);
        if (PERTINENT(theGraph, Wx))
        {
        	theGraph->IC.w = Wx;
        	break;
        }
        Wy = _GetNextVertexOnExternalFace(theGraph, Wy, &WyPrevLink);
        if (PERTINENT(theGraph, Wy))
        {
        	theGraph->IC.w = Wy;
        	break;
        }
    }

    if (theGraph->IC.w == NIL)
    	return NOTOK;

    // If the root copy is not a root copy of the current vertex I,
    // then the Walkdown terminated on a descendant bicomp, which is Minor A.
	if (theGraph->V[R - theGraph->N].DFSParent != I)
		theGraph->IC.minorType |= MINORTYPE_A;

    // If W has a pertinent child bicomp, then we've found Minor B.
    // Notice this is different from planarity, in which minor B is indicated
    // only if the pertinent child bicomp is also externally active under the
    // planarity processing model (i.e. future pertinent).
	else if (theGraph->V[theGraph->IC.w].pertinentBicompList != NIL)
		theGraph->IC.minorType |= MINORTYPE_B;

    // The only other result is minor E (we will search for the X-Y path later)
	else
		theGraph->IC.minorType |= MINORTYPE_E;

	return OK;
}

/****************************************************************************
 _K4_FindSecondActiveVertexOnLowExtFacePath()

 This method is used in the processing of obstruction A, so it can take
 advantage of the bicomp being oriented beforehand.

 This method determines whether there is an active vertex Z other than W on
 the path [X, ..., W, ..., Y].  By active, we mean a vertex that connects
 by an unembedded edge to either I or an ancestor of I.  That is, a vertext
 that is pertinent or future pertinent (would be pertinent in a future step
 of the embedder).

 Unlike the core planarity embedder, in outerplanarity-related algorithms,
 future pertinence is different from external activity, and we need to know
 about *actual connections* from each vertex to ancestors of IC.v, so we
 use PERTINENT() and FUTUREPERTINENT() rather than _VertexActiveStatus().

 TO DO: Double-check the isolator context input requirements and output expectations
 ****************************************************************************/
int _K4_FindSecondActiveVertexOnLowExtFacePath(graphP theGraph)
{
    int Z, ZPrevLink;

	// First we test X for future pertinence only (if it were pertinent, then
	// we wouldn't have been blocked up on this bicomp)
    if (FUTUREPERTINENT(theGraph, theGraph->IC.x, theGraph->IC.v))
	{
		theGraph->IC.z = theGraph->IC.x;
		theGraph->IC.uz = _GetLeastAncestorConnection(theGraph, theGraph->IC.x);
		return TRUE;
	}

	// Now we move on to test all the vertices strictly between X and Y on
	// the lower external face path, except W, for either pertinence or
	// future pertinence.
    ZPrevLink = 1;
	Z = _GetNextVertexOnExternalFace(theGraph, theGraph->IC.x, &ZPrevLink);

	while (Z != theGraph->IC.y)
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
    if (FUTUREPERTINENT(theGraph, theGraph->IC.y, theGraph->IC.v))
	{
		theGraph->IC.z = theGraph->IC.y;
		theGraph->IC.uz = _GetLeastAncestorConnection(theGraph, theGraph->IC.y);
		return TRUE;
	}

	// We didn't find the desired second vertex, so report FALSE
	return FALSE;
}

/****************************************************************************
 _K4_ReduceBicompToEdge()

 This method is used when reducing the main bicomp of obstruction A to a
 single edge (R, W).  We first delete all edges from the bicomp except
 those on the DFS tree path W to R, then we reduce that DFS tree path to
 a DFS tree edge.

 After the reduction, the outerplanarity Walkdown traversal can continue
 R to W without being blocked as was the case when R was adjacent to X and Y.

 Returns OK for success, NOTOK for internal (implementation) error.
 ****************************************************************************/

int  _K4_ReduceBicompToEdge(graphP theGraph, int R, int W)
{
	// TO DO: finish this
	return NOTOK;
}

/****************************************************************************
 _K4_FindPlanarityActiveVertex()
 This service routine starts out at R and heads off in the direction opposite
 the prevLink to find the first "planarity active" vertex, i.e. the first one
 that is pertinent or future pertinent.
 ****************************************************************************/
int  _K4_FindPlanarityActiveVertex(graphP theGraph, int I, int R, int prevLink, int *pW)
{
	int W = R, WPrevLink = prevLink;

	W = _GetNextVertexOnExternalFace(theGraph, R, &WPrevLink);

	while (W != R)
	{
	    if (PERTINENT(theGraph, W) || FUTUREPERTINENT(theGraph, W, I))
		{
	    	*pW = W;
	    	return OK;
		}

		W = _GetNextVertexOnExternalFace(theGraph, W, &WPrevLink);
	}

	return NOTOK;
}

/****************************************************************************
 _K4_FindSeparatingInternalEdge()

 Logically, this method is similar to calling MarkHighestXYPath() to
 see if there is an internal separator between R and W.
 However, that method cannot be called because the bicomp is not oriented.

 Because this is an outerplanarity related algorithm, there are no internal
 vertices to contend with, so it is easier to inspect the internal edges
 incident to each vertex internal to the path (R ... W), i.e. excluding endpoints,
 to see whether any of the edges connects outside of the path [R ... W],
 including endpoints.

 We will count on the pre-initialization of the vertex types to TYPE_UNKNOWN
 so that we don't have to initialize the whole bicomp. Each vertex along
 the path [R ... W] is marked TYPE_VERTEX_VISITED.  Then, for each vertex in the
 range (R ... W), if there is any edge that is also not incident to a vertex
 with TYPE_UNKNOWN, then that edge is the desired separator edge between
 R and W.  We mark that edge and save information about it.

 Finally, we put the vertex types along [R ... W] back to TYPE_UNKNOWN.

 This method sets the * pW, *pX and *pY values with the endpoints of
 the separator edge, if one is found.  It does not set the visited flags in
 the edge records because it is easier to set it later.

 Returns TRUE if separator edge found or FALSE otherwise
 ****************************************************************************/

int _K4_FindSeparatingInternalEdge(theGraph, int R, int W, int prevLink, int *pW, int *pX, int *pY)
{
	int Z, ZPrevLink, J, neighbor;

	// Mark the vertex types along the path [R ... W] as visited
	theGraph->G[R].type = TYPE_VERTEX_VISITED;
	ZPrevLink = prevLink;
	Z = R;
	while (Z != W)
	{
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
		theGraph->G[Z].type = TYPE_VERTEX_VISITED;
	}

	// Search each of the vertices in the range (R ... W)
	*pX = *pY = NIL;
	ZPrevLink = prevLink;
	Z = _GetNextVertexOnExternalFace(theGraph, R, &ZPrevLink);;
	while (Z != W)
	{
		// Search for a separator among the edges of Z
		// It is OK to not bother skipping the external face edges, since we
		// know they are marked with TYPE_VERTEX_VISITED
	    J = gp_GetFirstArc(theGraph, Z);
	    while (gp_IsArc(theGraph, J))
	    {
	        neighbor = theGraph->G[J].v;
	        if (theGraph->G[neighbor].type == TYPE_UNKNOWN)
	        {
	        	*pW = W;
	        	*pX = Z;
	        	*pY = neighbor;
	        	break;
	        }
	        J = gp_GetNextArc(theGraph, J);
	    }

	    // If we found the separator edge, then we don't need to go on
	    if (*pX != NIL)
	    	break;

		// Go to the next vertex
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
	}

	// Restore the vertex types along the path [R ... W] to the unknown state
	theGraph->G[R].type = TYPE_UNKNOWN;
	ZPrevLink = prevLink;
	Z = R;
	while (Z != W)
	{
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
		theGraph->G[Z].type = TYPE_UNKNOWN;
	}

	return *pX == NIL ? FALSE : TRUE;
}

/****************************************************************************
 ****************************************************************************/
int  _K4_IsolateMinorA1(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

/****************************************************************************
 ****************************************************************************/
int  _K4_IsolateMinorA2(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

/****************************************************************************
 ****************************************************************************/
int  _K4_IsolateMinorB1(graphP theGraph)
{
	// TO DO
	return NOTOK;
}

/****************************************************************************
 ****************************************************************************/
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
