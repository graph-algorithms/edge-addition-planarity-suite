/*
Planarity-Related Graph Algorithms Project
Copyright (c) 1997-2009, John M. Boyer
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

#include "graphColorVertices.h"
#include "graphColorVertices.private.h"

extern int DRAWPLANAR_ID;

#include "graph.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>

extern void _FillVisitedFlags(graphP theGraph, int FillValue);

/* Private functions exported to system */


/* Private functions */

/********************************************************************
 _ComputeEdgePositions()

  Performs a vertical sweep of the combinatorial planar embedding,
  developing the edge order in the horizontal sweep line as it
  advances through the vertices according to their assigned
  vertical positions.

  For expedience, the 'visited' flag for each vertex shall be used
  instead to indicate the location in the edge order list of the
  generator edge for the vertex, i.e. the first edge added to the
  vertex from a higher vertex (with lower position number).
  All edges added from this vertex to the neighbors below it are
  added immediately after the generator edge for the vertex.
 ********************************************************************/

int _ComputeEdgePositions(DrawPlanarContext *context)
{
graphP theEmbedding = context->theGraph;
int *vertexOrder = NULL;
listCollectionP edgeList = NULL;
int edgeListHead, edgeListInsertPoint;
int I, J, Jcur, e, v, vpos;
int eIndex, JTwin;

	gp_LogLine("\ngraphDrawPlanar.c/_ComputeEdgePositions() start");

    // Sort the vertices by vertical position (in linear time)

    if ((vertexOrder = (int *) malloc(theEmbedding->N * sizeof(int))) == NULL)
        return NOTOK;

    for (I = 0; I < theEmbedding->N; I++)
        vertexOrder[context->G[I].pos] = I;

    // Allocate the edge list of size M.
    //    This is an array of (prev, next) pointers.
    //    An edge at position X corresponds to the edge
    //    at position X in the graph structure, which is
    //    represented by a pair of adjacent graph nodes
    //    starting at index 2N + 2X.

    if (theEmbedding->M > 0 && (edgeList = LCNew(theEmbedding->M)) == NULL)
    {
        free(vertexOrder);
        return NOTOK;
    }

    edgeListHead = NIL;

    // Each vertex starts out with a NIL generator edge.

    for (I=0; I < theEmbedding->N; I++)
        theEmbedding->G[I].visited = NIL;

    // Perform the vertical sweep of the combinatorial embedding, using
    // the vertex ordering to guide the sweep.
    // For each vertex, each edge leading to a vertex with a higher number in
    // the vertex order is recorded as the "generator edge", or the edge of
    // first discovery of that higher numbered vertex, unless the vertex already has
    // a recorded generator edge
    for (vpos=0; vpos < theEmbedding->N; vpos++)
    {
        // Get the vertex associated with the position
        v = vertexOrder[vpos];
        gp_LogLine(gp_MakeLogStr3("Processing vertex %d with DFI=%d at position=%d",
    				 theEmbedding->G[v].v, v, vpos));

        // The DFS tree root of a connected component is always the least
        // number vertex in the vertex ordering.  We have to give it a
        // false generator edge so that it is still "visited" and then
        // all of its edges are generators for its neighbor vertices because
        // they all have greater numbers in the vertex order.
        if (theEmbedding->V[v].DFSParent == NIL)
        {
            // False generator edge, so the vertex is distinguishable from
            // a vertex with no generator edge when its neighbors are visited
            // This way, an edge from a neighbor won't get recorded as the
            // generator edge of the DFS tree root.
            theEmbedding->G[v].visited = 1;

            // Now we traverse the adjacency list of the DFS tree root and
            // record each edge as the generator edge of the neighbors
            J = gp_GetFirstArc(theEmbedding, v);
            while (gp_IsArc(theGraph, J))
            {
                e = (J - theEmbedding->edgeOffset) / 2;

                edgeListHead = LCAppend(edgeList, edgeListHead, e);
                gp_LogLine(gp_MakeLogStr2("Append generator edge (%d, %d) to edgeList",
                		theEmbedding->G[v].v, theEmbedding->G[theEmbedding->G[J].v].v));

                // Set the generator edge for the root's neighbor
                theEmbedding->G[theEmbedding->G[J].v].visited = J;

                // Go to the next node of the root's adj list
                J = gp_GetNextArc(theEmbedding, J);
            }
        }

        // Else, if we are not on a DFS tree root...
        else
        {
            // Get the generator edge of the vertex
            if ((JTwin = theEmbedding->G[v].visited) == NIL)
                return NOTOK;
            J = gp_GetTwinArc(theEmbedding, JTwin);

            // Traverse the edges of the vertex, starting
            // from the generator edge and going counterclockwise...

            e = (J - theEmbedding->edgeOffset) / 2;
            edgeListInsertPoint = e;

            Jcur = gp_GetNextArcCircular(theEmbedding, J);

            while (Jcur != J)
            {
                // If the neighboring vertex's position is greater
                // than the current vertex (meaning it is lower in the
                // diagram), then add that edge to the edge order.

                if (context->G[theEmbedding->G[Jcur].v].pos > vpos)
                {
                    e = (Jcur - theEmbedding->edgeOffset) / 2;
                    LCInsertAfter(edgeList, edgeListInsertPoint, e);

                    gp_LogLine(gp_MakeLogStr4("Insert (%d, %d) after (%d, %d)",
                    		theEmbedding->G[v].v,
                    		theEmbedding->G[theEmbedding->G[Jcur].v].v,
                    		theEmbedding->G[theEmbedding->G[gp_GetTwinArc(theEmbedding, J)].v].v,
                    		theEmbedding->G[theEmbedding->G[J].v].v));

                    edgeListInsertPoint = e;

                    // If the vertex does not yet have a generator edge, then set it.
                    if (theEmbedding->G[theEmbedding->G[Jcur].v].visited == NIL)
                    {
                        theEmbedding->G[theEmbedding->G[Jcur].v].visited = Jcur;
                        gp_LogLine(gp_MakeLogStr2("Generator edge (%d, %d)",
                        		theEmbedding->G[theEmbedding->G[gp_GetTwinArc(theEmbedding, J)].v].v,
                        		theEmbedding->G[theEmbedding->G[Jcur].v].v));
                    }
                }

                // Go to the next node in v's adjacency list
                Jcur = gp_GetNextArcCircular(theEmbedding, Jcur);
            }
        }
    }

    // Now iterate through the edgeList and assign positions to the edges.
    eIndex = 0;
    e = edgeListHead;
    while (e != NIL)
    {
        J = theEmbedding->edgeOffset + 2*e;
        JTwin = gp_GetTwinArc(theEmbedding, J);

        context->G[J].pos = context->G[JTwin].pos = eIndex;

        eIndex++;

        e = LCGetNext(edgeList, edgeListHead, e);
    }

    // Clean up and return
    LCFree(&edgeList);
    free(vertexOrder);

	gp_LogLine("graphDrawPlanar.c/_ComputeEdgePositions() end\n");

    return OK;
}

/********************************************************************
 _RenderToString()
 Draws the previously calculated visibility representation in a
 string of size (M+1)*2N + 1 characters, which should be deallocated
 with free().

 Returns NULL on failure, or the string containing the visibility
 representation otherwise.  The string can be printed using %s,
 ********************************************************************/

char *_RenderToString(graphP theEmbedding)
{
    DrawPlanarContext *context = NULL;
    gp_FindExtension(theEmbedding, DRAWPLANAR_ID, (void *) &context);

    if (context != NULL)
    {
        int N = theEmbedding->N;
        int M = theEmbedding->M;
        int I, J, e, Mid, Pos;
        char *visRep = (char *) malloc(sizeof(char) * ((M+1) * 2*N + 1));
        char numBuffer[32];

        if (visRep == NULL)
            return NULL;

        if (sp_NonEmpty(context->theGraph->edgeHoles))
        {
            free(visRep);
            return NULL;
        }

        // Clear the space
        for (I = 0; I < N; I++)
        {
            for (J=0; J < M; J++)
            {
                visRep[(2*I) * (M+1) + J] = ' ';
                visRep[(2*I+1) * (M+1) + J] = ' ';
            }

            visRep[(2*I) * (M+1) + M] = '\n';
            visRep[(2*I+1) * (M+1) + M] = '\n';
        }

        // Draw the vertices
        for (I = 0; I < N; I++)
        {
            Pos = context->G[I].pos;
            for (J=context->G[I].start; J<=context->G[I].end; J++)
                visRep[(2*Pos) * (M+1) + J] = '-';

            // Draw vertex label
            Mid = (context->G[I].start + context->G[I].end)/2;
            sprintf(numBuffer, "%d", I);
            if ((unsigned)(context->G[I].end - context->G[I].start + 1) >= strlen(numBuffer))
            {
                strncpy(visRep + (2*Pos) * (M+1) + Mid, numBuffer, strlen(numBuffer));
            }
            // If the vertex width is less than the label width, then fail gracefully
            else
            {
                if (strlen(numBuffer)==2)
                    visRep[(2*Pos) * (M+1) + Mid] = numBuffer[0];
                else
                    visRep[(2*Pos) * (M+1) + Mid] = '*';

                visRep[(2*Pos+1) * (M+1) + Mid] = numBuffer[strlen(numBuffer)-1];
            }
        }

        // Draw the edges
        for (e=0; e<M; e++)
        {
            J = 2*N + 2*e;
            Pos = context->G[J].pos;
            for (I=context->G[J].start; I<context->G[J].end; I++)
            {
                if (I > context->G[J].start)
                    visRep[(2*I) * (M+1) + Pos] = '|';
                visRep[(2*I+1) * (M+1) + Pos] = '|';
            }
        }

        // Null terminate string and return it
        visRep[(M+1) * 2*N] = '\0';
        return visRep;
    }

    return NULL;
}

/********************************************************************
 gp_DrawPlanar_RenderToFile()
 Creates a rendition of the planar graph visibility representation
 as a string, then dumps the string to the file.
 ********************************************************************/
int gp_DrawPlanar_RenderToFile(graphP theEmbedding, char *theFileName)
{
    if (sp_IsEmpty(theEmbedding->edgeHoles))
    {
        FILE *outfile;
        char *theRendition;

        if (strcmp(theFileName, "stdout") == 0)
             outfile = stdout;
        else if (strcmp(theFileName, "stderr") == 0)
             outfile = stderr;
        else outfile = fopen(theFileName, WRITETEXT);

        if (outfile == NULL)
            return NOTOK;

        theRendition = _RenderToString(theEmbedding);
        if (theRendition != NULL)
        {
            fprintf(outfile, "%s", theRendition);
            free(theRendition);
        }

        if (strcmp(theFileName, "stdout") == 0 || strcmp(theFileName, "stderr") == 0)
            fflush(outfile);

        else if (fclose(outfile) != 0)
            return NOTOK;

        return theRendition ? OK : NOTOK;
    }

    return NOTOK;
}

/********************************************************************
 _CheckVisibilityRepresentationIntegrity()
 ********************************************************************/

int _CheckVisibilityRepresentationIntegrity(DrawPlanarContext *context)
{
graphP theEmbedding = context->theGraph;
int I, e, J, JTwin, JPos, JIndex;

    if (sp_NonEmpty(context->theGraph->edgeHoles))
        return NOTOK;

    _FillVisitedFlags(theEmbedding, 0);

/* Test whether the vertex values make sense and
        whether the vertex positions are unique. */

    for (I = 0; I < theEmbedding->N; I++)
    {
    	if (theEmbedding->M > 0)
    	{
            if (context->G[I].pos < 0 ||
                context->G[I].pos >= theEmbedding->N ||
                context->G[I].start < 0 ||
                context->G[I].start > context->G[I].end ||
                context->G[I].end >= theEmbedding->M)
                return NOTOK;
    	}

        // Has the vertex position (context->G[I].pos) been used by a
        // vertex before vertex I?
        if (theEmbedding->G[context->G[I].pos].visited)
            return NOTOK;

        // Mark the vertex position as used by vertex I.
        // Note that this marking is made on some other vertex unrelated to I
        // We're just reusing the vertex visited array as cheap storage for a
        // detector of reusing vertex position integers.
        theEmbedding->G[context->G[I].pos].visited = 1;
    }

/* Test whether the edge values make sense and
        whether the edge positions are unique */

    for (e = 0; e < theEmbedding->M; e++)
    {
        /* Each edge has an index location J in the graph structure */
        J = theEmbedding->edgeOffset + 2*e;
        JTwin = gp_GetTwinArc(theEmbedding, J);

        if (context->G[J].pos != context->G[JTwin].pos ||
            context->G[J].start != context->G[JTwin].start ||
            context->G[J].end != context->G[JTwin].end ||
            context->G[J].pos < 0 ||
            context->G[J].pos >= theEmbedding->M ||
            context->G[J].start < 0 ||
            context->G[J].start > context->G[J].end ||
            context->G[J].end >= theEmbedding->N)
            return NOTOK;

        /* Get the recorded horizontal position of that edge,
            a number between 0 and M-1 */

        JPos = context->G[J].pos;

        /* Convert that to an index in the graph structure so we
            can use the visited flags in the graph's edges to
            tell us whether the positions are being reused. */

        JIndex = theEmbedding->edgeOffset + 2*JPos;
        JTwin = gp_GetTwinArc(theEmbedding, JIndex);

        if (theEmbedding->G[JIndex].visited || theEmbedding->G[JTwin].visited)
            return NOTOK;

        theEmbedding->G[JIndex].visited = theEmbedding->G[JTwin].visited = 1;
    }

/* Test whether any edge intersects any vertex position
    for a vertex that is not an endpoint of the edge. */

    for (e = 0; e < theEmbedding->M; e++)
    {
        J = theEmbedding->edgeOffset + 2*e;
        JTwin = gp_GetTwinArc(theEmbedding, J);

        for (I = 0; I < theEmbedding->N; I++)
        {
            /* If the vertex is an endpoint of the edge, then... */

            if (theEmbedding->G[J].v == I || theEmbedding->G[JTwin].v == I)
            {
                /* The vertical position of the vertex must be
                   at the top or bottom of the edge,  */
                if (context->G[J].start != context->G[I].pos &&
                    context->G[J].end != context->G[I].pos)
                    return NOTOK;

                /* The horizontal edge position must be in the range of the vertex */
                if (context->G[J].pos < context->G[I].start ||
                    context->G[J].pos > context->G[I].end)
                    return NOTOK;
            }

            /* If the vertex is not an endpoint of the edge... */

            else // if (theEmbedding->G[J].v != I && theEmbedding->G[JTwin].v != I)
            {
                /* If the vertical position of the vertex is in the
                    vertical range of the edge ... */

                if (context->G[J].start <= context->G[I].pos &&
                    context->G[J].end >= context->G[I].pos)
                {
                    /* And if the horizontal position of the edge is in the
                        horizontal range of the vertex, then return an error. */

                    if (context->G[I].start <= context->G[J].pos &&
                        context->G[I].end >= context->G[J].pos)
                        return NOTOK;
                }
            }
        }
    }


/* All tests passed */

    return OK;
}
