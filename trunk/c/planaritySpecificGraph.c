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

#define FILENAMEMAXLENGTH 128
#define ALGORITHMNAMEMAXLENGTH 32
#define SUFFIXMAXLENGTH 32

void ConstructPrimaryOutputFilename(char *theFileName, char *outfileName, char *algorithmName);

/****************************************************************************
 SpecificGraph()
 ****************************************************************************/

int SpecificGraph(char command, char *infileName, char *outfileName, char *outfile2Name)
{
graphP theGraph = gp_New();
char theFileName[FILENAMEMAXLENGTH+1+ALGORITHMNAMEMAXLENGTH+1+SUFFIXMAXLENGTH+1];
char *algorithmName = "";
int  Result;

	// Get the filename of the graph to test
	if (infileName == NULL)
	{
		Message("Enter graph file name: ");
		scanf(" %s", theFileName);

		if (!strchr(theFileName, '.'))
			strcat(theFileName, ".txt");
	}
	else
	{
		if (strlen(infileName) > FILENAMEMAXLENGTH)
		{
			ErrorMessage("Filename is too long");
			return NOTOK;
		}
		strcpy(theFileName, infileName);
	}

    // Read the graph into memory
	Result = gp_Read(theGraph, theFileName);
	if (Result == NONEMBEDDABLE)
	{
		Message("The graph contains too many edges.\n");
		// Some of the algorithms will still run correctly with some edges removed.
		if (strchr("pdo234", command))
		{
			Message("Some edges were removed, but the algorithm will still run correctly.\n");
			Result = OK;
		}
	}

	// If there was an unrecoverable error, report it
	if (Result != OK)
		ErrorMessage("Failed to read graph\n");

	// Otherwise, call the correct algorithm on it
	else
	{
        graphP origGraph = gp_DupGraph(theGraph);
        platform_time start, end;

    	switch (command)
    	{
    		case 'p' :
    	        platform_GetTime(start);
    			Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph is%s planar.\n", Result==OK ? "" : " not");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = "PlanarEmbed";
    			break;
    		case 'd' :
    			gp_AttachDrawPlanar(theGraph);
    	        platform_GetTime(start);
    			Result = gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph is%s planar.\n", Result==OK ? "" : " not");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = DRAWPLANAR_NAME;
    			break;
    		case 'o' :
    	        platform_GetTime(start);
    			Result = gp_Embed(theGraph, EMBEDFLAGS_OUTERPLANAR);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph is%s outer planar.\n", Result==OK ? "" : " not");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = "OuterplanarEmbed";
    			break;
    		case '2' :
    			gp_AttachK23Search(theGraph);
    	        platform_GetTime(start);
    			Result = gp_Embed(theGraph, EMBEDFLAGS_SEARCHFORK23);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph %s a subgraph homeomorphic to K_{2,3}.\n", Result==OK ? "does not contain" : "contains");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = K23SEARCH_NAME;
    			break;
    		case '3' :
    			gp_AttachK33Search(theGraph);
    	        platform_GetTime(start);
				Result = gp_Embed(theGraph, EMBEDFLAGS_SEARCHFORK33);
		        platform_GetTime(end);
    	        sprintf(Line, "The graph %s a subgraph homeomorphic to K_{3,3}.\n", Result==OK ? "does not contain" : "contains");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = K33SEARCH_NAME;
				break;
    		case '4' :
    			gp_AttachK4Search(theGraph);
    	        platform_GetTime(start);
    			Result = gp_Embed(theGraph, EMBEDFLAGS_SEARCHFORK4);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph %s a subgraph homeomorphic to K_4.\n", Result==OK ? "does not contain" : "contains");
    	        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
    	        algorithmName = K4SEARCH_NAME;
    			break;
    		case 'c' :
    			gp_AttachColorVertices(theGraph);
    	        platform_GetTime(start);
    			Result = gp_ColorVertices(theGraph);
    	        platform_GetTime(end);
    	        sprintf(Line, "The graph has been %d-colored.\n", gp_GetNumColorsUsed(theGraph));
    	        Result = gp_ColorVerticesIntegrityCheck(theGraph, origGraph);
    	        algorithmName = COLORVERTICES_NAME;
    			break;
    		default :
    	        platform_GetTime(start);
    			Result = NOTOK;
    	        platform_GetTime(end);
    	        sprintf(Line, "Unrecognized Command\n");
    			break;
    	}

    	// Show the result message for the algorithm
        Message(Line);

    	// Report the length of time it took
        sprintf(Line, "Algorithm '%s' executed in %.3lf seconds.\n", algorithmName, platform_GetDuration(start,end));
        Message(Line);

        // Free the graph obtained for integrity checking.
        gp_Free(&origGraph);
	}

	// Report an error, if there was one, free the graph, and return
	if (Result != OK && Result != NONEMBEDDABLE)
	{
		ErrorMessage("AN ERROR HAS BEEN DETECTED\n");
		Result = NOTOK;
	}

	// Provide the output file(s)
	else
	{
        // Restore the vertex ordering of the original graph (undo DFS numbering)
        gp_SortVertices(theGraph);

        // Determine the name of the primary output file
        ConstructPrimaryOutputFilename(theFileName, outfileName, algorithmName);

        // For some algorithms, the primary output file is not always written
        outfileName = theFileName;
        if ((strchr("pdo", command) && Result == NONEMBEDDABLE) ||
        	(strchr("234", command) && Result == OK))
        	outfileName = NULL;

        // Write the primary output file, if appropriate to do so
        if (outfileName != NULL)
			gp_Write(theGraph, outfileName, WRITE_ADJLIST);

        // NOW WE WANT TO WRITE THE SECONDARY OUTPUT FILE

		// When called from the menu system, we want to write the planar or outerplanar
		// obstruction, if one exists. For planar graph drawing, we want the character
        // art rendition.  A non-NULL empty string is passed to indicate the necessity
        // of selecting a default name for the second output file.
		if (outfile2Name != NULL && strlen(outfile2Name) == 0)
		{
			if (command == 'p' || command == 'o')
				outfile2Name = theFileName;

			else if (command == 'd' && Result == OK)
			{
				strcat(theFileName, ".render.txt");
				outfile2Name = theFileName;
			}
		}

        // Write the secondary output file, if it is required
        if (outfile2Name != NULL)
        {
			// For planar and outerplanar embedding, the secondary file receives
			// the obstruction to embedding
			if (command == 'p' || command == 'o')
			{
				if (Result == NONEMBEDDABLE)
					gp_Write(theGraph, outfile2Name, WRITE_ADJLIST);
			}
			// For planar graph drawing, the secondary file receives the drawing
			else if (command == 'd')
			{
				if (Result == OK)
					gp_DrawPlanar_RenderToFile(theGraph, outfile2Name);
			}
			// The secondary file should not have been provided otherwise
			else
			{
				ErrorMessage("Unsupported command for secondary output file request.");
			}
        }
	}

	// Free the graph and return the result
	gp_Free(&theGraph);
	return Result;
}

/****************************************************************************
 ConstructPrimaryOutputFilename()
 ****************************************************************************/

void ConstructPrimaryOutputFilename(char *theFileName, char *outfileName, char *algorithmName)
{
	if (outfileName == NULL)
	{
		// If the primary output filename has not been given, then we use
		// the input filename + the algorithm name + a simple suffix
		if (strlen(algorithmName) <= ALGORITHMNAMEMAXLENGTH)
		{
			strcat(theFileName, ".");
			strcat(theFileName, algorithmName);
		}
		else
			ErrorMessage("Algorithm Name is too long, so it will not be used in output filename.");

	    strcat(theFileName, ".out.txt");
	}
	else
	{
		if (strlen(outfileName) > FILENAMEMAXLENGTH)
		{
	    	if (strlen(algorithmName) <= ALGORITHMNAMEMAXLENGTH)
	    	{
	    		strcat(theFileName, ".");
	    		strcat(theFileName, algorithmName);
	    	}
	        strcat(theFileName, ".out.txt");
			sprintf(Line, "Outfile filename is too long. Result placed in %s", theFileName);
			ErrorMessage(Line);
		}
		else
			strcpy(theFileName, outfileName);
	}
}

