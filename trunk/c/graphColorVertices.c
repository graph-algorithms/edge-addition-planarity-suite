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

extern int COLORVERTICES_ID;

#include "graph.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>

extern void _FillVisitedFlags(graphP theGraph, int FillValue);

extern void _ColorVertices_Reinitialize(ColorVerticesContext *context);

/* Private functions exported to system */


/* Private functions */

void _AddVertexToDegList(ColorVerticesContext *context, graphP theGraph, int v, int deg);

/********************************************************************
 gp_ColorVertices()

 This is the entry point for requesting a vertex coloring by the
 the minimum degree selection method.

 The call pattern is to simply invoke this function on a graph to
 color it or recolor it after some mutations.  It will invoke
 gp_AttachColorVertices() to attach the auxiliary data needed to
 performing the coloring, and the attachment short-circuits if
 already done.

 After calling this function, call gp_ColorVertices_GetColors() to
 obtain the colors or gp_Write() to save the colors. To read a saved
 coloring, use gp_AttachColorVertices() then gp_Read().

 Returns OK on success, NOTOK on failure
 ********************************************************************/

int gp_ColorVertices(graphP theGraph)
{
    ColorVerticesContext *context = NULL;
    int v, deg;

	if (gp_AttachColorVertices(theGraph) != OK)
		return NOTOK;

    gp_FindExtension(theGraph, COLORVERTICES_ID, (void *)&context);

    if (color > -1)
    	_ColorVertices_Reinitialize(context);

    // Initialize the degree lists
    for (v = 0; v < theGraph->N; v++)
    {
    	deg = gp_GetVertexDegree(theGraph, v);
    	_AddVertexToDegList(context, theGraph, v, deg);
    }

    // Initialize the visited flags so they can be used during reductions
    _FillVisitedFlags(theGraph, 0);

    // Reduce the graph using minimum degree selection

    // Restore the graph, coloring each vertex distinctly from its neighbors
    // in the partially restored graph

	return NOTOK;
}

/********************************************************************
 ********************************************************************/

void _AddVertexToDegList(ColorVerticesContext *context, graphP theGraph, int v, int deg)
{
}

