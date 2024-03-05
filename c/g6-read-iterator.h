/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#ifndef G6_READ_ITERATOR
#define G6_READ_ITERATOR

#include <stdio.h>
#include <stdbool.h>

#include "graph.h"

typedef struct {
	FILE *g6Infile;
	bool doIOwnFilePointer;
	int numGraphsRead;

	int graphOrder;
	int numCharsForGraphOrder;
	int numCharsForGraphEncoding;
	int currGraphBuffSize;
	char *currGraphBuff;

	graphP currGraph;
} G6ReadIterator;

int allocateG6ReadIterator(G6ReadIterator **, graphP);
bool _isG6ReadIteratorAllocated(G6ReadIterator *pG6ReadIterator);

int getNumGraphsRead(G6ReadIterator *, int *);
int getOrderOfGraphToRead(G6ReadIterator *, int *);
int getPointerToGraphReadIn(G6ReadIterator *, graphP *);

int beginG6ReadIteration(G6ReadIterator *, char *);
int beginG6ReadIterationFromFilePointer(G6ReadIterator *, FILE *);
int _processAndCheckHeader(FILE *);
bool _firstCharIsValid(char, const int);
int _getGraphOrder(FILE *, int *);

int readGraphUsingG6ReadIterator(G6ReadIterator *);
int _checkGraphOrder(char *, int);
int _validateGraphEncoding(char *, const int, const int);
int _decodeGraph(char *, const int, const int, graphP);

int endG6ReadIteration(G6ReadIterator *);

int freeG6ReadIterator(G6ReadIterator **);

int _ReadGraphFromG6String(graphP, char *);
int _ReadGraphFromG6File(graphP, char *);
int _ReadGraphFromG6FilePointer(graphP pGraphToRead, FILE *g6Infile);

#endif /* G6_READ_ITERATOR */
