/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

typedef struct
{
    double duration;
    int numGraphsRead;
    int numOK;
    int numNONEMBEDDABLE;
    int errorFlag;
} testAllStats;

typedef testAllStats *testAllStatsP;

int testAllGraphs(char command, char modifier, char const *const infileName, testAllStatsP stats);
int outputTestAllGraphsResults(char command, char modifier, testAllStatsP stats, char const *const infileName, char *outfileName, char **pOutputStr);

/****************************************************************************
 TestAllGraphs()
 commandString - command to run; e.g.`-(pdo234)` (plus optional modifier
    character) to perform the corresponding algorithm on each graph in .g6 file
 infileName - non-NULL and nonempty string containing name of .g6 input file
 outfileName - name of primary output file, or NULL
 pOutputStr - pointer to string which we wish to use to store the result of
    applying the chosen graph algorithm extension to all graphs in the .g6 file
 ****************************************************************************/
int TestAllGraphs(char const *const commandString, char const *const infileName, char *outfileName, char **pOutputStr)
{
    int Result = OK;

    char command = '\0', modifier = '\0';
    platform_time start, end;
    testAllStats stats;

    memset(&stats, 0, sizeof(testAllStats));

    if (GetCommandAndOptionalModifier(commandString, &command, &modifier) != OK)
    {
        gp_ErrorMessage("Unable to determine command (and optional modifier) from "
                        "command string.\n");
        return NOTOK;
    }

    if (infileName == NULL)
    {
        gp_ErrorMessage("No input file provided.\n");
        return NOTOK;
    }

    gp_Message("Start testing all graphs in \"%.*s\".\n",
               FILENAME_MAX,
               infileName);

    // Start the timer
    platform_GetTime(start);

    Result = testAllGraphs(command, modifier, infileName, &stats);

    // Stop the timer
    platform_GetTime(end);
    stats.duration = platform_GetDuration(start, end);

    if (Result != OK)
    {
        gp_ErrorMessage("\nEncountered error while running command '%c' on all "
                        "graphs in \"%.*s\".\n",
                        command, FILENAME_MAX, infileName);
        Result = NOTOK;
    }
    else
    {
        gp_Message("\nDone testing all graphs (%.3lf seconds).\n", stats.duration);
    }

    if (outputTestAllGraphsResults(command, modifier, &stats, infileName, outfileName, pOutputStr) != OK)
    {
        gp_ErrorMessage("Error outputting results running command '%c' on all "
                        "graphs in \"%.*s\".\n",
                        command, FILENAME_MAX, infileName);
        Result = NOTOK;
    }

    return Result;
}

int testAllGraphs(char command, char modifier, char const *const infileName, testAllStatsP stats)
{
    int Result = OK;

    graphP origGraphRead = NULL;
    graphP graphForEmbedding = NULL;
    int embedFlags = 0, numOK = 0, numNONEMBEDDABLE = 0;
    int order = 0, maxNumEdgesForOrder = 0;

    G6ReadIteratorP theG6ReadIterator = NULL;

    if (GetEmbedFlags(command, modifier, &embedFlags) != OK)
    {
        gp_ErrorMessage("Unable to derive embedFlags from command and modifier "
                        "characters.\n");
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    if ((origGraphRead = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph.\n");
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    if (
        g6_NewReader((&theG6ReadIterator), origGraphRead) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, infileName) != OK)
    {
        gp_ErrorMessage("Unable to test all graphs due to failure to allocate or "
                        "initialize G6ReadIterator.\n");
        gp_Free(&origGraphRead);
        g6_FreeReader((&theG6ReadIterator));
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    // The order of the graphs in the G6 source file or string was determined by
    // g6_InitReaderWithFileName() and we obtain it to initialize the graph for
    // embedding
    order = gp_GetN(origGraphRead);

    if (
        (graphForEmbedding = gp_New()) == NULL ||
        gp_InitGraph(graphForEmbedding, order) != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize graph for embedding "
                        "operation.\n");
        g6_FreeReader((&theG6ReadIterator));
        gp_Free(&origGraphRead);
        gp_Free(&graphForEmbedding);
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    maxNumEdgesForOrder = (order * (order - 1)) / 2;
    // We have to set the maximum edge capacity (i.e. (N * (N - 1) / 2)) because
    // some of the test files contain graphs with an edge count greater than the
    // default of 3 * N.
    // Additionally, we have to set the maximum edge capacity because otherwise
    // gp_CopyGraph() will fail due to the destination graph (graphForEmbedding)
    // having a greater edge capacity than the source graph (origGraphRead)
    if (
        gp_EnsureEdgeCapacity(origGraphRead, (order * (order - 1) / 2)) != OK ||
        gp_EnsureEdgeCapacity(graphForEmbedding, maxNumEdgesForOrder) != OK)
    {
        gp_ErrorMessage("Unable to ensure sufficient edge capacity of the "
                        "original graph read or the graph for embedding.\n");
        g6_FreeReader((&theG6ReadIterator));
        gp_Free(&origGraphRead);
        gp_Free(&graphForEmbedding);
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    if (ExtendGraph(origGraphRead, command) != OK ||
        ExtendGraph(graphForEmbedding, command) != OK)
    {
        gp_ErrorMessage("Unable to extend graph to support requested graph "
                        "embedding operation.");
        g6_FreeReader(&theG6ReadIterator);
        gp_Free(&origGraphRead);
        gp_Free(&graphForEmbedding);
        stats->errorFlag = TRUE;
        return NOTOK;
    }

    while (true)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            int numGraphsRead = 0;
            g6_GetNumGraphsRead(theG6ReadIterator, &numGraphsRead);
            gp_ErrorMessage("Unable to read graph on line %d from .g6 read "
                            "iterator.\n",
                            numGraphsRead + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        if (gp_CopyGraph(graphForEmbedding, origGraphRead) != OK)
        {
            gp_ErrorMessage("Unable to copy graph read into graph for "
                            "embedding.\n");
            Result = NOTOK;
            break;
        }

        Result = gp_Embed(graphForEmbedding, embedFlags);
        if (Result != OK && Result != NONEMBEDDABLE)
        {
            int numGraphsRead = 0;
            g6_GetNumGraphsRead(theG6ReadIterator, &numGraphsRead);
            gp_ErrorMessage("Failed to embed graph on line %d for command '%c'.\n",
                            numGraphsRead + 1, command);
            Result = NOTOK;
        }

        if (gp_TestEmbedResultIntegrity(graphForEmbedding, origGraphRead, Result) != Result)
        {
            int numGraphsRead = 0;
            g6_GetNumGraphsRead(theG6ReadIterator, &numGraphsRead);
            gp_ErrorMessage("Embed integrity check failed for graph on line %d "
                            "for command '%c'.\n",
                            numGraphsRead + 1, command);
            Result = NOTOK;
        }

        if (Result == OK)
            numOK++;
        else if (Result == NONEMBEDDABLE)
        {
            numNONEMBEDDABLE++;
            // Now that we've processed the NONEMBEDDABLE result, we set the
            // Result to OK so that we exit the loop with an OK or NOTOK only
            Result = OK;
        }
        else
        {
            if (modifier == '\0')
            {
                int numGraphsRead = 0;
                g6_GetNumGraphsRead(theG6ReadIterator, &numGraphsRead);
                gp_ErrorMessage("Error applying algorithm '%c' to graph on line "
                                "%d.\n",
                                command, numGraphsRead + 1);
            }
            else
            {
                int numGraphsRead = 0;
                g6_GetNumGraphsRead(theG6ReadIterator, &numGraphsRead);
                gp_ErrorMessage("Error applying algorithm '%c' with modifier '%c' "
                                "to graph on line %d.\n",
                                command, modifier, numGraphsRead + 1);
            }
            Result = NOTOK;
            break;
        }
    }

    stats->numGraphsRead = 0;
    if (g6_GetNumGraphsRead(theG6ReadIterator, &stats->numGraphsRead) != OK)
        NOTOK;
    stats->numOK = numOK;
    stats->numNONEMBEDDABLE = numNONEMBEDDABLE;
    stats->errorFlag = (Result == OK) ? FALSE : TRUE;

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&origGraphRead);
    gp_Free(&graphForEmbedding);

    return Result;
}

int outputTestAllGraphsResults(char command, char modifier, testAllStatsP stats, char const *const infileName, char *outfileName, char **pOutputStr)
{
    int Result = OK;

    char *finalSlash = strrchr(infileName, FILE_DELIMITER);
    char const *infileBasename = finalSlash ? (finalSlash + 1) : infileName;

    char const *headerFormat = "FILENAME=\"%s\" DURATION=\"%.3lf\"\n";
    int numCharsToReprNumGraphsRead = 0, numCharsToReprNumOK = 0, numCharsToReprNumNONEMBEDDABLE = 0;

    char *theOutputStr = NULL;
    int headerStrLen = 0, resultStrLen = 0;
    char *resultsStr = NULL;

    if (outfileName == NULL && (pOutputStr == NULL || *pOutputStr == NULL))
    {
        gp_ErrorMessage("Invalid parameters: Must be able to output to file or memory.");
        return NOTOK;
    }

    headerStrLen =
        strlen(headerFormat) +
        strlen(infileBasename) +
        strlen("-1.7976931348623158e+308") + // -DBL_MAX from float.h
        3;

    if (GetNumCharsToReprInt(stats->numGraphsRead, &numCharsToReprNumGraphsRead) != OK ||
        GetNumCharsToReprInt(stats->numOK, &numCharsToReprNumOK) != OK ||
        GetNumCharsToReprInt(stats->numNONEMBEDDABLE, &numCharsToReprNumNONEMBEDDABLE) != OK)
    {
        gp_ErrorMessage("Unable to determine the number of characters required to "
                        "represent testAllGraphs stat values.\n");
        return NOTOK;
    }

    resultStrLen =
        1 + // - char
        1 + // command char
        1 + // optional modifier char
        1 + // space char
        numCharsToReprNumGraphsRead +
        1 + // space char
        numCharsToReprNumOK +
        1 + // space char
        numCharsToReprNumNONEMBEDDABLE +
        1 + // space char
        7 + // either ERROR or SUCCESS, so the longer of which is 7 chars
        3   // (carriage return,) newline and null terminator;
        ;

    theOutputStr = (char *)malloc((headerStrLen + resultStrLen + 1) * sizeof(char));
    if (theOutputStr == NULL)
    {
        gp_ErrorMessage("Unable allocate memory for the output.\n");
        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(theOutputStr, headerFormat, infileBasename, stats->duration);
#pragma GCC diagnostic pop

    resultsStr = theOutputStr + strlen(theOutputStr);

    if (modifier == '\0')
        sprintf(resultsStr, "-%c %d %d %d %s\n",
                command, stats->numGraphsRead, stats->numOK, stats->numNONEMBEDDABLE, stats->errorFlag ? "ERROR" : "SUCCESS");
    else
        sprintf(resultsStr, "-%c%c %d %d %d %s\n",
                command, modifier, stats->numGraphsRead, stats->numOK, stats->numNONEMBEDDABLE, stats->errorFlag ? "ERROR" : "SUCCESS");

    if (outfileName != NULL)
    {
        FILE *outfile = strcmp(outfileName, "stdout") == 0 ? stdout : fopen(outfileName, WRITETEXT);
        if (outfile != NULL)
        {
            fprintf(outfile, "%s", theOutputStr);
            if (strcmp(outfileName, "stdout") != 0)
                fclose(outfile);
            outfile = NULL;
            Result = OK;
        }
        else
            Result = NOTOK;

        free(theOutputStr);
        theOutputStr = NULL;
    }
    else
    {
        *pOutputStr = theOutputStr;
        theOutputStr = NULL;
        Result = OK;
    }

    return Result;
}
