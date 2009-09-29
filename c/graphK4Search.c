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

extern void _ClearIsolatorContext(graphP theGraph);
extern void _FillVisitedFlags(graphP, int);
extern void _FillVisitedFlagsInBicomp(graphP theGraph, int BicompRoot, int FillValue);
extern void _FillVisitedFlagsInOtherBicomps(graphP theGraph, int BicompRoot, int FillValue);
extern void _FillVisitedFlagsInUnembeddedEdges(graphP theGraph, int FillValue);
extern void _DeleteUnmarkedEdgesInBicomp(graphP theGraph, int BicompRoot);
extern void _ClearInvertedFlagsInBicomp(graphP theGraph, int BicompRoot);
extern int  _ComputeArcType(graphP theGraph, int a, int b, int edgeType);
extern int  _SetEdgeType(graphP theGraph, int u, int v);

extern int  _GetNextVertexOnExternalFace(graphP theGraph, int curVertex, int *pPrevLink);
extern void _FindActiveVertices(graphP theGraph, int R, int *pX, int *pY);
extern void _OrientVerticesInBicomp(graphP theGraph, int BicompRoot, int PreserveSigns);
extern void _OrientVerticesInEmbedding(graphP theGraph);
extern void _InvertVertex(graphP theGraph, int V);

extern int  _FindUnembeddedEdgeToAncestor(graphP theGraph, int cutVertex, int *pAncestor, int *pDescendant);
extern int  _FindUnembeddedEdgeToCurVertex(graphP theGraph, int cutVertex, int *pDescendant);
extern int  _GetLeastAncestorConnection(graphP theGraph, int cutVertex);

extern void _SetVertexTypesForMarkingXYPath(graphP theGraph);
extern int  _MarkHighestXYPath(graphP theGraph);
extern int  _MarkPathAlongBicompExtFace(graphP theGraph, int startVert, int endVert);
extern int  _AddAndMarkEdge(graphP theGraph, int ancestor, int descendant);
extern int  _DeleteUnmarkedVerticesAndEdges(graphP theGraph);

extern int  _IsolateOuterplanarityObstructionA(graphP theGraph);
extern int  _IsolateOuterplanarityObstructionB(graphP theGraph);
extern int  _IsolateOuterplanarityObstructionE(graphP theGraph);

/* Private functions for K4 searching (exposed to the extension). */

int  _SearchForK4(graphP theGraph, int I);
int  _SearchForK4InBicomp(graphP theGraph, K4SearchContext *context, int I, int R);

/* Private functions for K4 searching. */

int  _K4_ChooseTypeOfNonOuterplanarityMinor(graphP theGraph, int I, int R);

int  _K4_FindSecondActiveVertexOnLowExtFacePath(graphP theGraph);
int  _K4_FindPlanarityActiveVertex(graphP theGraph, int I, int R, int prevLink, int *pW);
int  _K4_FindSeparatingInternalEdge(graphP theGraph, int R, int prevLink, int A, int *pW, int *pX, int *pY);

int  _K4_IsolateMinorA1(graphP theGraph);
int  _K4_IsolateMinorA2(graphP theGraph);
int  _K4_IsolateMinorB1(graphP theGraph);
int  _K4_IsolateMinorB2(graphP theGraph);

int  _K4_ReduceBicompToEdge(graphP theGraph, K4SearchContext *context, int R, int W);
int  _K4_ReducePathComponent(graphP theGraph, K4SearchContext *context, int R, int prevLink, int A);
int  _K4_ReducePathToEdge(graphP theGraph, K4SearchContext *context, int edgeType, int R, int e_R, int A, int e_A);

int  _K4_TestPathComponentForAncestor(graphP theGraph, int R, int prevLink, int A);
void _K4_SetVisitedInPathComponent(graphP theGraph, int R, int prevLink, int A, int fill);
void _K4_DeleteUnmarkedEdgesInPathComponent(graphP theGraph, int R, int prevLink, int A);

int  _K4_RestoreReducedPath(graphP theGraph, K4SearchContext *context, int J);
int  _K4_RestoreAndOrientReducedPaths(graphP theGraph, K4SearchContext *context);
int  _K4_OrientPath(graphP theGraph, int u, int v, int w, int x);
void _K4_SetVisitedOnPath(graphP theGraph, int u, int v, int w, int x, int visited);

int  _MarkEdge(graphP theGraph, int u, int v);


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

            // Set up to isolate K4 homeomorph
            _FillVisitedFlags(theGraph, 0);

            if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
                return NOTOK;

            if (IC->uz < IC->v)
            {
            	if (_FindUnembeddedEdgeToAncestor(theGraph, IC->z, &IC->uz, &IC->dz) != TRUE)
            		return NOTOK;
            }
            else
            {
                if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->z, &IC->dz) != TRUE)
                    return NOTOK;
            }

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA1(theGraph) != OK  ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
    	}

    	// Case A2: Test whether the bicomp has an XY path
    	_SetVertexTypesForMarkingXYPath(theGraph);
        if (_MarkHighestXYPath(theGraph) == TRUE)
        {
        	// Restore the orientations of the vertices in the bicomp, then orient
    		// the whole embedding, so we can restore and orient the reduced paths
            _OrientVerticesInBicomp(theGraph, R, 1);
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

            // Set up to isolate K4 homeomorph
            _FillVisitedFlags(theGraph, 0);

            if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
                return NOTOK;

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorA2(theGraph) != OK ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
        }

        // Since neither A1 nor A2 is found, then we reduce the bicomp to the
        // tree edge (R, W).
    	if (_K4_ReduceBicompToEdge(theGraph, context, R, IC->w) != OK)
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
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

            // Set up to isolate K4 homeomorph
            _FillVisitedFlags(theGraph, 0);

            if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
                return NOTOK;

            IC->x = a_x;
            IC->y = a_y;

           	if (_FindUnembeddedEdgeToAncestor(theGraph, IC->x, &IC->ux, &IC->dx) != TRUE ||
           		_FindUnembeddedEdgeToAncestor(theGraph, IC->y, &IC->uy, &IC->dy) != TRUE)
           		return NOTOK;

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorB1(theGraph) != OK  ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
    	}

    	// Case B2: Determine whether there is an internal separating X-Y path for a_x or for a_y
    	// The method makes appropriate isolator context settings if the separator edge is found
    	if (_K4_FindSeparatingInternalEdge(theGraph, R, 1, a_x, &IC->w, &IC->px, &IC->py) == TRUE ||
    		_K4_FindSeparatingInternalEdge(theGraph, R, 0, a_y, &IC->w, &IC->py, &IC->px) == TRUE)
    	{
            _OrientVerticesInEmbedding(theGraph);
            if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
                return NOTOK;

            // Set up to isolate K4 homeomorph
            _FillVisitedFlags(theGraph, 0);

            if (PERTINENT(theGraph, IC->w))
            {
                if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != TRUE)
                    return NOTOK;
            }
            else
            {
            	if (_FindUnembeddedEdgeToAncestor(theGraph, IC->z, &IC->uz, &IC->dz) != TRUE)
            		return NOTOK;
            }

    		// Isolate the K4 homeomorph
    		if (_K4_IsolateMinorB2(theGraph) != OK  ||
    			_DeleteUnmarkedVerticesAndEdges(theGraph) != OK)
    			return NOTOK;

            // Indicate success by returning NONEMBEDDABLE
    		return NONEMBEDDABLE;
    	}

    	// If K_4 homeomorph not found, make reductions along a_x and a_y paths.
    	if (_K4_ReducePathComponent(theGraph, context, R, 1, a_x) != OK ||
    		_K4_ReducePathComponent(theGraph, context, R, 0, a_y) != OK)
    		return NOTOK;

    	// Return OK to indicate that WalkDown processing may proceed to resolve
    	// more of the pertinence of this bicomp.
		return OK;
    }

	// Minor E indicates the desired K4 homeomorph, so we isolate it and return NONEMBEDDABLE
    else if (theGraph->IC.minorType & MINORTYPE_E)
    {
        // Impose consistent orientation on the embedding so we can then
        // restore the reduced paths.
        _OrientVerticesInEmbedding(theGraph);
        if (_K4_RestoreAndOrientReducedPaths(theGraph, context) != OK)
            return NOTOK;

        // Set up to isolate minor E
        _FillVisitedFlags(theGraph, 0);

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
 see if there is an internal separator between R and A.
 However, that method cannot be called because the bicomp is not oriented.

 Because this is an outerplanarity related algorithm, there are no internal
 vertices to contend with, so it is easier to inspect the internal edges
 incident to each vertex internal to the path (R ... A), i.e. excluding endpoints,
 to see whether any of the edges connects outside of the path [R ... A],
 including endpoints.

 We will count on the pre-initialization of the vertex types to TYPE_UNKNOWN
 so that we don't have to initialize the whole bicomp. Each vertex along
 the path [R ... A] is marked TYPE_VERTEX_VISITED.  Then, for each vertex in the
 range (R ... A), if there is any edge that is also not incident to a vertex
 with TYPE_UNKNOWN, then that edge is the desired separator edge between
 R and W.  We mark that edge and save information about it.

 If the separator edge is found, then this method sets the *pW to A, and it
 sets *pX and *pY values with the endpoints of the separator edge.
 No visited flags are set at this time because it is easier to set them later.

 Lastly, we put the vertex types along [R ... A] back to TYPE_UNKNOWN.

 Returns TRUE if separator edge found or FALSE otherwise
 ****************************************************************************/

int _K4_FindSeparatingInternalEdge(graphP theGraph, int R, int prevLink, int A, int *pW, int *pX, int *pY)
{
	int Z, ZPrevLink, J, neighbor;

	// Mark the vertex types along the path [R ... A] as visited
	theGraph->G[R].type = TYPE_VERTEX_VISITED;
	ZPrevLink = prevLink;
	Z = R;
	while (Z != A)
	{
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
		theGraph->G[Z].type = TYPE_VERTEX_VISITED;
	}

	// Search each of the vertices in the range (R ... A)
	*pX = *pY = NIL;
	ZPrevLink = prevLink;
	Z = _GetNextVertexOnExternalFace(theGraph, R, &ZPrevLink);
	while (Z != A)
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
	        	*pW = A;
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

	// Restore the vertex types along the path [R ... A] to the unknown state
	theGraph->G[R].type = TYPE_UNKNOWN;
	ZPrevLink = prevLink;
	Z = R;
	while (Z != A)
	{
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
		theGraph->G[Z].type = TYPE_UNKNOWN;
	}

	return *pX == NIL ? FALSE : TRUE;
}

/****************************************************************************
 _K4_IsolateMinorA1()

 This pattern is essentially outerplanarity minor A, a K_{2,3}, except we get
 a K_4 via the additional path from some vertex Z to the current vertex.
 This path may be via some descendant of Z, and it may be a future pertinent
 connection to an ancestor of the current vertex.
 ****************************************************************************/

int  _K4_IsolateMinorA1(graphP theGraph)
{
	isolatorContextP IC = &theGraph->IC;

	if (IC->uz < IC->v)
	{
		if (theGraph->functions.fpMarkDFSPath(theGraph, IC->uz, IC->v) != OK)
			return OK;
	}

	if (theGraph->functions.fpMarkDFSPath(theGraph, IC->z, IC->dz) != OK)
    	return NOTOK;

	if (_IsolateOuterplanarityObstructionA(theGraph) != OK)
		return NOTOK;

    if (_AddAndMarkEdge(theGraph, IC->uz, IC->dz) != OK)
        return NOTOK;

	return OK;
}

/****************************************************************************
 _K4_IsolateMinorA2()

 This pattern is essentially outerplanarity minor A, a K_{2,3}, except we get
 a K_4 via an additional X-Y path within the main bicomp, which is guaranteed
 to exist by the time this method is invoked.
 One might think to simply invoke _MarkHighestXYPath() to obtain the path,
 but the IC->px and IC->py values are already set before invoking this method,
 and the bicomp is outerplanar, so the XY path is just an edge. Also, one
 subcase of pattern B2 reduces to this pattern, except that the XY path is
 determined by the B2 isolator.
 ****************************************************************************/

int  _K4_IsolateMinorA2(graphP theGraph)
{
	isolatorContextP IC = &theGraph->IC;

    if (_MarkEdge(theGraph, IC->px, IC->py) != TRUE)
    	return NOTOK;

	return _IsolateOuterplanarityObstructionA(theGraph);
}

/****************************************************************************
 _K4_IsolateMinorB1()

 This is essentially outerplanarity minor B, a K_{2,3}, except we geta  K_4
 via an additional path from a_x through ancestors of the current vertex to a_y.
 ****************************************************************************/

int  _K4_IsolateMinorB1(graphP theGraph)
{
	isolatorContextP IC = &theGraph->IC;

	if (theGraph->functions.fpMarkDFSPath(theGraph, IC->x, IC->dx) != OK)
    	return NOTOK;

	if (theGraph->functions.fpMarkDFSPath(theGraph, IC->y, IC->dy) != OK)
    	return NOTOK;

	if (theGraph->functions.fpMarkDFSPath(theGraph, MIN(IC->ux, IC->uy), MAX(IC->ux, IC->uy)) != OK)
    	return NOTOK;

	if (_IsolateOuterplanarityObstructionB(theGraph) != OK)
		return NOTOK;

    if (_AddAndMarkEdge(theGraph, IC->ux, IC->dx) != OK)
        return NOTOK;

    if (_AddAndMarkEdge(theGraph, IC->uy, IC->dy) != OK)
        return NOTOK;

	return OK;
}

/****************************************************************************
 _K4_IsolateMinorB2()

 The first subcase of B2 can be reduced to outerplanarity obstruction E
 The second subcase of B2 can be reduced to A2 by changing v to u
 ****************************************************************************/

int  _K4_IsolateMinorB2(graphP theGraph)
{
	isolatorContextP IC = &theGraph->IC;

	// First subcase, the active vertex is pertinent
    if (PERTINENT(theGraph, IC->w))
    {
        if (_MarkEdge(theGraph, IC->px, IC->py) != TRUE)
        	return NOTOK;

    	return _IsolateOuterplanarityObstructionE(theGraph);
    }

    // Second subcase, the active vertex is future pertinent
    else if (FUTUREPERTINENT(theGraph, IC->w, IC->v))
    {
    	IC->r = IC->v;
    	IC->v = IC->uz;
    	IC->dw = IC->dz;

    	return _K4_IsolateMinorA2(theGraph);
    }

	return OK;
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

int  _K4_ReduceBicompToEdge(graphP theGraph, K4SearchContext *context, int R, int W)
{
	int Rvisited = theGraph->G[R].visited, Wvisited = theGraph->G[W].visited;

	_OrientVerticesInBicomp(theGraph, R, 0);
    _FillVisitedFlagsInBicomp(theGraph, R, 0);
    if (theGraph->functions.fpMarkDFSPath(theGraph, R, W) != OK)
        return NOTOK;
    _DeleteUnmarkedEdgesInBicomp(theGraph, R);

    // Now we have to reduce the path W -> R to the DFS tree edge (R, W)
    if (_K4_ReducePathToEdge(theGraph, context, EDGE_DFSPARENT,
    		R, gp_GetFirstArc(theGraph, R), W, gp_GetFirstArc(theGraph, W)) != OK)
    	return NOTOK;

    // Finally, restore the visited flag settings of R and W, so that
    // the core embedder (esp. Walkup) will not have any problems.
	theGraph->G[R].visited = Rvisited;
	theGraph->G[W].visited = Wvisited;

	return OK;
}

/****************************************************************************
 _K4_ReducePathComponent()

 This method is invoked when the bicomp rooted by R contains a component
 subgraph that is separable from the bicomp by the 2-cut (R, A). The K_4
 homeomorph isolator will have processed a significant fraction of the
 component, and so it must be reduced to an edge to ensure that said
 processing happens at most once on the component (except for future
 operations that are bound to linear time in total by other arguments).

 Because the bicomp is an outerplanar embedding, the component is known to
 consists of an external face path plus some internal edges that are parallel
 to that path. Otherwise, it wouldn't be separable by the 2-cut (R, A).

 The goal of this method is to reduce the component to the edge (R, A). This
 is done in such a way that, if the reduction must be restored, the DFS tree
 structure connecting the restored vertices is retained.

 The first step is to ensure that (R, A) is not already just an edge, in which
 case no reduction is needed. This can occur if A is future pertinent.

 Assuming a non-trivial reduction component, the next step is to determine
 the DFS tree structure within the component. Because it is separable by the
 2-cut (R, A), there are only two cases:

 Case 1: The DFS tree path from A to R is within the reduction component.

 In this case, the DFS tree path is marked, the remaining edges of the
 reduction component are eliminated, and then the DFS tree path is reduced to
 the the tree edge (R, A).

 Note that the reduction component may also contain descendants of A as well
 as vertices that are descendant to R but are neither ancestors nor
 descendants of A. This depends on where the tree edge from R meets the
 external face path (R ... A). However, the reduction component can only
 contribute one path to any future K_4, so it suffices to preserve only the
 DFS tree path (A --> R).

 Case 2: The DFS tree path from A to R is not within the reduction component.

 In this case, the external face edge from R leads to a descendant D of A.
 We mark that back edge (R, D) plus the DFS tree path (D --> A). The
 remaining edges of the reduction component can be removed, and then the
 path (R, D, ..., A) is reduced to the edge (R, A).

 For the sake of contradiction, suppose that only part of the DFS tree path
 from A to R were contained by the reduction component. Then, a DFS tree edge
 would have to exit the reduction component and connect to some vertex not
 on the external face path (R, ..., A). This contradicts the assumption that
 the reduction subgraph is separable from the bicomp by the 2-cut (R, A).

 Returns OK for success, NOTOK for internal (implementation) error.
 ****************************************************************************/

int  _K4_ReducePathComponent(graphP theGraph, K4SearchContext *context, int R, int prevLink, int A)
{
	int  e_R, e_A, Z, ZPrevLink, edgeType;

	// Check whether the external face path (R, ..., A) is just an edge
	e_R = gp_GetArc(theGraph, R, 1^prevLink);
	if (theGraph->G[e_R].v == A)
	    return OK;

	// Check for Case 1: The DFS tree path from A to R is within the reduction component
	if (_K4_TestPathComponentForAncestor(theGraph, R, prevLink, A))
	{
		_K4_SetVisitedInPathComponent(theGraph, R, prevLink, A, 0);
	    if (theGraph->functions.fpMarkDFSPath(theGraph, R, A) != OK)
	        return NOTOK;
	    edgeType = EDGE_DFSPARENT;
	}

	// Otherwise Case 2: The DFS tree path from A to R is not within the reduction component
	else
	{
		_K4_SetVisitedInPathComponent(theGraph, R, prevLink, A, 0);
		Z = theGraph->G[e_R].v;
		theGraph->G[e_R].visited = 1;
		theGraph->G[gp_GetTwinArc(theGraph, e_R)].visited = 1;
	    if (theGraph->functions.fpMarkDFSPath(theGraph, A, Z) != OK)
	        return NOTOK;
		edgeType = EDGE_BACK;
	}

	// The path to be kept/reduced is marked, so the other edges can go
	_K4_DeleteUnmarkedEdgesInPathComponent(theGraph, R, prevLink, A);
	_K4_SetVisitedInPathComponent(theGraph, R, prevLink, A, theGraph->N);

	// Find the proper edges incident to A and R along the external face
	ZPrevLink = prevLink;
	Z = R;
	while (Z != A)
	{
		Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);
	}
	e_A = gp_GetArc(theGraph, A, ZPrevLink);
	e_R = gp_GetArc(theGraph, R, 1^prevLink);

	// Reduce the path (R ... A) to an edge
	if (_K4_ReducePathToEdge(theGraph, context, edgeType, R, e_R, A, e_A) != OK)
		return NOTOK;

	return OK;
}

/****************************************************************************
 _K4_TestPathComponentForAncestor()
 Tests the external face path between R and A for a DFS ancestor of A.
 Returns TRUE if found, FALSE otherwise.
 ****************************************************************************/

int _K4_TestPathComponentForAncestor(graphP theGraph, int R, int prevLink, int A)
{
	return FALSE;
}

/****************************************************************************
 _K4_SetVisitedInPathComponent()

 There is a subcomponent of the bicomp rooted by R that is separable by the
 2-cut (R, A) and contains the edge e_R = theGraph->G[R].link[1^prevLink].

 All vertices in this component are along the external face, so we first
 mark them specially. Then, for each edge incident to R and one of the
 marked vertices, and for each edge incident to A and one of the vertices,
 and for all edges incident to the internal vertices on the path, we set
 their visited members to the 'fill' value. Finally, we clear the special
 markings on the vertices to avoid side effects.
 ****************************************************************************/

void _K4_SetVisitedInPathComponent(graphP theGraph, int R, int prevLink, int A, int fill)
{
}

/****************************************************************************
 _K4_DeleteUnmarkedEdgesInPathComponent()

 There is a subcomponent of the bicomp rooted by R that is separable by the
 2-cut (R, A) and contains the edge e_R = theGraph->G[R].link[1^prevLink].
 The edges in the component have been marked unvisited except for a path we
 intend to preserve. This routine deletes the unvisited edges.
 ****************************************************************************/

void _K4_DeleteUnmarkedEdgesInPathComponent(graphP theGraph, int R, int prevLink, int A)
{
}

/****************************************************************************
 _K4_ReducePathToEdge()
 ****************************************************************************/

int  _K4_ReducePathToEdge(graphP theGraph, K4SearchContext *context, int edgeType, int R, int e_R, int A, int e_A)
{
	 int Rlink, Alink, v_R, v_A;

	 // If the path is a single edge, then no reduction is needed
	 if (theGraph->G[e_R].v == A)
		 return OK;

	 // Find out the links used in vertex R for edge e_R and in vertex A for edge e_A
	 Rlink = theGraph->G[R].link[0] == e_R ? 0 : 1;
	 Alink = theGraph->G[A].link[0] == e_A ? 0 : 1;

	 // Prepare for removing each of the two edges that join the path to the bicomp by
	 // restoring it if it is a reduction edge (a constant time operation)
	 if (context->G[e_R].pathConnector != NIL)
	 {
		 if (_K4_RestoreReducedPath(theGraph, context, e_R) != OK)
			 return NOTOK;

		 e_R = gp_GetArc(theGraph, R, Rlink);
	 }

	 if (context->G[e_A].pathConnector != NIL)
	 {
		 if (_K4_RestoreReducedPath(theGraph, context, e_A) != OK)
			 return NOTOK;
		 e_A = gp_GetArc(theGraph, A, Alink);
	 }

	 // Save the vertex neighbors of R and A indicated by e_R and e_A for
	 // later use in setting up the path connectors.
	 v_R = theGraph->G[e_R].v;
	 v_A = theGraph->G[e_A].v;

	 // Now delete the two edges that join the path to the bicomp.
	 gp_DeleteEdge(theGraph, e_R, 0);
	 gp_DeleteEdge(theGraph, e_A, 0);

	 // Now add a single edge to represent the path
	 gp_InsertEdge(theGraph, R, gp_GetArc(theGraph, R, Rlink), Rlink,
							 A, gp_GetArc(theGraph, A, Alink), Alink);

	 // Now set up the path connectors so the original path can be recovered if needed.
	 e_R = gp_GetArc(theGraph, R, Rlink);
	 context->G[e_R].pathConnector = v_R;

	 e_A = gp_GetArc(theGraph, A, Alink);
	 context->G[e_A].pathConnector = v_A;

	 // Also, set the reduction edge's type to preserve the DFS tree structure
	 theGraph->G[e_R].type = _ComputeArcType(theGraph, R, A, edgeType);
	 theGraph->G[e_A].type = _ComputeArcType(theGraph, A, R, edgeType);

	 // Set the external face data structure
     theGraph->extFace[R].vertex[Rlink] = A;
     theGraph->extFace[A].vertex[Alink] = R;

     // If the edge represents an entire bicomp, then more external face
     // settings are needed.
     if (gp_GetFirstArc(theGraph, R) == gp_GetLastArc(theGraph, R))
     {
         theGraph->extFace[R].vertex[1^Rlink] = A;
         theGraph->extFace[A].vertex[1^Alink] = R;
         theGraph->extFace[A].inversionFlag = 0;
     }

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

     // Get the locations of the graph nodes between which the new graph nodes
     // must be added in order to reconnect the path parallel to the edge.
     J0 = gp_GetNextArc(theGraph, J);
     J1 = gp_GetPrevArc(theGraph, J);
     JTwin0 = gp_GetNextArc(theGraph, JTwin);
     JTwin1 = gp_GetPrevArc(theGraph, JTwin);

     // We first delete the edge represented by J and JTwin. We do so before
     // restoring the path to ensure we do not exceed the maximum arc capacity.
     gp_DeleteEdge(theGraph, J, 0);

     // Now we add the two edges to reconnect the reduced path represented
     // by the edge [J, JTwin].  The edge record in u is added between J0 and J1.
     // Likewise, the new edge record in x is added between JTwin0 and JTwin1.
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

     // Set the types of the newly added edges. In both cases, the first of the two
     // vertex parameters is known to be degree 2 because they are internal to the
     // path being restored, so this operation is constant time.
     if (_SetEdgeType(theGraph, v, u) != OK || _SetEdgeType(theGraph, w, x) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _K4_RestoreAndOrientReducedPaths()

 This function searches the embedding for any edges that are specially marked
 as being representative of a path that was previously reduced to a single edge
 by _ReducePathToEdge().  This method restores the path by replacing the edge
 with the path.

 Note that the new path may contain more reduction edges, and these will be
 iteratively expanded by the outer for loop.  Equally, a reduced path may
 be restored into a path that itself is a reduction path that will only be
 attached to the embedding by some future step of the outer loop.

 The vertices along the path being restored must be given a consistent
 orientation with the endpoints.  It is expected that the embedding
 will have been oriented prior to this operation.
 ****************************************************************************/

int  _K4_RestoreAndOrientReducedPaths(graphP theGraph, K4SearchContext *context)
{
int  e, J, JTwin, u, v, w, x, visited;

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

    		 if (_K4_RestoreReducedPath(theGraph, context, J) != OK)
    			 return NOTOK;

			 if (_K4_OrientPath(theGraph, u, v, w, x) != OK)
				 return NOTOK;

             _K4_SetVisitedOnPath(theGraph, u, v, w, x, visited);
         }
         else e++;
     }

     return OK;
}

/****************************************************************************
 _K4_OrientPath()
 TO DO: NOT CHECKED FOR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
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
 TO DO: NOT CHECKED FOR SUITABILITY TO THIS ALGORITHM; MAY BE ABLE TO MAKE INTO A COMMON UTIL
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

/****************************************************************************
 _MarkEdge()
 If the edge (u, v) is found, it is marked visited.
 Returns TRUE if the edge is found, FALSE otherwise.
 ****************************************************************************/

int _MarkEdge(graphP theGraph, int u, int v)
{
	int eu = gp_GetNeighborEdgeRecord(theGraph, u, v);
	if (gp_IsArc(theGraph, eu))
	{
		int ev = gp_GetTwinArc(theGraph, eu);
		theGraph->G[u].visited = theGraph->G[v].visited =
		theGraph->G[eu].visited = theGraph->G[ev].visited = 1;
		return TRUE;
	}
	return FALSE;
}
