/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "graph.h"
#include "platformTime.h"
#include "g6-read-iterator.h"
#include "strOrFile.h"

typedef struct {
	double duration;
	int numGraphsRead;
	int numOK;
	int numNONEMBEDDABLE;
	int errorFlag;
} testAllStats;

typedef testAllStats * testAllStatsP;

int transformFile(graphP theGraph, char *infileName);
int transformString(graphP theGraph, char *inputStr);

int testAllGraphs(graphP theGraph, char command, FILE *infile, testAllStatsP stats);
int outputTestAllGraphsResults(char command, testAllStatsP stats, char *infileName, char *outfileName, char **outputStr);

int _getNumCharsToReprInt(int theNum)
{
	int numCharsRequired = 1;

	while(theNum /= 10)
		numCharsRequired++;

	return numCharsRequired;
}

/****************************************************************************
 TestGraphFunctionality()
 commandString - command to run; e.g. `-t(gam)` to transform graph to .g6, adjacency list, or
 adjacency matrix format, or `-(pdo234)` to perform the corresponding algorithm on each graph in
 a .g6 file
 infileName - name of file to read, or NULL to cause the program to prompt the user for a filename
 inputStr - string containing input graph, or NULL to cause the program to fall back on reading from file
 outputBase - pointer to the flag set for whether output is 0- or 1-based
 outputFormat - output format; currently only supports WRITE_ADJLIST
 outfileName - name of primary output file, or NULL to construct an output filename based on the input
 outputStr - pointer to string which we wish to use to store the transformation output
 ****************************************************************************/
int TestGraphFunctionality(char *commandString, char *infileName, char *inputStr, int *outputBase, char *outfileName, char **outputStr)
{
	int Result = OK;
	platform_time start, end;

	int charsAvailForFilename = 0;
	char *messageFormat = NULL;
	char messageContents[MAXLINE + 1];
	messageContents[MAXLINE] = '\0';

	graphP theGraph;

	// Create the graph and, if needed, attach the correct algorithm to it
	theGraph = gp_New();

	int outputFormat = -1;

	if (commandString[0] == '-')
	{
		if (commandString[1] == 't')
		{
			if (commandString[2] == 'g')
				outputFormat = WRITE_G6;
			else if (commandString[2] == 'a')
				outputFormat = WRITE_ADJLIST;
			else if (commandString[2] == 'm')
				outputFormat = WRITE_ADJMATRIX;
			else
			{
				ErrorMessage("Invalid argument; currently, only -t(gam) is allowed.\n");
				return NOTOK;
			}

			if (inputStr)
				Result = transformString(theGraph, inputStr);
			else
				Result = transformFile(theGraph, infileName);

			if (Result != OK) {
				ErrorMessage("Unable to transform input graph.\n");
			}
			else
			{
				// Want to know whether the output is 0- or 1-based; will always be 
				// 0-based for transformations of .g6 input
				if (outputBase != NULL)
					(*outputBase) = (theGraph->internalFlags & FLAGS_ZEROBASEDIO) ? 1 : 0;

				if (outputStr != NULL)
					Result = gp_WriteToString(theGraph, outputStr, outputFormat);
				else
					Result = gp_Write(theGraph, outfileName, outputFormat);
				
				if (Result != OK)
					ErrorMessage("Unable to write graph.\n");
			}
		}
		else if (strchr(GetAlgorithmChoices(), commandString[1]))
		{
			if (inputStr != NULL)
			{
				ErrorMessage("TestGraphFunctionality only supports applying chosen algorithm to graphs read from file at this time.\n");
				Result = NOTOK;
			}
			else
			{
				if (infileName == NULL)
				{
					ErrorMessage("No input file provided.\n");
					Result = NOTOK;
				}
				else
				{
					messageFormat = "Start testing all graphs in \"%.*s\".\n";
					charsAvailForFilename = (int) (MAXLINE - strlen(messageFormat));
					sprintf(messageContents, messageFormat, charsAvailForFilename, infileName);
					Message(messageContents);
					
					// Start the timer
					platform_GetTime(start);

					FILE *infile = fopen(infileName, "r");
					if (infile == NULL)
					{
						charsAvailForFilename = (int) (MAXLINE - strlen(infileName));
						messageFormat = "Unable to open file \"%.*s\" for input.\n";
						sprintf(messageContents, messageFormat, charsAvailForFilename, infileName);
						ErrorMessage(messageContents);

						gp_Free(&theGraph);

						return NOTOK;
					}

					testAllStats stats;
					memset(&stats, 0, sizeof(testAllStats));

					char command = commandString[1];
					Result = testAllGraphs(theGraph, command, infile, &stats);

					// Stop the timer
					platform_GetTime(end);
					stats.duration = platform_GetDuration(start, end);

					if (Result != OK && Result != NONEMBEDDABLE)
					{
						messageFormat = "\nEncountered error while running command '%c' on all graphs in \"%.*s\".\n";
						charsAvailForFilename = (int) (MAXLINE - strlen(messageFormat));
						sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
						ErrorMessage(messageContents);
					}
					else
					{
						sprintf(messageContents, "\nDone testing all graphs (%.3lf seconds).\n", stats.duration);
						Message(messageContents);
					}

					if (outputTestAllGraphsResults(command, &stats, infileName, outfileName, outputStr) != OK)
					{
						messageFormat = "Error outputting results running command '%c' on all graphs in \"%.*s\".\n";
						charsAvailForFilename = (int) (MAXLINE - strlen(messageFormat));
						sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
						ErrorMessage(messageContents);
						Result = NOTOK;
					}
				}
			}
		}
		else
		{
			ErrorMessage("Invalid argument; only -(pdo234)|-t(gam) is allowed.\n");
			Result = NOTOK;
		}
	}
	else
	{
		ErrorMessage("Invalid argument; must start with '-'.\n");
		Result = NOTOK;
	}

	gp_Free(&theGraph);
	return Result;
}

/*
	TRANSFORM GRAPH
*/

int transformFile(graphP theGraph, char *infileName)
{
	if (infileName == NULL)
	{
		if ((infileName = ConstructInputFilename(infileName)) == NULL)
			return NOTOK;
	}

	return gp_Read(theGraph, infileName);
}

int transformString(graphP theGraph, char *inputStr)
{
	if (inputStr == NULL || strlen(inputStr) == 0)
	{
		ErrorMessage("Input string is null or empty.\n");
		return NOTOK;
	}
	
	return gp_ReadFromString(theGraph, inputStr);
}

/*
	TEST ALL GRAPHS IN .G6
*/

int testAllGraphs(graphP theGraph, char command, FILE *infile, testAllStatsP stats)
{
	int Result = OK;

	char *messageFormat = NULL;
	char messageContents[MAXLINE + 1];
	messageContents[MAXLINE] = '\0';

	graphP copyOfOrigGraph = NULL;
	int embedFlags = GetEmbedFlags(command);
	int numGraphsRead = 0, numOK = 0, numNONEMBEDDABLE = 0, errorFlag = FALSE;

	G6ReadIterator *pG6ReadIterator = NULL;
	Result = allocateG6ReadIterator(&pG6ReadIterator, theGraph);

	if (Result != OK)
	{
		ErrorMessage("Unable to allocate G6ReadIterator.\n");
		stats->errorFlag = TRUE;
		return Result;
	}

	Result = beginG6ReadIterationFromG6FilePointer(pG6ReadIterator, infile);

	if (Result != OK)
	{
		ErrorMessage("Unable to begin .g6 read iteration.\n");
		freeG6ReadIterator(&pG6ReadIterator);
		stats->errorFlag = TRUE;
		return Result;
	}

	int graphOrder = pG6ReadIterator->graphOrder;
	// We have to set the maximum arc capacity (i.e. (N * (N - 1))) because some of the test files
	// can contain complete graphs, and the graph drawing, K_{3, 3} search, and K_4 search extensions
	// don't support expanding the arc capacity after being attached.
	if (strchr("d34", command) != NULL)
	{
		Result = gp_EnsureArcCapacity(pG6ReadIterator->currGraph, (graphOrder * (graphOrder - 1)));
		if (Result != OK)
		{
			ErrorMessage("Unable to maximize arc capacity of G6ReadIterator's graph struct.\n");
			freeG6ReadIterator(&pG6ReadIterator);
			stats->errorFlag = TRUE;
			return Result;
		}
	}
	
	AttachAlgorithm(pG6ReadIterator->currGraph, command);

	copyOfOrigGraph = gp_New();
	if (copyOfOrigGraph == NULL)
	{
		ErrorMessage("Unable to allocate graph to store copy of original graph before embedding.\n");
		stats->errorFlag = TRUE;
		return NOTOK;
	}

	Result = gp_InitGraph(copyOfOrigGraph, graphOrder);
	if (Result != OK)
	{
		ErrorMessage("Unable to initialize graph datastructure to store copy of original graph before embedding.\n");
		gp_Free(&copyOfOrigGraph);
		freeG6ReadIterator(&pG6ReadIterator);
		stats->errorFlag = TRUE;
		return Result;
	}

	if (strchr("d34", command) != NULL)
	{
		Result = gp_EnsureArcCapacity(copyOfOrigGraph, (graphOrder * (graphOrder - 1)));
		if (Result != OK)
		{
			ErrorMessage("Unable to maximize arc capacity of graph struct to contain copy of original graph.\n");
			freeG6ReadIterator(&pG6ReadIterator);
			stats->errorFlag = TRUE;
			return Result;
		}
	}


	while (true)
	{
		Result = readGraphUsingG6ReadIterator(pG6ReadIterator);

		if (Result != OK)
		{
			messageFormat = "Unable to read graph on line %d from .g6 read iterator.\n";
			sprintf(messageContents, messageFormat, pG6ReadIterator->numGraphsRead + 1);
			ErrorMessage(messageContents);
			errorFlag = TRUE;
			break;
		}

		if (pG6ReadIterator->currGraph == NULL)
			break;
		
		gp_CopyGraph(copyOfOrigGraph, pG6ReadIterator->currGraph);

		Result = gp_Embed(pG6ReadIterator->currGraph, embedFlags);

		if (gp_TestEmbedResultIntegrity(pG6ReadIterator->currGraph, copyOfOrigGraph, Result) != Result)
			Result = NOTOK;
		
		if (Result == OK)
			numOK++;
		else if (Result == NONEMBEDDABLE)
			numNONEMBEDDABLE++;
		else
		{
			messageFormat = "Error applying algorithm '%c' to graph on line %d.\n";
			sprintf(messageContents, messageFormat, command, pG6ReadIterator->numGraphsRead + 1);
			ErrorMessage(messageContents);
			errorFlag = TRUE;
			break;
		}

		gp_ReinitializeGraph(copyOfOrigGraph);
	}

	stats->numGraphsRead = pG6ReadIterator->numGraphsRead;
	stats->numOK = numOK;
	stats->numNONEMBEDDABLE = numNONEMBEDDABLE;
	stats->errorFlag = errorFlag;

	if (endG6ReadIteration(pG6ReadIterator) != OK)
		ErrorMessage("Unable to end G6ReadIterator.\n");

	if (freeG6ReadIterator(&pG6ReadIterator) != OK)
		ErrorMessage("Unable to free G6ReadIterator.\n");
	
	return Result;
}

int outputTestAllGraphsResults(char command, testAllStatsP stats, char * infileName, char *outfileName, char **outputStr)
{
	int Result = OK;

	int charsAvailForFilename = 0;
	char *messageFormat = NULL;
	char messageContents[MAXLINE + 1];

	char *finalSlash = strrchr(infileName, FILE_DELIMITER);
	char *infileBasename = finalSlash ? (finalSlash + 1) : infileName;

	char *headerFormat = "FILENAME=\"%s\" DURATION=\"%.3lf\"\n";
	char *headerStr = (char *) malloc(
										(
											strlen(headerFormat) +
											strlen(infileBasename) +
											strlen("-1.7976931348623158e+308") + // -DBL_MAX from float.h
											3
										) * sizeof(char));
	if (headerStr == NULL)
	{
		ErrorMessage("Unable allocate memory for output file header.\n");
		return NOTOK;
	}

	sprintf(headerStr, headerFormat, infileBasename, stats->duration);

	char *resultsStr = (char *) malloc(
										(
											3 + _getNumCharsToReprInt(stats->numGraphsRead) +
											1 + _getNumCharsToReprInt(stats->numOK) +
											1 + _getNumCharsToReprInt(stats->numNONEMBEDDABLE)+
											1 + 8 + // either ERROR or SUCCESS, so the longer of which is 7 + 1 chars
											3
										) * sizeof(char));
	if (resultsStr == NULL)
	{
		ErrorMessage("Unable allocate memory for results string.\n");

		free(headerStr);
		headerStr = NULL;

		return NOTOK;
	}

	sprintf(resultsStr, "-%c %d %d %d %s\n",
						command, stats->numGraphsRead, stats->numOK, stats->numNONEMBEDDABLE, stats->errorFlag ? "ERROR" : "SUCCESS");

	strOrFileP testOutput = NULL;

	if (outfileName != NULL)
	{
		FILE *outputFileP = fopen(outfileName, "w");
		if (outputFileP == NULL)
		{
			charsAvailForFilename = (int) (MAXLINE - strlen(outfileName));
			messageFormat = "Unable to open file \"%.*s\" for output.\n";
			sprintf(messageContents, messageFormat, charsAvailForFilename, outfileName);
			ErrorMessage(messageContents);
		}
		else
		{
			testOutput = sf_New(outputFileP, NULL);
			if (testOutput == NULL)
			{
				fclose(outputFileP);
				outputFileP = NULL;
				// TODO: (#56) What happens to file that we opened for write, then immediately close? We won't be able to remove
				// because testOutput failed to initialize
			}
		}
	}
	else
	{
		if (outputStr == NULL)
		{
			ErrorMessage("Both outfileName and pointer to outputStr are NULL.\n");
		}
		else
		{
			if ((*outputStr) != NULL)
				ErrorMessage("Expected memory to which outputStr points to be NULL.\n");
			else
			{
				(*outputStr) = (char *) malloc(1 * sizeof(char));
		
				if ((*outputStr) == NULL)
				{
					ErrorMessage("Unable to allocate memory for outputStr.\n");
				}
				else
				{
					(*outputStr)[0] = '\0';
					testOutput = sf_New(NULL, (*outputStr));
				}
			}
		}
	}

	if (testOutput == NULL)
	{
		ErrorMessage("Unable to set up string-or-file container for test output.\n");

		free(headerStr);
		headerStr = NULL;

		free(resultsStr);
		resultsStr = NULL;

		if (outputStr != NULL && (*outputStr) != NULL)
		{
			free((*outputStr));
			(*outputStr) = NULL;
		}

		return NOTOK;
	}

	if (sf_fputs(headerStr, testOutput) < 0)
	{
		ErrorMessage("Unable to output headerStr to testOutput.\n");
		Result = NOTOK;
	}

	if (Result == OK)
	{
		if (sf_fputs(resultsStr, testOutput) < 0)
		{
			ErrorMessage("Unable to output resultsStr to testOutput.\n");
			Result = NOTOK;
		}
	}

	if (Result == OK)
	{
		if (outputStr != NULL)
			(*outputStr) = sf_takeTheStr(testOutput);
	}

	free(headerStr);
	headerStr = NULL;

	free(resultsStr);
	resultsStr = NULL;

	if (sf_closeFile(testOutput) != OK)
	{
		charsAvailForFilename = (int) (MAXLINE - strlen(outfileName));
		messageFormat = "Unable to close output file \"%.*s\".\n";
		sprintf(messageContents, messageFormat, charsAvailForFilename, outfileName);
		ErrorMessage(messageContents);
		Result = NOTOK;
	}

	sf_Free(&testOutput);

	return Result;
}
