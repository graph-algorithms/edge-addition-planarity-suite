/*
Copyright (c) 1997-2022, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "graph.h"
#include "g6-read-iterator.h"

/* Private functions (exported to system) */

int  _ReadAdjMatrix(graphP theGraph, FILE *Infile, strBufP inBuf);
int  _ReadAdjList(graphP theGraph, FILE *Infile, strBufP inBuf);
int  _WriteAdjList(graphP theGraph, FILE *Outfile, strBufP outBuf);
int  _WriteAdjMatrix(graphP theGraph, FILE *Outfile, strBufP outBuf);
int  _WriteDebugInfo(graphP theGraph, FILE *Outfile);

/********************************************************************
 _ReadAdjMatrix()
 This function reads the undirected graph in upper triangular matrix format.
 Though O(N^2) time is required, this routine is useful during
 reliability testing due to the wealth of graph generating software
 that uses this format for output.
 Returns: OK, NOTOK on internal error, NONEMBEDDABLE if too many edges
 ********************************************************************/

int _ReadAdjMatrix(graphP theGraph, FILE *Infile, strBufP inBuf)
{
	int N = -1;
    int v, w, Flag;

    if (Infile == NULL  && inBuf == NULL)
    	return NOTOK;

    // Read the number of vertices from the first line of the file
    if (Infile != NULL)
    	fscanf(Infile, " %d ", &N);
    else
    {
    	sb_ReadSkipWhitespace(inBuf);
    	sscanf(sb_GetReadString(inBuf), " %d ", &N);
    	sb_ReadSkipInteger(inBuf);
    	sb_ReadSkipWhitespace(inBuf);
    }

    // Initialize the graph based on the number of vertices
    if (gp_InitGraph(theGraph, N) != OK)
        return NOTOK;

    // Read an upper-triangular matrix row for each vertex
    // Note that for the last vertex, zero flags are read, per the upper triangular format
    for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
    {
         gp_SetVertexIndex(theGraph, v, v);
         for (w = v+1; gp_VertexInRange(theGraph, w); w++)
         {
        	  // Read each of v's w-neighbor flags
        	  if (Infile != NULL)
        		  fscanf(Infile, " %1d", &Flag);
        	  else
        	  {
        		  sb_ReadSkipWhitespace(inBuf);
        		  sscanf(sb_GetReadString(inBuf),  " %1d", &Flag);
        		  sb_ReadSkipInteger(inBuf);
        	  }

              // Add the edge (v, w) if the flag is raised
              if (Flag)
              {
                  if (gp_AddEdge(theGraph, v, 0, w, 0) != OK)
               	      return NOTOK;
              }
         }
    }

    return OK;
}

/********************************************************************
 _ReadAdjList()
 This function reads the graph in adjacency list format.

 The file format is
 On the first line    : N= number of vertices
 On N subsequent lines: #: a b c ... -1
 where # is a vertex number and a, b, c, ... are its neighbors.

 NOTE:  The vertex number is for file documentation only.  It is an
        error if the vertices are not in sorted order in the file.

 NOTE:  If a loop edge is found, it is ignored without error.

 NOTE:  This routine supports digraphs.  For a directed arc (v -> W),
        an edge record is created in both vertices, v and W, and the
        edge record in v's adjacency list is marked OUTONLY while the
        edge record in W's list is marked INONLY.
        This makes it easy to used edge directedness when appropriate
        but also seamlessly process the corresponding undirected graph.

 Returns: OK on success, NONEMBEDDABLE if success except too many edges
 	 	  NOTOK on file content error (or internal error)
 ********************************************************************/

int  _ReadAdjList(graphP theGraph, FILE *Infile, strBufP inBuf)
{
     int N = -1;
     int v, W, adjList, e, indexValue, ErrorCode;
     int zeroBased = FALSE;

     if (Infile == NULL && inBuf == NULL)
    	 return NOTOK;

     // Skip the "N=" and then read the N value for number of vertices
     if (Infile != NULL)
     {
         fgetc(Infile);
         fgetc(Infile);
         fscanf(Infile, " %d ", &N);
     }
     else
     {
    	 sb_ReadSkipChar(inBuf);
    	 sb_ReadSkipChar(inBuf);
     	 sb_ReadSkipWhitespace(inBuf);
     	 sscanf(sb_GetReadString(inBuf), " %d ", &N);
     	 sb_ReadSkipInteger(inBuf);
     	 sb_ReadSkipWhitespace(inBuf);
     }

     // Initialize theGraph based on the number of vertices in the input
     if (gp_InitGraph(theGraph, N) != OK)
          return NOTOK;

     // Clear the visited members of the vertices so they can be used
     // during the adjacency list read operation
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
          gp_SetVertexVisitedInfo(theGraph, v, NIL);

     // Do the adjacency list read operation for each vertex in order
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
     {
          // Read the vertex number
    	  if (Infile != NULL)
    		  fscanf(Infile, "%d", &indexValue);
    	  else
    	  {
    		  sscanf(sb_GetReadString(inBuf), "%d", &indexValue);
    		  sb_ReadSkipInteger(inBuf);
    	  }

          if (indexValue == 0 && v == gp_GetFirstVertex(theGraph))
        	  zeroBased = TRUE;
          indexValue += zeroBased ? gp_GetFirstVertex(theGraph) : 0;

          gp_SetVertexIndex(theGraph, v, indexValue);

          // The vertices are expected to be in numeric ascending order
          if (gp_GetVertexIndex(theGraph, v) != v)
        	  return NOTOK;

          // Skip the colon after the vertex number
          if (Infile != NULL)
        	  fgetc(Infile);
          else
        	  sb_ReadSkipChar(inBuf);

          // If the vertex already has a non-empty adjacency list, then it is
          // the result of adding edges during processing of preceding vertices.
          // The list is removed from the current vertex v and saved for use
          // during the read operation for v.  Adjacencies to preceding vertices
          // are pulled from this list, if present, or added as directed edges
          // if not.  Adjacencies to succeeding vertices are added as undirected
          // edges, and will be corrected later if the succeeding vertex does not
          // have the matching adjacency using the following mechanism.  After the
          // read operation for a vertex v, any adjacency nodes left in the saved
          // list are converted to directed edges from the preceding vertex to v.
          adjList = gp_GetFirstArc(theGraph, v);
          if (gp_IsArc(adjList))
          {
        	  // Store the adjacency node location in the visited member of each
        	  // of the preceding vertices to which v is adjacent so that we can
        	  // efficiently detect the adjacency during the read operation and
        	  // efficiently find the adjacency node.
        	  e = gp_GetFirstArc(theGraph, v);
			  while (gp_IsArc(e))
			  {
				  gp_SetVertexVisitedInfo(theGraph, gp_GetNeighbor(theGraph, e), e);
				  e = gp_GetNextArc(theGraph, e);
			  }

        	  // Make the adjacency list circular, for later ease of processing
			  gp_SetPrevArc(theGraph, adjList, gp_GetLastArc(theGraph, v));
			  gp_SetNextArc(theGraph, gp_GetLastArc(theGraph, v), adjList);

        	  // Remove the list from the vertex
			  gp_SetFirstArc(theGraph, v, NIL);
			  gp_SetLastArc(theGraph, v, NIL);
          }

          // Read the adjacency list.
          while (1)
          {
        	 // Read the value indicating the next adjacent vertex (or the list end)
        	 if (Infile != NULL)
        		 fscanf(Infile, " %d ", &W);
        	 else
        	 {
             	 sb_ReadSkipWhitespace(inBuf);
             	 sscanf(sb_GetReadString(inBuf), " %d ", &W);
             	 sb_ReadSkipInteger(inBuf);
             	 sb_ReadSkipWhitespace(inBuf);
        	 }
             W += zeroBased ? gp_GetFirstVertex(theGraph) : 0;

             // A value below the valid range indicates the adjacency list end
             if (W < gp_GetFirstVertex(theGraph))
            	 break;

             // A value above the valid range is an error
             if (W > gp_GetLastVertex(theGraph))
            	 return NOTOK;

             // Loop edges are not supported
             else if (W == v)
            	 return NOTOK;

             // If the adjacency is to a succeeding, higher numbered vertex,
             // then we'll add an undirected edge for now
             else if (v < W)
             {
             	 if ((ErrorCode = gp_AddEdge(theGraph, v, 0, W, 0)) != OK)
             		 return ErrorCode;
             }

             // If the adjacency is to a preceding, lower numbered vertex, then
             // we have to pull the adjacency node from the preexisting adjList,
             // if it is there, and if not then we have to add a directed edge.
             else
             {
            	 // If the adjacency node (arc) already exists, then we add it
            	 // as the new first arc of the vertex and delete it from adjList
            	 if (gp_IsArc(gp_GetVertexVisitedInfo(theGraph, W)))
            	 {
            		 e = gp_GetVertexVisitedInfo(theGraph, W);

            		 // Remove the arc e from the adjList construct
            		 gp_SetVertexVisitedInfo(theGraph, W, NIL);
            		 if (adjList == e)
            		 {
            			 if ((adjList = gp_GetNextArc(theGraph, e)) == e)
            				 adjList = NIL;
            		 }
            		 gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, e), gp_GetPrevArc(theGraph, e));
            		 gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, e), gp_GetNextArc(theGraph, e));

            		 gp_AttachFirstArc(theGraph, v, e);
            	 }

            	 // If an adjacency node to the lower numbered vertex W does not
            	 // already exist, then we make a new directed arc from the current
            	 // vertex v to W.
            	 else
            	 {
            		 // It is added as the new first arc in both vertices
                	 if ((ErrorCode = gp_AddEdge(theGraph, v, 0, W, 0)) != OK)
                		 return ErrorCode;

					 // Note that this call also sets OUTONLY on the twin arc
					 gp_SetDirection(theGraph, gp_GetFirstArc(theGraph, W), EDGEFLAG_DIRECTION_INONLY);
            	 }
             }
          }

          // If there are still adjList entries after the read operation
          // then those entries are not representative of full undirected edges.
          // Rather, they represent incoming directed arcs from other vertices
          // into vertex v. They need to be added back into v's adjacency list but
          // marked as "INONLY", while the twin is marked "OUTONLY" (by the same function).
          while (gp_IsArc(adjList))
          {
        	  e = adjList;

        	  gp_SetVertexVisitedInfo(theGraph, gp_GetNeighbor(theGraph, e), NIL);

 			  if ((adjList = gp_GetNextArc(theGraph, e)) == e)
 				  adjList = NIL;

     		  gp_SetPrevArc(theGraph, gp_GetNextArc(theGraph, e), gp_GetPrevArc(theGraph, e));
     		  gp_SetNextArc(theGraph, gp_GetPrevArc(theGraph, e), gp_GetNextArc(theGraph, e));

     		  gp_AttachFirstArc(theGraph, v, e);
     		  gp_SetDirection(theGraph, e, EDGEFLAG_DIRECTION_INONLY);
          }
     }

     if (zeroBased)
    	theGraph->internalFlags |= FLAGS_ZEROBASEDIO;

     return OK;
}

/********************************************************************
 _ReadLEDAGraph()
 Reads the edge list from a LEDA file containing a simple undirected graph.
 LEDA files use a one-based numbering system, which is converted to
 zero-based numbers if the graph reports starting at zero as the first vertex.

 Returns: OK on success, NONEMBEDDABLE if success except too many edges
 	 	  NOTOK on file content error (or internal error)
 ********************************************************************/

int  _ReadLEDAGraph(graphP theGraph, FILE *Infile)
{
	char Line[256];
	int N = -1;
    int M, m, u, v, ErrorCode;
	int zeroBasedOffset = gp_GetFirstVertex(theGraph)==0 ? 1 : 0;

    /* Skip the lines that say LEDA.GRAPH and give the node and edge types */
    fgets(Line, 255, Infile);

    fgets(Line, 255, Infile);
    fgets(Line, 255, Infile);

    /* Read the number of vertices N, initialize the graph, then skip N. */
    fgets(Line, 255, Infile);
    sscanf(Line, " %d", &N);

    if (gp_InitGraph(theGraph, N) != OK)
         return NOTOK;

    for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
        fgets(Line, 255, Infile);

    /* Read the number of edges */
    fgets(Line, 255, Infile);
    sscanf(Line, " %d", &M);

    /* Read and add each edge, omitting loops and parallel edges */
    for (m = 0; m < M; m++)
    {
        fgets(Line, 255, Infile);
        sscanf(Line, " %d %d", &u, &v);
        if (u != v && !gp_IsNeighbor(theGraph, u-zeroBasedOffset, v-zeroBasedOffset))
        {
             if ((ErrorCode = gp_AddEdge(theGraph, u-zeroBasedOffset, 0, v-zeroBasedOffset, 0)) != OK)
                 return ErrorCode;
        }
    }

    if (zeroBasedOffset)
    	theGraph->internalFlags |= FLAGS_ZEROBASEDIO;

    return OK;
}

/********************************************************************
 gp_Read()
 Opens the given file, determines whether it is in adjacency list or
 matrix format based on whether the file start with N or just a number,
 calls the appropriate read function, then closes the file and returns
 the graph.

 Digraphs and loop edges are not supported in the adjacency matrix format,
 which is upper triangular.

 In the adjacency list format, digraphs are supported.  Loop edges are
 ignored without producing an error.

 Pass "stdin" for the FileName to read from the stdin stream

 Returns: OK, NOTOK on internal error, NONEMBEDDABLE if too many edges
 ********************************************************************/

int gp_Read(graphP theGraph, char *FileName)
{
FILE *Infile;
bool extraDataAllowed = false;
char lineBuff[255];
int RetVal;

    if (strcmp(FileName, "stdin") == 0)
        Infile = stdin;
    else if ((Infile = fopen(FileName, READTEXT)) == NULL)
        return NOTOK;

    fgets(lineBuff, 255, Infile);
    // Reset file pointer to beginning of file
    fseek(Infile, 0, SEEK_SET);
    if (strncmp(lineBuff, "LEDA.GRAPH", strlen("LEDA.GRAPH")) == 0)
        RetVal = _ReadLEDAGraph(theGraph, Infile);
    else if (strncmp(lineBuff, "N=", strlen("N=")) == 0)
    {
        RetVal = _ReadAdjList(theGraph, Infile, NULL);
        if (RetVal == OK)
            extraDataAllowed = true;
    }
    else if (isdigit(lineBuff[0]))
    {
        RetVal = _ReadAdjMatrix(theGraph, Infile, NULL);
        if (RetVal == OK)
            extraDataAllowed = true;
    }
    else
        RetVal = _ReadGraphFromG6FilePointer(theGraph, Infile);

    // The possibility of "extra data" is not allowed for .g6 format:
    // .g6 files may contain multiple graphs, which are not valid input
    // for the extra data readers (i.e. fpReadPostProcess) Additionally,
    // we don't want to add extra data if the graph is nonembeddable, as
    // the FILE pointer isn't necessarily advanced past the graph
    // encoding unless OK is returned.
    if (extraDataAllowed)
    {
        void *extraData = NULL;
        long filePos = ftell(Infile);
        long fileSize;

        fseek(Infile, 0, SEEK_END);
        fileSize = ftell(Infile);
        fseek(Infile, filePos, SEEK_SET);

        if (filePos < fileSize)
        {
        extraData = malloc(fileSize - filePos + 1);
        fread(extraData, fileSize - filePos, 1, Infile);
        }
/*
        // Useful for quick debugging of IO extensibility
        if (extraData == NULL)
            printf("extraData == NULL\n");
        else
        {
            char *extraDataString = (char *) extraData;
            extraDataString[fileSize - filePos] = '\0';
            printf("extraData = '%s'\n", extraDataString);
        }
*/

        if (extraData != NULL)
        {
            RetVal = theGraph->functions.fpReadPostprocess(theGraph, extraData, fileSize - filePos);
            free((void *) extraData);
        }
    }

    if (strcmp(FileName, "stdin") != 0)
        fclose(Infile);

    return RetVal;
}

/********************************************************************
 gp_ReadFromString()
 Populates theGraph using the information stored in inputStr.
 Supports adjacency list and adjacency matrix formats, not LEDA.
 Returns NOTOK for any error, or OK otherwise
 ********************************************************************/

int	 gp_ReadFromString(graphP theGraph, char *inputStr)
{
    int RetVal;
    char Ch;
    bool extraDataAllowed = false;

    strBufP inBuf = sb_New(0);
    if (inBuf == NULL)
        return NOTOK;

    if (sb_ConcatString(inBuf, inputStr) != OK)
    {
        sb_Free(&inBuf);
        return NOTOK;
    }

    if (strncmp(inputStr, "LEDA.GRAPH", strlen("LEDA.GRAPH")) == 0)
        return NOTOK;
    else if (strncmp(inputStr, "N=", strlen("N=")) == 0)
    {
        RetVal = _ReadAdjList(theGraph, NULL, inBuf);
        if (RetVal == OK)
            extraDataAllowed = true;
    }
    else if (isdigit(inputStr[0]))
    {
        RetVal = _ReadAdjMatrix(theGraph, NULL, inBuf);
        if (RetVal == OK)
            extraDataAllowed = true;
    }
    else
        RetVal = _ReadGraphFromG6String(theGraph, inputStr);

    // The possibility of "extra data" is not allowed for .g6 format:
    // .g6 files may contain multiple graphs, which are not valid input
    // for the extra data readers (i.e. fpReadPostProcess) Additionally,
    // we don't want to add extra data if the graph is nonembeddable, as
    // the FILE pointer isn't necessarily advanced past the graph
    // encoding unless OK is returned.
    if (extraDataAllowed)
     {
        char *extraData = sb_GetReadString(inBuf);
        int extraDataLen = extraData == NULL ? 0 : strlen(extraData);

        if (extraDataLen > 0)
            RetVal = theGraph->functions.fpReadPostprocess(theGraph, extraData, extraDataLen);
     }

    sb_Free(&inBuf);
    return RetVal;
}

int  _ReadPostprocess(graphP theGraph, void *extraData, long extraDataSize)
{
     return OK;
}

/********************************************************************
 _WriteAdjList()
 For each vertex, we write its number, a colon, the list of adjacent vertices,
 then a NIL.  The vertices occupy the first N positions of theGraph.  Each
 vertex is also has indicators of the first and last adjacency nodes (arcs)
 in its adjacency list.

 Returns: NOTOK for parameter errors; OK otherwise.
 ********************************************************************/

int  _WriteAdjList(graphP theGraph, FILE *Outfile, strBufP outBuf)
{
int v, e;
int zeroBasedOffset = (theGraph->internalFlags & FLAGS_ZEROBASEDIO) ? gp_GetFirstVertex(theGraph) : 0;
char numberStr[128];

     if (theGraph==NULL || (Outfile==NULL && outBuf == NULL))
    	 return NOTOK;

     // Write the number of vertices of the graph to the file or string buffer
     if (Outfile != NULL)
    	 fprintf(Outfile, "N=%d\n", theGraph->N);
     else
     {
    	 sprintf(numberStr, "N=%d\n", theGraph->N);
    	 if (sb_ConcatString(outBuf, numberStr) != OK)
    		 return NOTOK;
     }

     // Write the adjacency list of each vertex
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
     {
    	  if (Outfile != NULL)
    		  fprintf(Outfile, "%d:", v - zeroBasedOffset);
    	  else
    	  {
    		  sprintf(numberStr, "%d:", v - zeroBasedOffset);
    	      if (sb_ConcatString(outBuf, numberStr) != OK)
    	    	  return NOTOK;
    	  }

          e = gp_GetLastArc(theGraph, v);
          while (gp_IsArc(e))
          {
        	  if (gp_GetDirection(theGraph, e) != EDGEFLAG_DIRECTION_INONLY)
        	  {
        		  if (Outfile != NULL)
        			  fprintf(Outfile, " %d", gp_GetNeighbor(theGraph, e) - zeroBasedOffset);
        		  else
        		  {
        			  sprintf(numberStr, " %d", gp_GetNeighbor(theGraph, e) - zeroBasedOffset);
            	      if (sb_ConcatString(outBuf, numberStr) != OK)
            	    	  return NOTOK;
        		  }
        	  }

              e = gp_GetPrevArc(theGraph, e);
          }

          // Write NIL at the end of the adjacency list (in zero-based I/O, NIL was -1)
          if (Outfile != NULL)
        	  fprintf(Outfile, " %d\n", (theGraph->internalFlags & FLAGS_ZEROBASEDIO) ? -1 : NIL);
          else
          {
        	  sprintf(numberStr, " %d\n", (theGraph->internalFlags & FLAGS_ZEROBASEDIO) ? -1 : NIL);
    	      if (sb_ConcatString(outBuf, numberStr) != OK)
    	    	  return NOTOK;
          }
     }

     return OK;
}

/********************************************************************
 _WriteAdjMatrix()
 Outputs upper triangular matrix representation capable of being
 read by _ReadAdjMatrix().

 theGraph and one of Outfile or theStrBuf must be non-NULL.

 Note: This routine does not support digraphs and will return an
       error if a directed edge is found.

 returns OK for success, NOTOK for failure
 ********************************************************************/

int  _WriteAdjMatrix(graphP theGraph, FILE *Outfile, strBufP outBuf)
{
int  v, e, K;
char *Row = NULL;
char numberStr[128];

     if (theGraph == NULL || (Outfile == NULL && outBuf == NULL))
    	 return NOTOK;

     // Write the number of vertices in the graph to the file or string buffer
     if (Outfile != NULL)
    	 fprintf(Outfile, "%d\n", theGraph->N);
     else
     {
    	 sprintf(numberStr, "%d\n", theGraph->N);
    	 if (sb_ConcatString(outBuf, numberStr) != OK)
    		 return NOTOK;
     }

     // Allocate memory for storing a string expression of one row at a time
     Row = (char *) malloc((theGraph->N+2)*sizeof(char));
     if (Row == NULL)
         return NOTOK;

     // Construct the upper triangular matrix representation one row at a time
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
     {
          for (K = gp_GetFirstVertex(theGraph); K <= v; K++)
               Row[K - gp_GetFirstVertex(theGraph)] = ' ';
          for (K = v+1; gp_VertexInRange(theGraph, K); K++)
               Row[K - gp_GetFirstVertex(theGraph)] = '0';

          e = gp_GetFirstArc(theGraph, v);
          while (gp_IsArc(e))
          {
        	  if (gp_GetDirection(theGraph, e) == EDGEFLAG_DIRECTION_INONLY)
        		  return NOTOK;

              if (gp_GetNeighbor(theGraph, e) > v)
                  Row[gp_GetNeighbor(theGraph, e) - gp_GetFirstVertex(theGraph)] = '1';

              e = gp_GetNextArc(theGraph, e);
          }

          Row[theGraph->N] = '\n';
          Row[theGraph->N+1] = '\0';

          // Write the row to the file or string buffer
          if (Outfile != NULL)
        	  fprintf(Outfile, "%s", Row);
          else
        	  sb_ConcatString(outBuf, Row);
     }

     free(Row);
     return OK;
}

/********************************************************************
 ********************************************************************/

char _GetEdgeTypeChar(graphP theGraph, int e)
{
	char type = 'U';

	if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_CHILD)
		type = 'C';
	else if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_FORWARD)
		type = 'F';
	else if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_PARENT)
		type = 'P';
	else if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_BACK)
		type = 'B';
	else if (gp_GetEdgeType(theGraph, e) == EDGE_TYPE_RANDOMTREE)
		type = 'T';

	return type;
}

/********************************************************************
 ********************************************************************/

char _GetVertexObstructionTypeChar(graphP theGraph, int v)
{
	char type = 'U';

	if (gp_GetVertexObstructionType(theGraph, v) == VERTEX_OBSTRUCTIONTYPE_HIGH_RXW)
		type = 'X';
	else if (gp_GetVertexObstructionType(theGraph, v) == VERTEX_OBSTRUCTIONTYPE_LOW_RXW)
		type = 'x';
	if (gp_GetVertexObstructionType(theGraph, v) == VERTEX_OBSTRUCTIONTYPE_HIGH_RYW)
		type = 'Y';
	else if (gp_GetVertexObstructionType(theGraph, v) == VERTEX_OBSTRUCTIONTYPE_LOW_RYW)
		type = 'y';

	return type;
}

/********************************************************************
 _WriteDebugInfo()
 Writes adjacency list, but also includes the type value of each
 edge (e.g. is it DFS child  arc, forward arc or back arc?), and
 the L, A and DFSParent of each vertex.
 ********************************************************************/

int  _WriteDebugInfo(graphP theGraph, FILE *Outfile)
{
int v, e, EsizeOccupied;

     if (theGraph==NULL || Outfile==NULL) return NOTOK;

     /* Print parent copy vertices and their adjacency lists */

     fprintf(Outfile, "DEBUG N=%d M=%d\n", theGraph->N, theGraph->M);
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
     {
          fprintf(Outfile, "%d(P=%d,lA=%d,LowPt=%d,v=%d):",
                             v, gp_GetVertexParent(theGraph, v),
                                gp_GetVertexLeastAncestor(theGraph, v),
                                gp_GetVertexLowpoint(theGraph, v),
                                gp_GetVertexIndex(theGraph, v));

          e = gp_GetFirstArc(theGraph, v);
          while (gp_IsArc(e))
          {
              fprintf(Outfile, " %d(e=%d)", gp_GetNeighbor(theGraph, e), e);
              e = gp_GetNextArc(theGraph, e);
          }

          fprintf(Outfile, " %d\n", NIL);
     }

     /* Print any root copy vertices and their adjacency lists */

     for (v = gp_GetFirstVirtualVertex(theGraph); gp_VirtualVertexInRange(theGraph, v); v++)
     {
          if (!gp_VirtualVertexInUse(theGraph, v))
              continue;

          fprintf(Outfile, "%d(copy of=%d, DFS child=%d):",
                           v, gp_GetVertexIndex(theGraph, v),
                           gp_GetDFSChildFromRoot(theGraph, v));

          e = gp_GetFirstArc(theGraph, v);
          while (gp_IsArc(e))
          {
              fprintf(Outfile, " %d(e=%d)", gp_GetNeighbor(theGraph, e), e);
              e = gp_GetNextArc(theGraph, e);
          }

          fprintf(Outfile, " %d\n", NIL);
     }

     /* Print information about vertices and root copy (virtual) vertices */
     fprintf(Outfile, "\nVERTEX INFORMATION\n");
     for (v = gp_GetFirstVertex(theGraph); gp_VertexInRange(theGraph, v); v++)
     {
         fprintf(Outfile, "V[%3d] index=%3d, type=%c, first arc=%3d, last arc=%3d\n",
                          v,
                          gp_GetVertexIndex(theGraph, v),
                          (gp_IsVirtualVertex(theGraph, v) ? 'X' : _GetVertexObstructionTypeChar(theGraph, v)),
                          gp_GetFirstArc(theGraph, v),
                          gp_GetLastArc(theGraph, v));
     }
     for (v = gp_GetFirstVirtualVertex(theGraph); gp_VirtualVertexInRange(theGraph, v); v++)
     {
         if (gp_VirtualVertexNotInUse(theGraph, v))
             continue;

         fprintf(Outfile, "V[%3d] index=%3d, type=%c, first arc=%3d, last arc=%3d\n",
                          v,
                          gp_GetVertexIndex(theGraph, v),
                          (gp_IsVirtualVertex(theGraph, v) ? 'X' : _GetVertexObstructionTypeChar(theGraph, v)),
                          gp_GetFirstArc(theGraph, v),
                          gp_GetLastArc(theGraph, v));
     }

     /* Print information about edges */

     fprintf(Outfile, "\nEDGE INFORMATION\n");
     EsizeOccupied = gp_EdgeInUseIndexBound(theGraph);
     for (e = gp_GetFirstEdge(theGraph); e < EsizeOccupied; e++)
     {
          if (gp_EdgeInUse(theGraph, e))
          {
              fprintf(Outfile, "E[%3d] neighbor=%3d, type=%c, next arc=%3d, prev arc=%3d\n",
                               e,
                               gp_GetNeighbor(theGraph, e),
                               _GetEdgeTypeChar(theGraph, e),
                               gp_GetNextArc(theGraph, e),
                               gp_GetPrevArc(theGraph, e));
          }
     }

     return OK;
}

/********************************************************************
 gp_Write()
 Writes theGraph into the file.
 Pass "stdout" or "stderr" to FileName to write to the corresponding stream
 Pass WRITE_ADJLIST, WRITE_ADJMATRIX or WRITE_DEBUGINFO for the Mode

 NOTE: For digraphs, it is an error to use a mode other than WRITE_ADJLIST

 Returns NOTOK on error, OK on success.
 ********************************************************************/

int  gp_Write(graphP theGraph, char *FileName, int Mode)
{
FILE *Outfile;
int RetVal;

     if (theGraph == NULL || FileName == NULL)
    	 return NOTOK;

     if (strcmp(FileName, "nullwrite") == 0)
    	  return OK;

     if (strcmp(FileName, "stdout") == 0)
          Outfile = stdout;
     else if (strcmp(FileName, "stderr") == 0)
          Outfile = stderr;
     else if ((Outfile = fopen(FileName, WRITETEXT)) == NULL)
          return NOTOK;

     switch (Mode)
     {
         case WRITE_ADJLIST   :
        	 RetVal = _WriteAdjList(theGraph, Outfile, NULL);
             break;
         case WRITE_ADJMATRIX :
        	 RetVal = _WriteAdjMatrix(theGraph, Outfile, NULL);
             break;
         case WRITE_DEBUGINFO :
        	 RetVal = _WriteDebugInfo(theGraph, Outfile);
             break;
        // TODO: Issue 18
        // case WRITE_G6 :
        // 	 RetVal = _WriteG6(theGraph, Outfile);
        //      break;
         default :
        	 RetVal = NOTOK;
        	 break;
     }

     if (RetVal == OK)
     {
         void *extraData = NULL;
         long extraDataSize;

         RetVal = theGraph->functions.fpWritePostprocess(theGraph, &extraData, &extraDataSize);

         if (extraData != NULL)
         {
             if (!fwrite(extraData, extraDataSize, 1, Outfile))
                 RetVal = NOTOK;
             free(extraData);
         }
     }

     if (strcmp(FileName, "stdout") == 0 || strcmp(FileName, "stderr") == 0)
         fflush(Outfile);

     else if (fclose(Outfile) != 0)
         RetVal = NOTOK;

     return RetVal;
}

/********************************************************************
 * gp_WriteToString()
 *
 * Writes the information of theGraph into a string that is returned
 * to the caller via the pointer pointer pOutputStr.
 * The string is owned by the caller and should be released with
 * free() when the caller doesn't need the string anymore.
 * The format of the content written into the returned string is based
 * on the Mode parameter: WRITE_ADJLIST or WRITE_ADJMATRIX
 * (the WRITE_DEBUGINFO Mode is not supported at this time)

 NOTE: For digraphs, it is an error to use a mode other than WRITE_ADJLIST

 Returns NOTOK on error, or OK on success along with an allocated string
         *pOutputStr that the caller must free()
 ********************************************************************/
int  gp_WriteToString(graphP theGraph, char **pOutputStr, int Mode)
{
	 int RetVal;
	 strBufP outBuf = sb_New(0);

	 if (theGraph == NULL || pOutputStr == NULL || outBuf == NULL)
	 {
		 sb_Free(&outBuf);
	  	 return NOTOK;
	 }

	 switch (Mode)
	 {
	     case WRITE_ADJLIST   :
	    	  RetVal = _WriteAdjList(theGraph, NULL, outBuf);
	          break;
	     case WRITE_ADJMATRIX :
	          RetVal = _WriteAdjMatrix(theGraph, NULL, outBuf);
	          break;
	     default :
	          RetVal = NOTOK;
	          break;
	 }

	 if (RetVal == OK)
	 {
	     void *extraData = NULL;
	     long extraDataSize;

	     RetVal = theGraph->functions.fpWritePostprocess(theGraph, &extraData, &extraDataSize);

	     if (extraData != NULL)
	     {
	    	 for (int i = 0; i < extraDataSize; i++)
	    		 sb_ConcatChar(outBuf, ((char *) extraData)[i]);
	         free(extraData);
	     }
	 }

	 *pOutputStr = sb_TakeString(outBuf);
	 sb_Free(&outBuf);

     return RetVal;
}

/********************************************************************
 _WritePostprocess()

 By default, no additional information is written.
 ********************************************************************/

int  _WritePostprocess(graphP theGraph, void **pExtraData, long *pExtraDataSize)
{
     return OK;
}

/********************************************************************
 _Log()

 When the project is compiled with LOGGING enabled, this method writes
 a string to the file PLANARITY.LOG in the current working directory.
 On first write, the file is created or cleared.
 Call this method with NULL to close the log file.
 ********************************************************************/

void _Log(char *Str)
{
static FILE *logfile = NULL;

    if (logfile == NULL)
    {
        if ((logfile = fopen("PLANARITY.LOG", WRITETEXT)) == NULL)
        	return;
    }

    if (Str != NULL)
    {
        fprintf(logfile, "%s", Str);
        fflush(logfile);
    }
    else
        fclose(logfile);
}

void _LogLine(char *Str)
{
	_Log(Str);
	_Log("\n");
}

static char LogStr[512];

char *_MakeLogStr1(char *format, int one)
{
	sprintf(LogStr, format, one);
	return LogStr;
}

char *_MakeLogStr2(char *format, int one, int two)
{
	sprintf(LogStr, format, one, two);
	return LogStr;
}

char *_MakeLogStr3(char *format, int one, int two, int three)
{
	sprintf(LogStr, format, one, two, three);
	return LogStr;
}

char *_MakeLogStr4(char *format, int one, int two, int three, int four)
{
	sprintf(LogStr, format, one, two, three, four);
	return LogStr;
}

char *_MakeLogStr5(char *format, int one, int two, int three, int four, int five)
{
	sprintf(LogStr, format, one, two, three, four, five);
	return LogStr;
}
