/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runTestAllGraphsTests(void);
int runTestAllGraphsTest(char const *commandString, char const *infileName, char const *expectedValidationStr, int (*mutatorFunc)(graphP));

int runTestAllGraphsTests(void)
{
    int retVal = OK;

    // Run TestAllGraphs Tests
    if (runTestAllGraphsTest("-p", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("Planarity test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-d", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("Planar graph drawing test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-o", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("Outerplanarity test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-2", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("K2,3 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-3", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("K3,3 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-4", "n8.mALL.g6", NULL, NULL) != OK)
    {
        gp_ErrorMessage("K4 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }

    return retVal;
}

int runTestAllGraphsTest(char const *commandString, char const *infileName, char const *expectedValidationStr, int (*mutatorFunc)(graphP))
{
    char *outputStr = NULL;
    int Result = OK;
    char command = '\0', modifier = '\0';

    if (GetCommandAndOptionalModifier(commandString, &command, &modifier) != OK)
    {
        gp_ErrorMessage("Unable to extract command (or optional modifier) from "
                        "command string.");
        return NOTOK;
    }

    Result = TestAllGraphs(commandString, infileName, NULL, &outputStr, mutatorFunc);

    if (Result == OK && expectedValidationStr != NULL)
    {
        Result = strstr(outputStr, expectedValidationStr) ? OK : NOTOK;
    }
    else if (Result == OK)
    {
        const char *planarityValidationStr = "-p 12346 6966 5380 SUCCESS";
        const char *drawPlanarValidationStr = "-d 12346 6966 5380 SUCCESS";
        const char *outerplanarityValidationStr = "-o 12346 1150 11196 SUCCESS";
        const char *K23SearchValidationStr = "-2 12346 1251 11095 SUCCESS";
        const char *K33SearchValidationStr = "-3 12346 7200 5146 SUCCESS";
        const char *K4SearchValidationStr = "-4 12346 1715 10631 SUCCESS";
        const char *theValidationStr = NULL;

        switch (command)
        {
        case 'p':
            theValidationStr = planarityValidationStr;
            break;
        case 'd':
            theValidationStr = drawPlanarValidationStr;
            break;
        case 'o':
            theValidationStr = outerplanarityValidationStr;
            break;
        case '2':
            theValidationStr = K23SearchValidationStr;
            break;
        case '3':
            theValidationStr = K33SearchValidationStr;
            break;
        case '4':
            theValidationStr = K4SearchValidationStr;
            break;
        default:
            Result = NOTOK;
            break;
        }

        if (theValidationStr != NULL)
            Result = strstr(outputStr, theValidationStr) ? OK : NOTOK;
    }

    gp_Message(" ");

    if (outputStr != NULL)
    {
        free(outputStr);
        outputStr = NULL;
    }

    return Result == OK ? OK : NOTOK;
}
