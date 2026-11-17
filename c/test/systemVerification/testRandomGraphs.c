/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "../../planarityApp/planarity.h"

/* Private function declarations (exported within system) */
int runRandomGraphsTests(void);

int runRandomGraphsTests(void)
{
    int retVal = OK;
    unsigned quietModeCache = gp_GetQuietMode();
    platform_time start, end;
    double duration;

    gp_Message("Starting Random Graph Tests");
    platform_GetTime(start);

    if (RandomGraphs("-p", 1000, 20, NULL, TRUE, FALSE, NULL) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-p", 1000, 20, NULL, TRUE, TRUE, NULL) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraphEx() test failed.");
        retVal = NOTOK;
    }

    // Suppress RandomGraph()'s interactive save prompts while exercising the
    // maximal-planar and nonplanar paths used by the -rm and -rn endpoints.
    gp_SetQuietMode(quietModeCache | QUIETMODE_MESSAGES);

    // N=46342 chosen to sanitize signed int overflow on simple graph clique size
    if (RandomGraph("-p", 0, 46342, NULL, NULL) != OK)
    {
        gp_ErrorMessage("Random maximal planar graph test failed.");
        retVal = NOTOK;
    }

    // N=65538 chosen to sanitize unsigned int overflow on simple graph clique size
    if (RandomGraph("-p", 1, 65538, NULL, NULL) != NONEMBEDDABLE)
    {
        gp_ErrorMessage("Random nonplanar graph test failed.");
        retVal = NOTOK;
    }

    gp_SetQuietMode(quietModeCache);

    platform_GetTime(end);
    duration = platform_GetDuration(start, end);

    if (retVal == OK)
        gp_Message("Finished Random Graph Tests (%.3lf seconds).\n", duration);

    return retVal;
}
