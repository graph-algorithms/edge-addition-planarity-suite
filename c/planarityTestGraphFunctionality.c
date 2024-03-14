/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "graph.h"

int transformFile(graphP theGraph, char *infileName);
int transformString(graphP theGraph, char *inputStr);

/****************************************************************************
 TestGraphFunctionality()
 commandString - command to run, e.g. `-ta` to transform graph to adjacency list format
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
	graphP theGraph;

	// Create the graph and, if needed, attach the correct algorithm to it
	theGraph = gp_New();

	int outputFormat = -1;

	if (commandString[0] == '-')
	{
		if (commandString[1] == 't')
		{
			if (commandString[2] == 'a')
				outputFormat = WRITE_ADJLIST;
			// TODO: handle .g6 and adjacency matrix output formats
			// else if (commandString[2] == 'm')
			// 	outputFormat = WRITE_ADJMATRIX;
			else
			{
				ErrorMessage("Invalid argument; currently, only -ta is allowed.\n");
				return -1;
			}

			if (inputStr)
				Result = transformString(theGraph, inputStr);
			else
				Result = transformFile(theGraph, infileName);

			if (Result != OK) {
				ErrorMessage("Unable to transform file.\n");
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
		// TODO: add elif for algorithm command handling
		else
		{
			// TODO: update error message to capture C|-t(gam)
			ErrorMessage("Invalid argument; currently, only -ta is allowed.\n");
			return -1;
		}
	}
	else
	{
		ErrorMessage("Invalid argument; must start with '-'.\n");
		return -1;
	}

	gp_Free(&theGraph);

	return Result;
}

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
