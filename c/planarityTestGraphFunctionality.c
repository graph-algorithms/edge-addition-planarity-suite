/*
Copyright (c) 1997-2024, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

int transformFile(char *infileName, int outputFormat, char *outfileName);
int transformString(char *inputStr, int outputFormat, char *outfileName);

/****************************************************************************
 TestGraphFunctionality()
 infileName - name of file to read, or NULL to cause the program to prompt the user for a filename
 outputFormat - output format; currently only supports WRITE_ADJLIST
 outfileName - name of primary output file, or NULL to construct an output filename based on the input
 ****************************************************************************/

int TestGraphFunctionality(char *commandString, char *infileName, char *inputStr, char *outfileName)
{
    int Result = OK;
    int outputFormat = -1;

    if (commandString[0] == '-')
	{
		if (commandString[1] == 't')
		{
			if (commandString[2] == 'a')
			{
				outputFormat = WRITE_ADJLIST;
			}
			// TODO: handle .g6 and adjacency matrix output formats
			else
			{
				ErrorMessage("Invalid argument; currently, only -ta is allowed.\n");
				return -1;
			}

			if (inputStr)
				Result = transformString(inputStr, outputFormat, outfileName);
			else
            	Result = transformFile(infileName, outputFormat, outfileName);
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

    return Result;
}

int transformFile(char *infileName, int outputFormat, char *outfileName)
{
    int Result = OK;
    graphP theGraph;

    // Create the graph and, if needed, attach the correct algorithm to it
    theGraph = gp_New();

    // Get the filename of the graph to test
    if (infileName == NULL)
    {
        if ((infileName = ConstructInputFilename(infileName)) == NULL)
	        return NOTOK;
    }

	Result = gp_Read(theGraph, infileName);

    if (Result == OK) {
        Result = gp_Write(theGraph, outfileName, outputFormat);
    }
    
    return Result;
}

int transformString(char *inputStr, int outputFormat, char *outfileName)
{
    int Result = OK;
    graphP theGraph;

    // Create the graph and, if needed, attach the correct algorithm to it
    theGraph = gp_New();

    // Get the filename of the graph to test
    if (inputStr == NULL || strlen(inputStr) == 0)
    {
		ErrorMessage("Input string is null or empty.\n");
	    return NOTOK;
    }

	Result = gp_ReadFromString(theGraph, inputStr);

    if (Result == OK) {
        Result = gp_Write(theGraph, outfileName, outputFormat);
    }
    
    return Result;
}