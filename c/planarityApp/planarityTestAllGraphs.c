/*
Copyright (c) 1997-2025, John M. Boyer
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
int outputTestAllGraphsResults(char command, char modifier, testAllStatsP stats, char const *const infileName, char *outfileName, char **outputStr);

/****************************************************************************
 TestAllGraphs()
 commandString - command to run; e.g.`-(pdo234)` (plus optional modifier
    character) to perform the corresponding algorithm on each graph in .g6 file
 infileName - non-NULL and nonempty string containing name of .g6 input file
 outfileName - name of primary output file, or NULL
 outputStr - pointer to string which we wish to use to store the result of
    applying the chosen graph algorithm extension to all graphs in the .g6 file
 ****************************************************************************/
int TestAllGraphs(char const *const commandString, char const *const infileName, char *outfileName, char **outputStr)
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
        ErrorMessage("Unable to determine command (and optional modifier) from command string.\n");

        return NOTOK;
    }

    if (infileName == NULL)
    {
        ErrorMessage("No input file provided.\n");

        return NOTOK;
    }

    messageFormat = "Start testing all graphs in \"%.*s\".\n";
    charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(messageContents, messageFormat, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
    Message(messageContents);

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
        ErrorMessage(messageContents);
    }
    else
    {
        sprintf(messageContents, "\nDone testing all graphs (%.3lf seconds).\n", stats.duration);
        Message(messageContents);
    }

    if (outputTestAllGraphsResults(command, modifier, &stats, infileName, outfileName, outputStr) != OK)
    {
        messageFormat = "Error outputting results running command '%c' on all graphs in \"%.*s\".\n";
        charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
        ErrorMessage(messageContents);
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

    G6ReadIteratorP pG6ReadIterator = NULL;
    char const *messageFormat = NULL;
    int graphOrder = 0;
    char messageContents[MAXLINE + 1];
    messageContents[MAXLINE] = '\0';

    if ((Result = GetEmbedFlags(command, modifier, &embedFlags)) != OK)
    {
        ErrorMessage("Unable to derive embedFlags from command and modifier characters.\n");

        stats->errorFlag = TRUE;

        return Result;
    }

    theGraph = gp_New();

    if (theGraph == NULL)
    {
        ErrorMessage("Unable to allocate graph.\n");

        stats->errorFlag = TRUE;

        return NOTOK;
    }

    if ((Result = allocateG6ReadIterator(&pG6ReadIterator, theGraph)) != OK)
    {
        ErrorMessage("Unable to allocate G6ReadIterator.\n");

        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    if ((Result = beginG6ReadIterationFromG6FilePath(pG6ReadIterator, infileName)) != OK)
    {
        ErrorMessage("Unable to begin .g6 read iteration.\n");

        if (freeG6ReadIterator(&pG6ReadIterator) != OK)
            ErrorMessage("Unable to free G6ReadIterator.\n");

        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    graphOrder = gp_getN(theGraph);
    // We have to set the maximum arc capacity (i.e. (N * (N - 1))) because some of the test files
    // can contain complete graphs, and the graph drawing, K_{3, 3} search, and K_4 search extensions
    // don't support expanding the arc capacity after being attached.
    if (strchr("d34", command) != NULL)
    {
        if ((Result = gp_EnsureArcCapacity(theGraph, (graphOrder * (graphOrder - 1)))) != OK)
        {
            ErrorMessage("Unable to maximize arc capacity of G6ReadIterator's graph struct.\n");

            if (freeG6ReadIterator(&pG6ReadIterator) != OK)
                ErrorMessage("Unable to free G6ReadIterator.\n");

            gp_Free(&theGraph);
            stats->errorFlag = TRUE;

            return Result;
        }
    }

    if ((Result = AttachAlgorithm(theGraph, command)) != OK)
    {
        if (modifier == '\0')
        {
            messageFormat = "Unable to attach graph algorithm extension corresponding to command specifier '%c' to graphP.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, command);
#pragma GCC diagnostic pop
        }
        else
        {
            messageFormat = "Unable to attach graph algorithm extension corresponding to command specifier '%c' with modifier '%c' to graphP.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, command, modifier);
#pragma GCC diagnostic pop
        }

        ErrorMessage(messageContents);

        if (freeG6ReadIterator(&pG6ReadIterator) != OK)
            ErrorMessage("Unable to free G6ReadIterator.\n");

        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    copyOfOrigGraph = gp_New();
    if (copyOfOrigGraph == NULL)
    {
        ErrorMessage("Unable to allocate graph to store copy of original graph before embedding.\n");

        if (freeG6ReadIterator(&pG6ReadIterator) != OK)
            ErrorMessage("Unable to free G6ReadIterator.\n");

        gp_Free(&theGraph);
        stats->errorFlag = TRUE;

        return NOTOK;
    }

    if (gp_InitGraph(copyOfOrigGraph, graphOrder) != OK)
    {
        ErrorMessage("Unable to initialize graph datastructure to store copy of original graph before embedding.\n");

        if (freeG6ReadIterator(&pG6ReadIterator) != OK)
            ErrorMessage("Unable to free G6ReadIterator.\n");

        gp_Free(&theGraph);
        gp_Free(&copyOfOrigGraph);
        stats->errorFlag = TRUE;

        return Result;
    }

    while (true)
    {
        if (readGraphUsingG6ReadIterator(pG6ReadIterator) != OK)
        {
            messageFormat = "Unable to read graph on line %d from .g6 read iterator.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, pG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }

        if (contentsExhausted(pG6ReadIterator))
            break;

        gp_CopyGraph(copyOfOrigGraph, theGraph);

        Result = gp_Embed(theGraph, embedFlags);
        if (Result != OK && Result != NONEMBEDDABLE)
        {
            messageFormat = "Failed to embed graph on line %d for command '%c'.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, pG6ReadIterator->numGraphsRead + 1, command);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }

        if (gp_TestEmbedResultIntegrity(theGraph, copyOfOrigGraph, Result) != Result)
        {
            messageFormat = "Embed integrity check failed for graph on line %d for command '%c'.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, pG6ReadIterator->numGraphsRead + 1, command);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

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
                sprintf(messageContents, messageFormat, command, pG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            }
            else
            {
                messageFormat = "Error applying algorithm '%c' with modifier '%c' to graph on line %d.\n";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                sprintf(messageContents, messageFormat, command, modifier, pG6ReadIterator->numGraphsRead + 1);
#pragma GCC diagnostic pop
            }
            ErrorMessage(messageContents);

            errorFlag = TRUE;

            break;
        }
    }

    stats->numGraphsRead = pG6ReadIterator->numGraphsRead;
    stats->numOK = numOK;
    stats->numNONEMBEDDABLE = numNONEMBEDDABLE;
    stats->errorFlag = errorFlag;

    if (endG6ReadIteration(pG6ReadIterator) != OK)
        ErrorMessage("Unable to end G6ReadIterator.\n");

    if (freeG6ReadIterator(&pG6ReadIterator) != OK)
        ErrorMessage("Unable to free G6ReadIterator.\n");

    gp_Free(&theGraph);
    gp_Free(&copyOfOrigGraph);

    return Result;
}

int outputTestAllGraphsResults(char command, char modifier, testAllStatsP stats, char const *const infileName, char *outfileName, char **outputStr)
{
    int Result = OK;

    char *finalSlash = strrchr(infileName, FILE_DELIMITER);
    char const *infileBasename = finalSlash ? (finalSlash + 1) : infileName;

    char const *headerFormat = "FILENAME=\"%s\" DURATION=\"%.3lf\"\n";
    char *headerStr = NULL;
    int numCharsToReprNumGraphsRead = 0, numCharsToReprNumOK = 0, numCharsToReprNumNONEMBEDDABLE = 0;
    char *resultsStr = NULL;
    strOrFileP testOutput = NULL;

    headerStr = (char *)malloc(
        (
            strlen(headerFormat) +
            strlen(infileBasename) +
            strlen("-1.7976931348623158e+308") + // -DBL_MAX from float.h
            3) *
        sizeof(char));

    if (headerStr == NULL)
    {
        ErrorMessage("Unable allocate memory for output file header.\n");

        return NOTOK;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(headerStr, headerFormat, infileBasename, stats->duration);
#pragma GCC diagnostic pop

    if (GetNumCharsToReprInt(stats->numGraphsRead, &numCharsToReprNumGraphsRead) != OK ||
        GetNumCharsToReprInt(stats->numOK, &numCharsToReprNumOK) != OK ||
        GetNumCharsToReprInt(stats->numNONEMBEDDABLE, &numCharsToReprNumNONEMBEDDABLE) != OK)
    {
        ErrorMessage("Unable to determine the number of characters required to represent testAllGraphs stat values.\n");

        if (headerStr != NULL)
        {
            free(headerStr);
            headerStr = NULL;
        }

        return NOTOK;
    }

    resultsStr = (char *)malloc(
        (
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
            3   // (carriage return,) newline and null terminator
            ) *
        sizeof(char));

    if (resultsStr == NULL)
    {
        ErrorMessage("Unable allocate memory for results string.\n");

        if (headerStr != NULL)
        {
            free(headerStr);
            headerStr = NULL;
        }

        return NOTOK;
    }

    if (modifier == '\0')
        sprintf(resultsStr, "-%c %d %d %d %s\n",
                command, stats->numGraphsRead, stats->numOK, stats->numNONEMBEDDABLE, stats->errorFlag ? "ERROR" : "SUCCESS");
    else
        sprintf(resultsStr, "-%c%c %d %d %d %s\n",
                command, modifier, stats->numGraphsRead, stats->numOK, stats->numNONEMBEDDABLE, stats->errorFlag ? "ERROR" : "SUCCESS");

    if (outfileName != NULL)
    {
        testOutput = sf_New(NULL, outfileName, WRITETEXT);
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
                testOutput = sf_New(NULL, NULL, WRITETEXT);
            }
        }
    }

    if (testOutput == NULL)
    {
        ErrorMessage("Unable to set up string-or-file container for test output.\n");

        Result = NOTOK;
    }

    if (Result == OK || Result == NONEMBEDDABLE)
    {
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
    }

    if (headerStr != NULL)
    {
        free(headerStr);
        headerStr = NULL;
    }

    if (resultsStr != NULL)
    {
        free(resultsStr);
        resultsStr = NULL;
    }

    sf_Free(&testOutput);

    return Result;
}
