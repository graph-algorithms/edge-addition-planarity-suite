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

    platform_time start, end;
    char command = '\0', modifier = '\0';

    int charsAvailForFilename = 0;
    char const *messageFormat = NULL;
    char messageContents[MAXLINE + 1];

    testAllStats stats;

    memset(&stats, 0, sizeof(testAllStats));
    messageContents[MAXLINE] = '\0';

    if (GetCommandAndOptionalModifier(commandString, &command, &modifier) != OK)
    {
        gp_ErrorMessage("Unable to determine command (and optional modifier) from command string.\n");

        return NOTOK;
    }

    if (infileName == NULL)
    {
        gp_ErrorMessage("No input file provided.\n");

        return NOTOK;
    }

    messageFormat = "Start testing all graphs in \"%.*s\".\n";
    charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(messageContents, messageFormat, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
    gp_Message(messageContents);

    // Start the timer
    platform_GetTime(start);

    Result = testAllGraphs(command, modifier, infileName, &stats);

    // Stop the timer
    platform_GetTime(end);
    stats.duration = platform_GetDuration(start, end);

    if (Result != OK && Result != NONEMBEDDABLE)
    {
        messageFormat = "\nEncountered error while running command '%c' on all graphs in \"%.*s\".\n";
        charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
        gp_ErrorMessage(messageContents);

        Result = NOTOK;
    }
    else
    {
        sprintf(messageContents, "\nDone testing all graphs (%.3lf seconds).\n", stats.duration);
        gp_Message(messageContents);
    }

    if (outputTestAllGraphsResults(command, modifier, &stats, infileName, outfileName, pOutputStr) != OK)
    {
        messageFormat = "Error outputting results running command '%c' on all graphs in \"%.*s\".\n";
        charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
        gp_ErrorMessage(messageContents);

        Result = NOTOK;
    }

    return Result;
}

int testAllGraphs(char command, char modifier, char const *const infileName, testAllStatsP stats)
{
    int Result = OK;
    graphP theGraph = NULL;
    graphP copyOfOrigGraph = NULL;
    int embedFlags = 0, numOK = 0, numNONEMBEDDABLE = 0, errorFlag = FALSE;

    G6ReadIteratorP theG6ReadIterator = NULL;
    char const *messageFormat = NULL;
    int order = 0;
    char messageContents[MAXLINE + 1];
    messageContents[MAXLINE] = '\0';

    if ((Result = GetEmbedFlags(command, modifier, &embedFlags)) != OK)
    {
        gp_ErrorMessage("Unable to derive embedFlags from command and modifier characters.\n");

        stats->errorFlag = TRUE;

        return Result;
    }

    theGraph = gp_New();

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph.\n");

        stats->errorFlag = TRUE;

        return NOTOK;
    }

    if ((Result = g6_NewReader((&theG6ReadIterator), theGraph)) != OK)
    {
        gp_ErrorMessage("Unable to allocate G6ReadIterator.\n");

        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    if ((Result = g6_InitReaderWithFileName(theG6ReadIterator, infileName)) != OK)
    {
        gp_ErrorMessage("Unable to test all graphs due to failure to initialize"
                        "G6ReadIterator.\n");

        g6_FreeReader((&theG6ReadIterator));
        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    order = gp_GetN(theGraph);
    // We have to set the maximum edge capacity (i.e. (N * (N - 1) / 2)) because some of the test files
    // can contain complete graphs, and the graph drawing, K_{3, 3} search, and K_4 search extensions
    // don't support expanding the edge capacity after being attached.
    if (strchr("d34", command) != NULL)
    {
        if ((Result = gp_EnsureEdgeCapacity(theGraph, (order * (order - 1) / 2))) != OK)
        {
            gp_ErrorMessage("Unable to ensure sufficient edge capacity of the G6ReadIterator's graph struct.\n");

            g6_FreeReader((&theG6ReadIterator));
            gp_Free(&theGraph);
            stats->errorFlag = TRUE;

            return Result;
        }
    }

    if ((Result = ExtendGraph(theGraph, command)) != OK)
    {
        char commandStr[3];
        commandStr[0] = command;
        commandStr[1] = modifier;
        commandStr[2] = '\0';

        messageFormat = "Unable to extend graph with command %s\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, commandStr);
#pragma GCC diagnostic pop

        gp_ErrorMessage(messageContents);

        g6_FreeReader(&theG6ReadIterator);
        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    copyOfOrigGraph = gp_New();
    if (copyOfOrigGraph == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph to store copy of original graph before embedding.\n");

        g6_FreeReader((&theG6ReadIterator));
        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return NOTOK;
    }

    if (gp_InitGraph(copyOfOrigGraph, order) != OK)
    {
        gp_ErrorMessage("Unable to initialize graph datastructure to store copy of original graph before embedding.\n");

        g6_FreeReader((&theG6ReadIterator));
        gp_Free(&theGraph);
        gp_Free(&copyOfOrigGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    while (true)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            messageFormat = "Unable to read graph on line %d from .g6 read iterator.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, theG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            gp_ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        gp_CopyGraph(copyOfOrigGraph, theGraph);

        Result = gp_Embed(theGraph, embedFlags);
        if (Result != OK && Result != NONEMBEDDABLE)
        {
            messageFormat = "Failed to embed graph on line %d for command '%c'.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, theG6ReadIterator->numGraphsRead + 1, command);
#pragma GCC diagnostic pop
            gp_ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }

        if (gp_TestEmbedResultIntegrity(theGraph, copyOfOrigGraph, Result) != Result)
        {
            messageFormat = "Embed integrity check failed for graph on line %d for command '%c'.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, theG6ReadIterator->numGraphsRead + 1, command);
#pragma GCC diagnostic pop
            gp_ErrorMessage(messageContents);

            errorFlag = TRUE;

            Result = NOTOK;
            break;
        }

        if (Result == OK)
            numOK++;
        else if (Result == NONEMBEDDABLE)
            numNONEMBEDDABLE++;
        else
        {
            if (modifier == '\0')
            {
                messageFormat = "Error applying algorithm '%c' to graph on line %d.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                sprintf(messageContents, messageFormat, command, theG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            }
            else
            {
                messageFormat = "Error applying algorithm '%c' with modifier '%c' to graph on line %d.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                sprintf(messageContents, messageFormat, command, modifier, theG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            }
            gp_ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }
    }

    stats->numGraphsRead = theG6ReadIterator->numGraphsRead;
    stats->numOK = numOK;
    stats->numNONEMBEDDABLE = numNONEMBEDDABLE;
    stats->errorFlag = errorFlag;

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);
    gp_Free(&copyOfOrigGraph);

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
        gp_ErrorMessage("Unable to determine the number of characters required to represent testAllGraphs stat values.\n");
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
