/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runSpecificGraphTests(void);

/* Private functions */
static int runSpecificGraphTest(char const *command, char const *infileName, int inputInMemFlag);
// TODO: There's no corresponding test for checking the output of a planar embedding, the ascii rendition
static int runDrawPlanarNonplanarWriteTest(void);

int runSpecificGraphTests(void)
{
    int retVal = OK;

#ifdef USE_1BASEDARRAYS
    gp_Message("\n\tStarting 1-based Array Index Tests\n");

    if (runSpecificGraphTest("-p", "maxPlanar5.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Planarity test on maxPlanar5.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "maxPlanar5.txt", FALSE) != OK)
    {
        gp_ErrorMessage("Graph drawing test maxPlanar5.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "drawExample.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Graph drawing on drawExample.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-p", "Petersen.txt", FALSE) != OK)
    {
        gp_ErrorMessage("Planarity test on Petersen.txt failed.");
        retVal = NOTOK;
    }

    if (runDrawPlanarNonplanarWriteTest() != OK)
    {
        gp_ErrorMessage("DrawPlanar non-planar write test on Petersen.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-o", "Petersen.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Outerplanarity test on Petersen.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-2", "Petersen.txt", FALSE) != OK)
    {
        gp_ErrorMessage("K_{2,3} search on Petersen.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-3", "Petersen.txt", TRUE) != OK)
    {
        gp_ErrorMessage("K_{3,3} search on Petersen.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-4", "Petersen.txt", FALSE) != OK)
    {
        gp_ErrorMessage("K_4 search on Petersen.txt failed.");
        retVal = NOTOK;
    }

    gp_Message("\tFinished 1-based Array Index Tests.\n");
#endif

    if (runSpecificGraphTest("-p", "maxPlanar5.0-based.txt", FALSE) != OK)
    {
        gp_ErrorMessage("Planarity test on maxPlanar5.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "maxPlanar5.0-based.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Graph drawing test maxPlanar5.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "drawExample.0-based.txt", FALSE) != OK)
    {
        gp_ErrorMessage("Graph drawing on drawExample.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-p", "Petersen.0-based.txt", TRUE) != OK)
    {
        gp_ErrorMessage("Planarity test on Petersen.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-o", "Petersen.0-based.txt", FALSE) != OK)
    {
        gp_ErrorMessage("Outerplanarity test on Petersen.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-2", "Petersen.0-based.txt", TRUE) != OK)
    {
        gp_ErrorMessage("K_{2,3} search on Petersen.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-3", "Petersen.0-based.txt", FALSE) != OK)
    {
        gp_ErrorMessage("K_{3,3} search on Petersen.0-based.txt failed.");
        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-4", "Petersen.0-based.txt", TRUE) != OK)
    {
        gp_ErrorMessage("K_4 search on Petersen.0-based.txt failed.");
        retVal = NOTOK;
    }

    return retVal;
}

int runSpecificGraphTest(char const *commandString, char const *infileName, int inputInMemFlag)
{
    int Result = OK;

    char *inputString = NULL, *actualOutput = NULL, *actualOutput2 = NULL;
    char const *expectedPrimaryResultFileName = "";

    char command = '\0', modifier = '\0';

    if (GetCommandAndOptionalModifier(commandString, &command, &modifier) != OK)
    {
        gp_ErrorMessage("Unable to extract command (or optional modifier) from "
                        "command string.");
        return NOTOK;
    }

    // The algorithm, indicated by algorithmCode, operating on 'infileName' is expected to produce
    // an output that is stored in the file named 'expectedResultFileName' (return string not owned)
    expectedPrimaryResultFileName = ConstructPrimaryOutputFileName(infileName, NULL, command);

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
        // Perform the indicated algorithm on the graph in the input file or string.
        Result = SpecificGraph(commandString,
                               infileName, NULL, NULL,
                               inputString, &actualOutput, &actualOutput2);
    }

    if (Result != OK && Result != NONEMBEDDABLE)
    {
        gp_ErrorMessage("Test failed (graph processor returned failure "
                        "result).");
        Result = NOTOK;
    }
    else
    {
        // Test that the primary actual output matches the primary expected output
        if (TextFileMatchesString(expectedPrimaryResultFileName, actualOutput) == TRUE)
            gp_Message("Test succeeded (result equal to exemplar).");
        else
        {
            gp_ErrorMessage("Test failed (result not equal to exemplar).");
            Result = NOTOK;
        }
    }

    // Test that the secondary actual output matches the secondary expected output
    if (command == 'd' && (Result == OK || Result == NONEMBEDDABLE))
    {
        char *expectedSecondaryResultFileName = (char *)malloc(strlen(expectedPrimaryResultFileName) + strlen(".render.txt") + 1);

        if (expectedSecondaryResultFileName == NULL)
        {
            gp_ErrorMessage("Unable to allocate memory for expected secondary "
                            "output file name.");
            Result = NOTOK;
        }
        else
        {
            sprintf(expectedSecondaryResultFileName, "%s%s", expectedPrimaryResultFileName, ".render.txt");

            if (TextFileMatchesString(expectedSecondaryResultFileName, actualOutput2) == TRUE)
                gp_Message("Test succeeded (secondary result equal to "
                           "exemplar).");
            else
            {
                gp_ErrorMessage("Test failed (secondary result not equal to "
                                "exemplar).");
                Result = NOTOK;
            }

            if (expectedSecondaryResultFileName != NULL)
            {
                free(expectedSecondaryResultFileName);
                expectedSecondaryResultFileName = NULL;
            }
        }
    }

    gp_Message(" ");

    if (inputString != NULL)
    {
        free(inputString);
        inputString = NULL;
    }

    if (actualOutput != NULL)
    {
        free(actualOutput);
        actualOutput = NULL;
    }

    if (actualOutput2 != NULL)
    {
        free(actualOutput2);
        actualOutput2 = NULL;
    }

    // NOTE: Test run successfully if OK or NONEMBEDDABLE result; Result is only
    // NOTOK when an error occurs during one of the subordinate function calls,
    // or if the output does not match what is expected.
    return (Result == OK || Result == NONEMBEDDABLE) ? OK : Result;
}

int runDrawPlanarNonplanarWriteTest(void)
{
    int Result = OK;
    graphP theGraph = NULL, origGraph = NULL;
    char *actualOutput = NULL;

    gp_Message("Calling DrawPlanar Algorithm on a non-planar graph.");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_Read(theGraph, "Petersen.txt") != OK)
        Result = NOTOK;

    if (Result == OK && (origGraph = gp_DupGraph(theGraph)) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_ExtendWith_DrawPlanar(theGraph) != OK)
        Result = NOTOK;

    if (Result == OK)
    {
        Result = gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR);
        if (Result != NONEMBEDDABLE)
            Result = NOTOK;
    }

    if (Result == NONEMBEDDABLE)
    {
        Result = gp_TestEmbedResultIntegrity(theGraph, origGraph, Result);
        if (Result != NONEMBEDDABLE)
            Result = NOTOK;
    }

    if (Result == NONEMBEDDABLE && gp_SortVertices(theGraph) != OK)
        Result = NOTOK;

    if (Result == NONEMBEDDABLE)
    {
        if (gp_WriteToString(theGraph, &actualOutput, WRITE_ADJLIST) != OK ||
            TextFileMatchesString("Petersen.txt.Planarity.out.txt", actualOutput) != TRUE)
        {
            Result = NOTOK;
        }
        else
            gp_Message("Test succeeded.\n");
    }

    if (actualOutput != NULL)
        free(actualOutput);

    gp_Free(&origGraph);
    gp_Free(&theGraph);

    return Result == NONEMBEDDABLE ? OK : Result;
}
