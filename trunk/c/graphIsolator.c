/* Copyright (c) 1999-2003 by John M. Boyer, All Rights Reserved.
        This code may not be reproduced or disseminated in whole or in part
        without the written permission of the author. */

#define GRAPHISOLATOR_C

#include "graph.h"

/* Imported functions */

extern void _FillVisitedFlags(graphP, int);

extern int  _GetNextVertexOnExternalFace(graphP theGraph, int curVertex, int *pPrevLink);
extern int  _JoinBicomps(graphP theGraph);

extern int _ChooseTypeOfNonplanarityMinor(graphP theGraph, int I, int R);

/* Private function declarations (exported within system) */

int _IsolateKuratowskiSubgraph(graphP theGraph, int I);

int  _FindUnembeddedEdgeToAncestor(graphP theGraph, int cutVertex,
                                   int *pAncestor, int *pDescendant);
int  _FindUnembeddedEdgeToCurVertex(graphP theGraph, int cutVertex,
                                    int *pDescendant);
int  _FindUnembeddedEdgeToSubtree(graphP theGraph, int ancestor,
                                  int SubtreeRoot, int *pDescendant);

int  _MarkPathAlongBicompExtFace(graphP theGraph, int startVert, int endVert);

int  _AddAndMarkEdge(graphP theGraph, int ancestor, int descendant);
void _AddBackEdge(graphP theGraph, int ancestor, int descendant);
int  _DeleteUnmarkedVerticesAndEdges(graphP theGraph);

int  _InitializeIsolatorContext(graphP theGraph);

int  _IsolateMinorA(graphP theGraph);
int  _IsolateMinorB(graphP theGraph);
int  _IsolateMinorC(graphP theGraph);
int  _IsolateMinorD(graphP theGraph);
int  _IsolateMinorE(graphP theGraph);

int  _IsolateMinorE1(graphP theGraph);
int  _IsolateMinorE2(graphP theGraph);
int  _IsolateMinorE3(graphP theGraph);
int  _IsolateMinorE4(graphP theGraph);

int  _GetLeastAncestorConnection(graphP theGraph, int cutVertex);
int  _MarkDFSPathsToDescendants(graphP theGraph);
int  _AddAndMarkUnembeddedEdges(graphP theGraph);

/****************************************************************************
 gp_IsolateKuratowskiSubgraph()
 ****************************************************************************/

int  _IsolateKuratowskiSubgraph(graphP theGraph, int I)
{
int  RetVal = NOTOK;

/* Begin by determining which of the non-planarity Minors was encountered
        and the principal bicomp on which the isolator will focus attention. */

     if (_ChooseTypeOfNonplanarityMinor(theGraph, I, NIL) != OK)
         return NOTOK;

     if (_InitializeIsolatorContext(theGraph) != OK)
         return NOTOK;

/* Call the appropriate isolator */

     if (theGraph->IC.minorType & MINORTYPE_A)
         RetVal = _IsolateMinorA(theGraph);
     else if (theGraph->IC.minorType & MINORTYPE_B)
         RetVal = _IsolateMinorB(theGraph);
     else if (theGraph->IC.minorType & MINORTYPE_C)
         RetVal = _IsolateMinorC(theGraph);
     else if (theGraph->IC.minorType & MINORTYPE_D)
         RetVal = _IsolateMinorD(theGraph);
     else if (theGraph->IC.minorType & MINORTYPE_E)
         RetVal = _IsolateMinorE(theGraph);

/* Delete the unmarked edges and vertices, and return */

     if (RetVal == OK)
         RetVal = _DeleteUnmarkedVerticesAndEdges(theGraph);

     return RetVal;
}

/****************************************************************************
 _InitializeIsolatorContext()
 ****************************************************************************/

int  _InitializeIsolatorContext(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

/* Obtains the edges connecting X and Y to ancestors of the current vertex */

     if (_FindUnembeddedEdgeToAncestor(theGraph, IC->x, &IC->ux, &IC->dx) != OK ||
         _FindUnembeddedEdgeToAncestor(theGraph, IC->y, &IC->uy, &IC->dy) != OK)
         return NOTOK;

/* For Minor B, we seek the last pertinent child biconnected component, which
     is externally active, and obtain the DFS child in its root edge.
     This child is the subtree root containing vertices with connections to
     both the current vertex and an ancestor of the current vertex. */

     if (theGraph->IC.minorType & MINORTYPE_B)
     {
     int SubtreeRoot = LCGetPrev(theGraph->BicompLists,
                                 theGraph->V[IC->w].pertinentBicompList, NIL);

         IC->uz = theGraph->V[SubtreeRoot].Lowpoint;

         if (_FindUnembeddedEdgeToSubtree(theGraph, IC->v, SubtreeRoot, &IC->dw) != OK ||
             _FindUnembeddedEdgeToSubtree(theGraph, IC->uz, SubtreeRoot, &IC->dz) != OK)
             return NOTOK;
     }

/* For all other minors, we obtain  */

     else
     {
         if (_FindUnembeddedEdgeToCurVertex(theGraph, IC->w, &IC->dw) != OK)
             return NOTOK;

         if (theGraph->IC.minorType & MINORTYPE_E)
             if (_FindUnembeddedEdgeToAncestor(theGraph, IC->z, &IC->uz, &IC->dz) != OK)
                 return NOTOK;
     }

     return OK;
}

/****************************************************************************
 _IsolateMinorA(): Isolate a K3,3 homeomorph
 ****************************************************************************/

int  _IsolateMinorA(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->r) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, MIN(IC->ux, IC->uy), IC->r) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _IsolateMinorB(): Isolate a K3,3 homeomorph
 ****************************************************************************/

int  _IsolateMinorB(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->r) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, MIN3(IC->ux,IC->uy,IC->uz),
                                    MAX3(IC->ux,IC->uy,IC->uz)) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _IsolateMinorC(): Isolate a K3,3 homeomorph
 ****************************************************************************/

int  _IsolateMinorC(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (theGraph->G[IC->px].type == VERTEX_HIGH_RXW)
     {
     int highY = theGraph->G[IC->py].type == VERTEX_HIGH_RYW
                 ? IC->py : IC->y;
         if (_MarkPathAlongBicompExtFace(theGraph, IC->r, highY) != OK)
             return NOTOK;
     }
     else
     {
         if (_MarkPathAlongBicompExtFace(theGraph, IC->x, IC->r) != OK)
             return NOTOK;
     }

     if (_MarkDFSPathsToDescendants(theGraph) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, MIN(IC->ux, IC->uy), IC->r) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _IsolateMinorD(): Isolate a K3,3 homeomorph
 ****************************************************************************/

int  _IsolateMinorD(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (_MarkPathAlongBicompExtFace(theGraph, IC->x, IC->y) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, MIN(IC->ux, IC->uy), IC->r) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _IsolateMinorE()
 ****************************************************************************/

int  _IsolateMinorE(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

/* Minor E1: Isolate a K3,3 homeomorph */

     if (IC->z != IC->w)
         return _IsolateMinorE1(theGraph);

/* Minor E2: Isolate a K3,3 homeomorph */

     if (IC->uz > MAX(IC->ux, IC->uy))
         return _IsolateMinorE2(theGraph);

/* Minor E3: Isolate a K3,3 homeomorph */

     if (IC->uz < MAX(IC->ux, IC->uy) && IC->ux != IC->uy)
         return _IsolateMinorE3(theGraph);

/* Minor E4: Isolate a K3,3 homeomorph */

     else if (IC->x != IC->px || IC->y != IC->py)
         return _IsolateMinorE4(theGraph);

/* Minor E: Isolate a K5 homeomorph */

     if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->r) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, MIN3(IC->ux, IC->uy, IC->uz), IC->r) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     return OK;
}

/****************************************************************************
 _IsolateMinorE1()

 Reduce to Minor C if the vertex Z responsible for external activity
 below the X-Y path does not equal the pertinent vertex W.
 ****************************************************************************/

int  _IsolateMinorE1(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (theGraph->G[IC->z].type == VERTEX_LOW_RXW)
     {
         theGraph->G[IC->px].type = VERTEX_HIGH_RXW;
         IC->x=IC->z; IC->ux=IC->uz; IC->dx=IC->dz;
     }
     else if (theGraph->G[IC->z].type == VERTEX_LOW_RYW)
     {
         theGraph->G[IC->py].type = VERTEX_HIGH_RYW;
         IC->y=IC->z; IC->uy=IC->uz; IC->dy=IC->dz;
     }
     else return NOTOK;

     IC->z = IC->uz = IC->dz = NIL;
     theGraph->IC.minorType ^= MINORTYPE_E;
     theGraph->IC.minorType |= (MINORTYPE_C|MINORTYPE_E1);
     return _IsolateMinorC(theGraph);
}

/****************************************************************************
 _IsolateMinorE2()

 If uZ (which is the ancestor of I that is adjacent to Z) is a
 descendant of both uY and uX, then we reduce to Minor A
 ****************************************************************************/

int  _IsolateMinorE2(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     _FillVisitedFlags(theGraph, 0);

     IC->v = IC->uz;
     IC->dw = IC->dz;
     IC->z = IC->uz = IC->dz = NIL;

     theGraph->IC.minorType ^= MINORTYPE_E;
     theGraph->IC.minorType |= (MINORTYPE_A|MINORTYPE_E2);
     return _IsolateMinorA(theGraph);
}

/****************************************************************************
 _IsolateMinorE3()
 ****************************************************************************/

int  _IsolateMinorE3(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (IC->ux < IC->uy)
     {
         if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->px) != OK ||
             _MarkPathAlongBicompExtFace(theGraph, IC->w, IC->y) != OK)
             return NOTOK;
     }
     else
     {
         if (_MarkPathAlongBicompExtFace(theGraph, IC->x, IC->w) != OK ||
             _MarkPathAlongBicompExtFace(theGraph, IC->py, IC->r) != OK)
             return NOTOK;
     }

     if (theGraph->functions.fpMarkDFSPath(theGraph, MIN3(IC->ux, IC->uy, IC->uz), IC->r) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     theGraph->IC.minorType |= MINORTYPE_E3;
     return OK;
}

/****************************************************************************
 _IsolateMinorE4()
 ****************************************************************************/

int  _IsolateMinorE4(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (IC->px != IC->x)
     {
         if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->w) != OK ||
             _MarkPathAlongBicompExtFace(theGraph, IC->py, IC->r) != OK)
             return NOTOK;
     }
     else
     {
         if (_MarkPathAlongBicompExtFace(theGraph, IC->r, IC->px) != OK ||
             _MarkPathAlongBicompExtFace(theGraph, IC->w, IC->r) != OK)
             return NOTOK;
     }

     if (theGraph->functions.fpMarkDFSPath(theGraph, MIN3(IC->ux, IC->uy, IC->uz),
                                    MAX3(IC->ux, IC->uy, IC->uz)) != OK ||
         _MarkDFSPathsToDescendants(theGraph) != OK ||
         _JoinBicomps(theGraph) != OK ||
         _AddAndMarkUnembeddedEdges(theGraph) != OK)
         return NOTOK;

     theGraph->IC.minorType |= MINORTYPE_E4;
     return OK;
}

/****************************************************************************
 _GetLeastAncestorConnection()

 This function searches for an ancestor of the current vertex I adjacent by a
 cycle edge to the given cutVertex or one of its DFS descendants appearing in
 a separated bicomp. The given cutVertex is assumed to be externally active
 such that either the leastAncestor or the lowpoint of a separated DFS child
 is less than I.  We obtain the minimum possible connection from the cutVertex
 to an ancestor of I.
 ****************************************************************************/

int  _GetLeastAncestorConnection(graphP theGraph, int cutVertex)
{
int  subtreeRoot = theGraph->V[cutVertex].separatedDFSChildList;
int  ancestor = theGraph->V[cutVertex].leastAncestor;

     if (subtreeRoot != NIL &&
         ancestor > theGraph->V[subtreeRoot].Lowpoint)
         ancestor = theGraph->V[subtreeRoot].Lowpoint;

     return ancestor;
}

/****************************************************************************
 _FindUnembeddedEdgeToAncestor()

 This function searches for an ancestor of the current vertex I adjacent by a
 cycle edge to the given cutVertex or one of its DFS descendants appearing in
 a separated bicomp.

 The given cutVertex is assumed to be externally active such that either the
 leastAncestor or the lowpoint of a separated DFS child is less than I.
 We obtain the minimum possible connection from the cutVertex to an ancestor
 of I, then compute the descendant accordingly.
 ****************************************************************************/

int  _FindUnembeddedEdgeToAncestor(graphP theGraph, int cutVertex,
                                   int *pAncestor, int *pDescendant)
{
     *pAncestor = _GetLeastAncestorConnection(theGraph, cutVertex);

     if (*pAncestor == theGraph->V[cutVertex].leastAncestor)
     {
         *pDescendant = cutVertex;
         return OK;
     }
     else
     {
     int subtreeRoot = theGraph->V[cutVertex].separatedDFSChildList;

         return _FindUnembeddedEdgeToSubtree(theGraph, *pAncestor,
                                             subtreeRoot, pDescendant);
     }
}

/****************************************************************************
 _FindUnembeddedEdgeToCurVertex()

 Given the current vertex I, we search for an edge connecting I to either
 a given pertinent vertex W or one of its DFS descendants in the subtree
 indicated by the the last pertinent child biconnected component.
 ****************************************************************************/

int  _FindUnembeddedEdgeToCurVertex(graphP theGraph, int cutVertex, int *pDescendant)
{
int  RetVal = OK, I = theGraph->IC.v;

     if (theGraph->V[cutVertex].adjacentTo != NIL)
         *pDescendant = cutVertex;
     else
     {
     int subtreeRoot = theGraph->V[cutVertex].pertinentBicompList;

         RetVal = _FindUnembeddedEdgeToSubtree(theGraph, I,
                                               subtreeRoot, pDescendant);
     }

     return RetVal;
}

/****************************************************************************
 _FindUnembeddedEdgeToSubtree()

 Given the root vertex of a DFS subtree and an ancestor of that subtree,
 find a vertex in the subtree that is adjacent to the ancestor by a
 cycle edge.
 ****************************************************************************/

int  _FindUnembeddedEdgeToSubtree(graphP theGraph, int ancestor,
                                  int SubtreeRoot, int *pDescendant)
{
int  J, Z, ZNew;

     *pDescendant = NIL;

/* If SubtreeRoot is a root copy, then we change to the DFS child in the
        DFS tree root edge of the bicomp rooted by SubtreeRoot. */

     if (SubtreeRoot >= theGraph->N)
         SubtreeRoot -= theGraph->N;

/* Find the least descendant of the cut vertex incident to the ancestor. */

     J = theGraph->V[ancestor].fwdArcList;
     while (J != NIL)
     {
          if (theGraph->G[J].v >= SubtreeRoot)
          {
              if (*pDescendant == NIL || *pDescendant > theGraph->G[J].v)
                  *pDescendant = theGraph->G[J].v;
          }

          J = theGraph->G[J].link[0];
          if (J == theGraph->V[ancestor].fwdArcList)
              J = NIL;
     }

     if (*pDescendant == NIL) return NOTOK;

/* Make sure the identified descendant actually descends from the cut vertex */

     Z = *pDescendant;
     while (Z != SubtreeRoot)
     {
         ZNew = theGraph->V[Z].DFSParent;
         if (ZNew == NIL || ZNew == Z)
             return NOTOK;
         Z = ZNew;
     }

/* Return successfully */

     return OK;
}


/****************************************************************************
 _MarkPathAlongBicompExtFace()

 Sets the visited flags of vertices and edges on the external face of a
 bicomp from startVert to endVert, inclusive, by following the link[0] out of
 each visited vertex.
 ****************************************************************************/

int  _MarkPathAlongBicompExtFace(graphP theGraph, int startVert, int endVert)
{
int  Z, ZPrevLink, ZPrevArc;

/* Mark the start vertex (and if it is a root copy, mark the parent copy too. */

     theGraph->G[startVert].visited = 1;

/* For each vertex visited after the start vertex, mark the vertex and the
        edge used to get there.  Stop after marking the ending vertex. */

     Z = startVert;
     ZPrevLink = 1;
     do {
        Z = _GetNextVertexOnExternalFace(theGraph, Z, &ZPrevLink);

        ZPrevArc = theGraph->G[Z].link[ZPrevLink];

        theGraph->G[ZPrevArc].visited = 1;
        theGraph->G[gp_GetTwinArc(theGraph, ZPrevArc)].visited = 1;
        theGraph->G[Z].visited = 1;

     } while (Z != endVert);

     return OK;
}

/****************************************************************************
 _MarkDFSPath()

 Sets visited flags of vertices and edges from descendant to ancestor,
 including root copy vertices, and including the step of hopping from
 a root copy to its parent copy.

 The DFSParent of a vertex indicates the next vertex to visit, so we
 search the adjacency list looking for the edge leading either to the
 DFSParent or to a root copy of the DFSParent.  We start by marking the
 descendant, then traverse up the DFS tree, stopping after we mark
 the ancestor.
 ****************************************************************************/

int  _MarkDFSPath(graphP theGraph, int ancestor, int descendant)
{
int  J, parent, Z, N;

     N = theGraph->N;

     /* If we are marking from a root vertex upward, then go up to the parent
        copy before starting the loop */

     if (descendant >= N)
         descendant = theGraph->V[descendant-N].DFSParent;

     /* Mark the lowest vertex (i.e. the descendant with the highest number) */
     theGraph->G[descendant].visited = 1;

     /* Mark all ancestors of the lowest vertex, and the edges used to reach
        them, up to the given ancestor vertex. */

     while (descendant != ancestor)
     {
          /* Get the parent vertex */

          parent = theGraph->V[descendant].DFSParent;

          /* If the descendant was a DFS tree root, then obviously
                we aren't going to find the ancestor, so something is wrong.*/

          if (parent == NIL || parent == descendant)
              return NOTOK;

          /* Find the edge from descendant that leads either to
                parent or to a root copy of the parent.
                When the edge is found, mark it and break the loop */

          J = theGraph->G[descendant].link[0];
          while (J >= theGraph->edgeOffset)
          {
              Z = theGraph->G[J].v;
              if ((Z < N && Z == parent) ||
                  (Z >= N && theGraph->V[Z-N].DFSParent == parent))
              {
                  theGraph->G[J].visited = 1;
                  theGraph->G[gp_GetTwinArc(theGraph, J)].visited = 1;
                  break;
              }
              J = theGraph->G[J].link[0];
          }

          /* Mark the parent copy of the DFS parent */
          theGraph->G[parent].visited = 1;

          /* Hop to the parent */
          descendant = parent;
     }

     return OK;
}

/****************************************************************************
 _MarkDFSPathsToDescendants()
 ****************************************************************************/

int  _MarkDFSPathsToDescendants(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (theGraph->functions.fpMarkDFSPath(theGraph, IC->x, IC->dx) != OK ||
         theGraph->functions.fpMarkDFSPath(theGraph, IC->y, IC->dy) != OK)
         return NOTOK;

     if (IC->dw != NIL)
         if (theGraph->functions.fpMarkDFSPath(theGraph, IC->w, IC->dw) != OK)
             return NOTOK;

     if (IC->dz != NIL)
         if (theGraph->functions.fpMarkDFSPath(theGraph, IC->w, IC->dz) != OK)
             return NOTOK;

     return OK;
}

/****************************************************************************
 _AddAndMarkUnembeddedEdges()
 ****************************************************************************/

int  _AddAndMarkUnembeddedEdges(graphP theGraph)
{
isolatorContextP IC = &theGraph->IC;

     if (_AddAndMarkEdge(theGraph, IC->ux, IC->dx) != OK ||
         _AddAndMarkEdge(theGraph, IC->uy, IC->dy) != OK)
         return NOTOK;

     if (IC->dw != NIL)
         if (_AddAndMarkEdge(theGraph, IC->v, IC->dw) != OK)
             return NOTOK;

     if (IC->dz != NIL)
         if (_AddAndMarkEdge(theGraph, IC->uz, IC->dz) != OK)
             return NOTOK;

     return OK;
}

/****************************************************************************
 _AddAndMarkEdge()

 Adds edge records for the edge (ancestor, descendant) and marks the edge
 records and vertex structures that represent the edge.
 ****************************************************************************/

int _AddAndMarkEdge(graphP theGraph, int ancestor, int descendant)
{
    _AddBackEdge(theGraph, ancestor, descendant);

    /* Mark the edge so it is not deleted */

    theGraph->G[ancestor].visited = 1;
    theGraph->G[theGraph->G[ancestor].link[0]].visited = 1;
    theGraph->G[theGraph->G[descendant].link[0]].visited = 1;
    theGraph->G[descendant].visited = 1;

    return OK;
}

/****************************************************************************
 _AddBackEdge()

 This function transfers the edge records for the edge between the ancestor
 and descendant from the forward edge list of the ancestor to the adjacency
 lists of the ancestor and descendant.
 ****************************************************************************/

void _AddBackEdge(graphP theGraph, int ancestor, int descendant)
{
int fwdArc, backArc;

    /* We get the two edge records of the back edge to embed. */

     fwdArc = theGraph->V[ancestor].fwdArcList;
     while (fwdArc != NIL)
     {
          if (theGraph->G[fwdArc].v == descendant)
              break;

          fwdArc = theGraph->G[fwdArc].link[0];
          if (fwdArc == theGraph->V[ancestor].fwdArcList)
              fwdArc = NIL;
     }

     if (fwdArc == NIL)
         return;

    backArc = gp_GetTwinArc(theGraph, fwdArc);

    /* The forward arc is removed from the fwdArcList of the ancestor. */

    if (theGraph->V[ancestor].fwdArcList == fwdArc)
    {
        if (theGraph->G[fwdArc].link[0] == fwdArc)
             theGraph->V[ancestor].fwdArcList = NIL;
        else theGraph->V[ancestor].fwdArcList = theGraph->G[fwdArc].link[0];
    }

    theGraph->G[theGraph->G[fwdArc].link[0]].link[1] = theGraph->G[fwdArc].link[1];
    theGraph->G[theGraph->G[fwdArc].link[1]].link[0] = theGraph->G[fwdArc].link[0];

    /* The forward arc is added to the adjacency list of the ancestor. */

    theGraph->G[fwdArc].link[1] = ancestor;
    theGraph->G[fwdArc].link[0] = theGraph->G[ancestor].link[0];
    theGraph->G[theGraph->G[ancestor].link[0]].link[1] = fwdArc;
    theGraph->G[ancestor].link[0] = fwdArc;

    /* The back arc is added to the adjacency list of the descendant. */

    theGraph->G[backArc].v = ancestor;

    theGraph->G[backArc].link[1] = descendant;
    theGraph->G[backArc].link[0] = theGraph->G[descendant].link[0];
    theGraph->G[theGraph->G[descendant].link[0]].link[1] = backArc;
    theGraph->G[descendant].link[0] = backArc;
}

/****************************************************************************
 _DeleteUnmarkedVerticesAndEdges()

 For each vertex, traverse its adjacency list and delete all unvisited edges.
 ****************************************************************************/

int  _DeleteUnmarkedVerticesAndEdges(graphP theGraph)
{
int  I, J, fwdArc, descendant;

     /* All of the forward and back arcs of all of the edge records
        were removed from the adjacency lists in the planarity algorithm
        preprocessing.  We now put them back into the adjacency lists
        (and we do not mark them), so they can be properly deleted below. */

     for (I = 0; I < theGraph->N; I++)
     {
         while (theGraph->V[I].fwdArcList != NIL)
         {
             fwdArc = theGraph->V[I].fwdArcList;
             descendant = theGraph->G[fwdArc].v;
             _AddBackEdge(theGraph, I, descendant);
         }
     }

     /* Now we delete all unmarked edges.  We don't delete vertices
        from the embedding, but the ones we should delete will become
        degree zero. */

     for (I = 0; I < theGraph->N; I++)
     {
          J = theGraph->G[I].link[0];
          while (J >= theGraph->edgeOffset)
          {
                if (theGraph->G[J].visited)
                     J = theGraph->G[J].link[0];
                else J = gp_DeleteEdge(theGraph, J, 0);
          }
     }

     return OK;
}
