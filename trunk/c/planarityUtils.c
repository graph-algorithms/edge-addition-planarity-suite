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

#include "planarity.h"

/****************************************************************************
 Configuration
 ****************************************************************************/

char Mode='r',
     OrigOut='n',
     EmbeddableOut='n',
     ObstructedOut='n',
     AdjListsForEmbeddingsOut='n',
     quietMode='n';

void Reconfigure()
{
     fflush(stdin);

     Message("\nDo you want to \n"
    		 "  Randomly generate graphs (r),\n"
    		 "  Specify a graph (s),\n"
    		 "  Randomly generate a maximal planar graph (m), or\n"
    		 "  Randomly generate a non-planar graph (n)?");
     scanf(" %c", &Mode);

     Mode = tolower(Mode);
     if (!strchr("rsmn", Mode))
    	 Mode = 's';

     if (Mode == 'r')
     {
        Message("\nNOTE: The directories for the graphs you want must exist.\n\n");

        Message("Do you want original graphs in directory 'random' (last 10 max)?");
        scanf(" %c", &OrigOut);

        Message("Do you want adj. matrix of embeddable graphs in directory 'embedded' (last 10 max))?");
        scanf(" %c", &EmbeddableOut);

        Message("Do you want adj. matrix of obstructed graphs in directory 'obstructed' (last 10 max)?");
        scanf(" %c", &ObstructedOut);

        Message("Do you want adjacency list format of embeddings in directory 'adjlist' (last 10 max)?");
        scanf(" %c", &AdjListsForEmbeddingsOut);
     }

     Message("\n");
}

/****************************************************************************
 MESSAGE - prints a string, but when debugging adds \n and flushes stdout
 ****************************************************************************/

#define MAXLINE 1024
char Line[MAXLINE];

void Message(char *message)
{
	if (quietMode == 'n')
	{
	    fprintf(stdout, "%s", message);

#ifdef DEBUG
	    fprintf(stdout, "\n");
	    fflush(stdout);
#endif
	}
}

void ErrorMessage(char *message)
{
	if (quietMode == 'n')
	{
		fprintf(stderr, "%s", message);

#ifdef DEBUG
		fprintf(stderr, "\n");
		fflush(stderr);
#endif
	}
}

/****************************************************************************
 ****************************************************************************/

void SaveAsciiGraph(graphP theGraph, char *graphName)
{
char Ch;

    Message("Do you want to save the graph in Ascii format (to test.dat)?");
    scanf(" %c", &Ch);
    fflush(stdin);

    if (Ch == 'y')
    {
        int  e, limit;
        FILE *outfile = fopen("test.dat", "wt");
        fprintf(outfile, "%s\n", graphName);

        limit = theGraph->edgeOffset + 2*(theGraph->M + sp_GetCurrentSize(theGraph->edgeHoles));

        for (e = theGraph->edgeOffset; e < limit; e+=2)
        {
            if (theGraph->G[e].v != NIL)
                fprintf(outfile, "%d %d\n", theGraph->G[e].v+1, theGraph->G[e+1].v+1);
        }

        fprintf(outfile, "0 0\n");

        fclose(outfile);
    }
}

/****************************************************************************
 ****************************************************************************/

int GetEmbedFlags(char command)
{
	int embedFlags = 0;

	switch (command)
	{
		case 'o' : embedFlags = EMBEDFLAGS_OUTERPLANAR; break;
		case 'p' : embedFlags = EMBEDFLAGS_PLANAR; break;
		case 'd' : embedFlags = EMBEDFLAGS_DRAWPLANAR; break;
		case '2' : embedFlags = EMBEDFLAGS_SEARCHFORK23; break;
		case '3' : embedFlags = EMBEDFLAGS_SEARCHFORK33; break;
		case '4' : embedFlags = EMBEDFLAGS_SEARCHFORK4; break;
	}

	return embedFlags;
}
