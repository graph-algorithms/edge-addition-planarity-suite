/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include <stdlib.h>
#include <string.h>

#include "g6-write-iterator.h"
#include "g6-api-utilities.h"


int allocateG6WriteIterator(G6WriteIterator **ppG6WriteIterator, graphP pGraph)
{
	int exitCode = OK;

	if (ppG6WriteIterator != NULL && (*ppG6WriteIterator) != NULL)
	{
		ErrorMessage("G6WriteIterator is not NULL and therefore can't be allocated.\n");
		return NOTOK;
	}

	// numGraphsWritten, graphOrder, numCharsForGraphOrder,
	// numCharsForGraphEncoding, and currGraphBuffSize all set to 0
	(*ppG6WriteIterator) = (G6WriteIterator *) calloc(1, sizeof(G6WriteIterator));

	if ((*ppG6WriteIterator) == NULL)
	{
		ErrorMessage("Unable to allocate memory for G6WriteIterator.\n");
		return NOTOK;
	}

	(*ppG6WriteIterator)->g6Output = NULL;
	(*ppG6WriteIterator)->currGraphBuff = NULL;
	(*ppG6WriteIterator)->columnOffsets = NULL;

	if (pGraph == NULL || pGraph->N <= 0)
	{
		ErrorMessage("[ERROR] Must allocate and initialize graph with an order greater than 0 to use the G6WriteIterator.\n");

		exitCode = freeG6WriteIterator(ppG6WriteIterator);

		if (exitCode != OK)
			ErrorMessage("Unable to free the G6WriteIterator.\n");
	}
	else
		(*ppG6WriteIterator)->currGraph = pGraph;

	return exitCode;
}

bool _isG6WriteIteratorAllocated(G6WriteIterator *pG6WriteIterator)
{
	bool G6WriteIteratorIsAllocated = true;

	if (pG6WriteIterator == NULL || pG6WriteIterator->currGraph == NULL)
		G6WriteIteratorIsAllocated = false;

	return G6WriteIteratorIsAllocated;
}

int getNumGraphsWritten(G6WriteIterator *pG6WriteIterator, int *pNumGraphsRead)
{
	if (pG6WriteIterator == NULL)
	{
		ErrorMessage("G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	(*pNumGraphsRead) = pG6WriteIterator->numGraphsWritten;

	return OK;
}

int getOrderOfGraphToWrite(G6WriteIterator *pG6WriteIterator, int *pGraphOrder)
{
	if (pG6WriteIterator == NULL)
	{
		ErrorMessage("G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	(*pGraphOrder) = pG6WriteIterator->graphOrder;

	return OK;
}

int getPointerToGraphToWrite(G6WriteIterator *pG6WriteIterator, graphP *ppGraph)
{
	if (pG6WriteIterator == NULL)
	{
		ErrorMessage("[ERROR] G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	(*ppGraph) = pG6WriteIterator->currGraph;

	return OK;
}

int getGraphBuff(G6WriteIterator *pG6WriteIterator, char**ppCurrGraphBuff)
{
	if (pG6WriteIterator == NULL)
	{
		ErrorMessage("G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	(*ppCurrGraphBuff) = pG6WriteIterator->currGraphBuff;

	return OK;
}

int beginG6WriteIterationToG6FilePath(G6WriteIterator *pG6WriteIterator, char *g6FilePath)
{
	int exitCode = OK;

	if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
	{
		ErrorMessage("G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	if (g6FilePath == NULL || strlen(g6FilePath) == 0)
	{
		ErrorMessage("g6FilePath is null or has length 0.\n");
		return NOTOK;
	}

	g6FilePath[strcspn(g6FilePath, "\n\r")] = '\0';

	FILE *g6Outfile = fopen(g6FilePath, "w");

	if (g6Outfile == NULL)
	{
		char *messageFormat = "Unable to open \"%.*s\" for writing.\n";
		char messageContents[MAXLINE + 1];
		int charsAvailForStr = (int) (MAXLINE - strlen(messageFormat));
		sprintf(messageContents, messageFormat, charsAvailForStr, g6FilePath);
		ErrorMessage(messageContents);
		return NOTOK;
	}

	exitCode = beginG6WriteIterationToG6FilePointer(pG6WriteIterator, g6Outfile);

	if (exitCode != OK)
		ErrorMessage("Unable to begin write iteration from .g6 file pointer corresponding to given file path.\n");

	return exitCode;
}

int beginG6WriteIterationToG6FilePointer(G6WriteIterator *pG6WriteIterator, FILE *g6Outfile)
{
	int exitCode = OK;

	if (g6Outfile == NULL)
	{
		ErrorMessage(".g6 outfile pointer is NULL.\n");
		return NOTOK;
	}

	pG6WriteIterator->g6Output = sf_New(g6Outfile, NULL);

	exitCode = _beginG6WriteIteration(pG6WriteIterator);

	if (exitCode != OK)
		ErrorMessage("Unable to begin .g6 write iteration from given file pointer.\n");

	return exitCode;
}

int beginG6WriteIterationToG6String(G6WriteIterator *pG6WriteIterator, char **g6OutputStr)
{
	int exitCode = OK;

	if (g6OutputStr == NULL)
	{
		ErrorMessage("No .g6 output string pointer provided.\n");
		return NOTOK;
	}

	if ((*g6OutputStr) == NULL)
	{
		(*g6OutputStr) = (char *) malloc(1 * sizeof(char));
		(*g6OutputStr)[0] = '\0';
	}

	pG6WriteIterator->g6Output = sf_New(NULL, (*g6OutputStr));

	exitCode = _beginG6WriteIteration(pG6WriteIterator);

	if (exitCode != OK)
		ErrorMessage("Unable to begin .g6 write iteration from given file pointer.\n");

	return exitCode;
}

int _beginG6WriteIteration(G6WriteIterator *pG6WriteIterator)
{
	int exitCode = OK;

	char *g6Header = ">>graph6<<";
	if (sf_fputs(g6Header, pG6WriteIterator->g6Output) < 0)
	{
		ErrorMessage("Unable to fputs header to g6Output.\n");
		return NOTOK;
	}

	pG6WriteIterator->graphOrder = pG6WriteIterator->currGraph->N;

	pG6WriteIterator->columnOffsets = (int *) calloc(pG6WriteIterator->graphOrder + 1, sizeof(int));

	if ( pG6WriteIterator->columnOffsets == NULL)
	{
		ErrorMessage("Unable to allocate memory for column offsets.\n");
		return NOTOK;
	}

	_precomputeColumnOffsets(pG6WriteIterator->columnOffsets, pG6WriteIterator->graphOrder);

	pG6WriteIterator->numCharsForGraphOrder = _getNumCharsForGraphOrder(pG6WriteIterator->graphOrder);
	pG6WriteIterator->numCharsForGraphEncoding = _getNumCharsForGraphEncoding(pG6WriteIterator->graphOrder);
	// Must add 3 bytes for newline, possible carriage return, and null terminator
	pG6WriteIterator->currGraphBuffSize = pG6WriteIterator->numCharsForGraphOrder + pG6WriteIterator->numCharsForGraphEncoding + 3;
	pG6WriteIterator->currGraphBuff = (char *) calloc(pG6WriteIterator->currGraphBuffSize, sizeof(char));

	if (pG6WriteIterator->currGraphBuff == NULL)
	{
		ErrorMessage("Unable to allocate memory for currGraphBuff.\n");
		exitCode = NOTOK;
	}

	return exitCode;
}

void _precomputeColumnOffsets(int *columnOffsets, int graphOrder)
{
	if (columnOffsets == NULL)
	{
		ErrorMessage("Must allocate columnOffsets memory before precomputation.\n");
		return;
	}

	columnOffsets[0] = 0;
	columnOffsets[1] = 0;
	for (int i = 2; i <= graphOrder; i++)
		columnOffsets[i] = columnOffsets[i - 1] + (i - 1);
}

int writeGraphUsingG6WriteIterator(G6WriteIterator *pG6WriteIterator)
{
	int exitCode = OK;

	if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
	{
		ErrorMessage("Unable to write graph, as G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	exitCode = _encodeAdjMatAsG6(pG6WriteIterator);

	if (exitCode != OK)
	{
		ErrorMessage("Error converting adjacency matrix to g6 format.\n");
		return exitCode;
	}

	exitCode = _printEncodedGraph(pG6WriteIterator);
	if (exitCode != OK)
		ErrorMessage("Unable to output g6 encoded graph to string-or-file container.\n");

	return exitCode;
}

int _encodeAdjMatAsG6(G6WriteIterator *pG6WriteIterator) {
	int exitCode = OK;

	if (!_isG6WriteIteratorAllocated(pG6WriteIterator))
	{
		ErrorMessage("Unable to encode graph, as G6WriteIterator is not allocated.\n");
		return NOTOK;
	}

	char *g6Encoding = pG6WriteIterator->currGraphBuff;

	if (g6Encoding == NULL)
	{
		ErrorMessage("[ERROR] Graph buffer is not allocated.\n");
		return NOTOK;
	}

	int *columnOffsets = pG6WriteIterator->columnOffsets;
	if (columnOffsets == NULL)
	{
		ErrorMessage("Column offsets array is not allocated.\n");
		return NOTOK;
	}

	graphP pGraph = pG6WriteIterator->currGraph;

	if (pGraph == NULL || pGraph->N == 0)
	{
		ErrorMessage("Graph is not allocated.\n");
		return NOTOK;
	}

	// memset ensures all bits are zero, which means we only need to set the bits
	// that correspond to an edge; this also takes care of padding zeroes for us
	memset(pG6WriteIterator->currGraphBuff, 0, (pG6WriteIterator->currGraphBuffSize) * sizeof(char));

	int graphOrder = pG6WriteIterator->graphOrder;
	int numCharsForGraphOrder = pG6WriteIterator->numCharsForGraphOrder;
	int numCharsForGraphEncoding = pG6WriteIterator->numCharsForGraphEncoding;
	int totalNumCharsForOrderAndGraph = numCharsForGraphOrder + numCharsForGraphEncoding;

	if (graphOrder > 62) {
		g6Encoding[0] = 126;
		// bytes 1 through 3 will be populated with the 18-bit representation of the graph order
		int intermediate = -1;
		for (int i = 0; i < 3; i++) {
			intermediate = graphOrder >> (6 * i);
			g6Encoding[3 - i] = intermediate & 63;
			g6Encoding[3 - i] += 63;
		}
	} else if (graphOrder > 1 && graphOrder < 63) {
		g6Encoding[0] = graphOrder + 63;
	}

	int u = NIL, v = NIL, e = NIL;
	exitCode = _getFirstEdge(pGraph, &e, &u, &v);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to fetch first edge in graph.\n");
		return exitCode;
	}

	int charOffset = 0;
	int bitPositionPower = 0;
	int tempVert = NIL;
	while (u != NIL && v != NIL)
	{
		// The internal graph representation is usually 1-based, but may be 0-based, so
		// one must subtract the index of the first vertex (i.e. result of gp_GetFirstVertex)
		// because the .g6 format is 0-based
		u -=  gp_GetFirstVertex(theGraph);
		v -=  gp_GetFirstVertex(theGraph);

		// The columnOffset machinery assumes that we are traversing the edges represented in 
		// the upper-triangular matrix. Since we are dealing with simple graphs, if (v, u)
		// exists, then (u, v) exists, and so the edge is indicated by a 1 in row = min(u, v)
		// and col = max(u, v) in the upper-triangular adjacency matrix.
		if (v < u)
		{
			tempVert = v;
			v = u;
			u = tempVert;
		}

		// (columnOffsets[v] + u) describes the bit index of the current edge
		// given the column and row in the adjacency matrix representation;
		// the byte is floor((columnOffsets[v] + u) / 6) and the we determine which
		// bit to set in that byte by left-shifting 1 by (5 - ((columnOffsets[v] + u) % 6))
		// (transforming the ((columnOffsets[v] + u) % 6)th bit from the left to the
		// (5 - ((columnOffsets[v] + u) % 6))th bit from the right)
		charOffset = numCharsForGraphOrder + ((columnOffsets[v] + u) / 6);
		bitPositionPower = 5 - ((columnOffsets[v] + u) % 6);

		g6Encoding[charOffset] |= (1u << bitPositionPower);

		exitCode = _getNextEdge(pGraph, &e, &u, &v);

		if (exitCode != OK)
		{
			ErrorMessage("Unable to fetch next edge in graph.\n");
			free(columnOffsets);
			free(g6Encoding);
			return exitCode;
		}
	}

	// Bytes corresponding to graph order have already been modified to
	// correspond to printable ascii character (i.e. by adding 63); must
	// now do the same for bytes corresponding to edge lists
	for (int i = numCharsForGraphOrder; i < totalNumCharsForOrderAndGraph; i++)
		g6Encoding[i] += 63;

	return exitCode;
}

int _getFirstEdge(graphP theGraph, int *e, int *u, int *v)
{
	if (theGraph == NULL) return NOTOK;

	if ((*e) >= gp_EdgeInUseIndexBound(theGraph))
	{
		ErrorMessage("First edge is outside bounds.");
		return NOTOK;
	}

	(*e) = gp_GetFirstEdge(theGraph);

	return _getNextInUseEdge(theGraph, e, u, v);
}

int _getNextEdge(graphP theGraph, int *e, int *u, int *v)
{
	if (theGraph == NULL) return NOTOK;

	(*e) += 2;

	return _getNextInUseEdge(theGraph, e, u, v);
}

int _getNextInUseEdge(graphP theGraph, int *e, int *u, int *v)
{
	int exitCode = OK;
	int EsizeOccupied = gp_EdgeInUseIndexBound(theGraph);

	(*u) = NIL;
	(*v) = NIL;

	if ((*e) < EsizeOccupied)
	{
		while (!gp_EdgeInUse(theGraph, (*e)))
		{
			(*e) += 2;
			if ((*e) >= EsizeOccupied)
				break;
		}
		
		if ((*e) < EsizeOccupied && gp_EdgeInUse(theGraph, (*e)))
		{
			(*u) = gp_GetNeighbor(theGraph, (*e));
			(*v) = gp_GetNeighbor(theGraph, gp_GetTwinArc(theGraph, (*e)));
		}
	}

	return exitCode;
}

int _printEncodedGraph(G6WriteIterator *pG6WriteIterator)
{
	int exitCode = OK;

	if (pG6WriteIterator->g6Output == NULL)
	{
		ErrorMessage("Unable to print to NULL string-or-file container.\n");
		return NOTOK;
	}

	if (pG6WriteIterator->currGraphBuff == NULL || strlen(pG6WriteIterator->currGraphBuff) == 0)
	{
		ErrorMessage("Unable to print; g6 encoding is empty.\n");
		return NOTOK;
	}

	if (sf_fputs(pG6WriteIterator->currGraphBuff, pG6WriteIterator->g6Output) < 0)
	{
		ErrorMessage("Failed to output all characters of g6 encoding.\n");
		exitCode = NOTOK;
	}

	return exitCode;
}

int endG6WriteIteration(G6WriteIterator *pG6WriteIterator)
{
	int exitCode = OK;

	if (pG6WriteIterator != NULL)
	{
		if (pG6WriteIterator->g6Output != NULL)
		{
			exitCode = sf_closeFile(pG6WriteIterator->g6Output);

			if (exitCode != OK)
				ErrorMessage("Unable to close g6Output file pointer.\n");

			sf_Free(&(pG6WriteIterator->g6Output));
		}

		if (pG6WriteIterator->currGraphBuff != NULL)
		{
			free(pG6WriteIterator->currGraphBuff);
			pG6WriteIterator->currGraphBuff = NULL;
		}

		if (pG6WriteIterator->columnOffsets != NULL)
		{
			free((pG6WriteIterator->columnOffsets));
			pG6WriteIterator->columnOffsets = NULL;
		}
	}

	return exitCode;
}

int freeG6WriteIterator(G6WriteIterator **ppG6WriteIterator)
{
	int exitCode = OK;

	if (ppG6WriteIterator != NULL && (*ppG6WriteIterator) != NULL)
	{
		if ((*ppG6WriteIterator)->g6Output != NULL)
			sf_Free(&((*ppG6WriteIterator)->g6Output));

		(*ppG6WriteIterator)->numGraphsWritten = 0;
		(*ppG6WriteIterator)->graphOrder = 0;

		if ((*ppG6WriteIterator)->currGraphBuff != NULL)
		{
			free((*ppG6WriteIterator)->currGraphBuff);
			(*ppG6WriteIterator)->currGraphBuff = NULL;
		}

		if ((*ppG6WriteIterator)->columnOffsets != NULL)
		{
			free(((*ppG6WriteIterator)->columnOffsets));
			(*ppG6WriteIterator)->columnOffsets = NULL;
		}

		// N.B. The G6WriteIterator doesn't "own" the graph, so we don't free it.
		(*ppG6WriteIterator)->currGraph = NULL;

		free((*ppG6WriteIterator));
		(*ppG6WriteIterator) = NULL;
	}

	return exitCode;
}

int _WriteGraphToG6FilePath(graphP pGraph, char *g6OutputFilename)
{
	int exitCode = OK;

	G6WriteIterator *pG6WriteIterator = NULL;

	exitCode = allocateG6WriteIterator(&pG6WriteIterator, pGraph);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to allocate G6WriteIterator.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = beginG6WriteIterationToG6FilePath(pG6WriteIterator, g6OutputFilename);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to begin G6 write iteration.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = writeGraphUsingG6WriteIterator(pG6WriteIterator);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to write graph using G6WriteIterator.\n");
	}
	else
	{
		int endG6WriteIterationCode = endG6WriteIteration(pG6WriteIterator);

		if (endG6WriteIterationCode != OK)
		{
			ErrorMessage("Unable to end G6 write iteration.\n");
			exitCode = endG6WriteIterationCode;
		}
	}

	int freeG6WriteIteratorCode = freeG6WriteIterator(&pG6WriteIterator);
	
	if (freeG6WriteIteratorCode != OK)
	{
		ErrorMessage("Unable to free G6Writer.\n");
		exitCode = freeG6WriteIteratorCode;
	}

	return exitCode;
}

int _WriteGraphToG6FilePointer(graphP pGraph, FILE *g6Outfile)
{
	int exitCode = OK;

	G6WriteIterator *pG6WriteIterator = NULL;

	exitCode = allocateG6WriteIterator(&pG6WriteIterator, pGraph);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to allocate G6WriteIterator.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = beginG6WriteIterationToG6FilePointer(pG6WriteIterator, g6Outfile);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to begin G6 write iteration.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = writeGraphUsingG6WriteIterator(pG6WriteIterator);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to write graph using G6WriteIterator.\n");
	}
	else
	{
		int endG6WriteIterationCode = endG6WriteIteration(pG6WriteIterator);

		if (endG6WriteIterationCode != OK)
		{
			ErrorMessage("Unable to end G6 write iteration.\n");
			exitCode = endG6WriteIterationCode;
		}
	}

	int freeG6WriteIteratorCode = freeG6WriteIterator(&pG6WriteIterator);
	
	if (freeG6WriteIteratorCode != OK)
	{
		ErrorMessage("Unable to free G6Writer.\n");
		exitCode = freeG6WriteIteratorCode;
	}

	return exitCode;
}

int _WriteGraphToG6String(graphP pGraph, char **g6OutputStr)
{
	int exitCode = OK;

	G6WriteIterator *pG6WriteIterator = NULL;

	exitCode = allocateG6WriteIterator(&pG6WriteIterator, pGraph);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to allocate G6WriteIterator.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = beginG6WriteIterationToG6String(pG6WriteIterator, g6OutputStr);

	if (exitCode != OK)
	{
		ErrorMessage("Unable to begin G6 write iteration.\n");
		freeG6WriteIterator(&pG6WriteIterator);
		return exitCode;
	}

	exitCode = writeGraphUsingG6WriteIterator(pG6WriteIterator);

	if (exitCode != OK)
		ErrorMessage("Unable to write graph using G6WriteIterator.\n");
	else
	{
		(*g6OutputStr) = sf_takeTheStr(pG6WriteIterator->g6Output);

		int endG6WriteIterationCode = endG6WriteIteration(pG6WriteIterator);

		if (endG6WriteIterationCode != OK)
		{
			ErrorMessage("Unable to end G6 write iteration.\n");
			exitCode = endG6WriteIterationCode;
		}
	}

	int freeG6WriteIteratorCode = freeG6WriteIterator(&pG6WriteIterator);
	
	if (freeG6WriteIteratorCode != OK)
	{
		ErrorMessage("Unable to free G6Writer.\n");
		exitCode = freeG6WriteIteratorCode;
	}

	return exitCode;
}
