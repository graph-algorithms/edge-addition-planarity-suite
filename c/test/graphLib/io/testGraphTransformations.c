/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runGraphTransformationTests(void);
int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag);

int runGraphTransformationTests(void)
{
    int retVal = OK;

    /*
        GRAPH TRANSFORMATION TESTS
    */
    gp_Message("Starting G6-Related Graph Transformation Tests");

    //  TRANSFORM TO ADJACENCY LIST

    // runGraphTransformationTest by reading file contents into string
    if (runGraphTransformationTest("-a", "nauty_example.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming nauty_example.g6 file contents as string "
                        "to adjacency list failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-a", "nauty_example.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming nauty_example.g6 using file pointer to "
                        "adjacency list failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file into string
    if (runGraphTransformationTest("-a", "N5-all.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming first graph in N5-all.g6 (read as "
                        "string) to adjacency list failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file pointer
    if (runGraphTransformationTest("-a", "N5-all.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming first graph in N5-all.g6 (read from file "
                        "pointer) to adjacency list failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading file contents corresponding to dense graph into string
    if (runGraphTransformationTest("-a", "K10.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming K10.g6 file contents as string to "
                        "adjacency list failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading dense graph from file
    if (runGraphTransformationTest("-a", "K10.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming K10.g6 using file pointer to adjacency "
                        "list failed.");
        retVal = NOTOK;
    }

    //  TRANSFORM TO ADJACENCY MATRIX

    // runGraphTransformationTest by reading file contents into string
    if (runGraphTransformationTest("-m", "nauty_example.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming nauty_example.g6 file contents as string "
                        "to adjacency matrix failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-m", "nauty_example.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming nauty_example.g6 using file pointer to "
                        "adjacency matrix failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file into string
    if (runGraphTransformationTest("-m", "N5-all.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming first graph in N5-all.g6 (read as "
                        "string) to adjacency matrix failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file pointer
    if (runGraphTransformationTest("-m", "N5-all.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming first graph in N5-all.g6 (read from file "
                        "pointer) to adjacency matrix failed.");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading file contents corresponding to dense graph into string
    if (runGraphTransformationTest("-m", "K10.g6", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming K10.g6 file contents as string to "
                        "adjacency matrix failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading dense graph from file
    if (runGraphTransformationTest("-m", "K10.g6", FALSE) != OK)
    {
        gp_ErrorMessage("Transforming K10.g6 using file pointer to adjacency "
                        "matrix failed.");
        retVal = NOTOK;
    }

    //  TRANSFORM TO .G6

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-g", "nauty_example.g6.0-based.AdjList.out.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming nauty_example.g6.0-based.AdjList.out.txt "
                        "using file pointer to .g6 failed.");
        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-g", "K10.g6.0-based.AdjList.out.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Transforming K10.g6.0-based.AdjList.out.txt using "
                        "file pointer to .g6 failed.");
        retVal = NOTOK;
    }

    if (retVal == OK)
        gp_Message("Successfully finished G6-related Graph Transformation Tests.\n");
    else
        gp_Message("Error detected in G6-related Graph Transformation Tests.\n");

    return retVal;
}

int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag)
{
    int Result = OK;

    char *inputString = NULL;
    char transformationCode = '\0';

    // runGraphTransformationTest will not test performing an algorithm on a given
    // input graph; it will only support "-(gsdam)"
    if (command == NULL || strlen(command) < 2)
    {
        gp_ErrorMessage("runGraphTransformationTest only supports -(gsdam).");
        return NOTOK;
    }
    else if (strlen(command) == 2)
        transformationCode = command[1];

    // SpecificGraph() can invoke gp_Read() if the graph is to be read from a file, or it can invoke
    // gp_ReadFromString() if the inputInMemFlag is set.
    if (inputInMemFlag)
    {
        inputString = ReadTextFileIntoString(infileName);
        if (inputString == NULL)
        {
            gp_ErrorMessage("Failed to read input file into string.");
            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
        // We need to capture whether output is 0- or 1-based to construct the name of the file to compare actualOutput with
        int zeroBasedOutputFlag = 0;
        char *actualOutput = NULL;
        // We want to handle the test being run when we read from an input file or read from a string,
        // so pass both infileName and inputString.
        // We want to output to string, so we pass in the address of the actualOutput string.
        Result = TransformGraph(command, infileName, inputString, &zeroBasedOutputFlag, NULL, &actualOutput);

        if (Result != OK || actualOutput == NULL)
        {
            gp_ErrorMessage("Failed to perform transformation.");
            Result = NOTOK;
        }
        else
        {
            char *expectedOutfileName = NULL;
            // Final arg is baseFlag, which is dependent on whether the FLAGS_ZEROBASEDIO is set in a graph's graphFlags
            Result = ConstructTransformationExpectedResultFileName(infileName, &expectedOutfileName, transformationCode, zeroBasedOutputFlag ? 0 : 1);

            if (Result != OK || expectedOutfileName == NULL)
            {
                gp_ErrorMessage("Unable to construct output file name for "
                                "expected transformation output.");
                Result = NOTOK;
            }
            else
            {
                Result = TextFileMatchesString(expectedOutfileName, actualOutput);

                if (Result == TRUE)
                {
                    gp_Message("For the transformation %s on \"%.*s\", "
                               "actual output matched expected output file.",
                               command, FILENAME_MAX, infileName);
                    Result = OK;
                }
                else
                {
                    gp_ErrorMessage("For the transformation %s on \"%.*s\", "
                                    "actual output did not match expected "
                                    "output file.",
                                    command, FILENAME_MAX, infileName);
                    Result = NOTOK;
                }

                if (expectedOutfileName != NULL)
                {
                    free(expectedOutfileName);
                    expectedOutfileName = NULL;
                }

                if (actualOutput != NULL)
                {
                    free(actualOutput);
                    actualOutput = NULL;
                }
            }
        }
    }

    if (inputString != NULL)
    {
        free(inputString);
        inputString = NULL;
    }

    return Result;
}
