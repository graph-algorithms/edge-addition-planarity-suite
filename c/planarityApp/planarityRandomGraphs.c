/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

int GetNumberIfZero(int *pNum, char const *prompt, int min, int max);
void ReinitializeGraph(graphP *pGraph, int ReuseGraphs, char command);
graphP MakeGraph(int Size, char command);
int WriteEdgeListFormat(graphP theGraph, graphP origGraph, int extraEdges);

/****************************************************************************
 * RandomGraphs()
 *
 * Top-level method to randomly generate graphs to test the algorithm given by
 * the command parameter.
 * The number of graphs to generate, and the number of vertices for each graph,
 * can be sent as the second and third params.  For each that is sent as zero,
 * this method will prompt the user for a value.
 ****************************************************************************/

#define NUM_MINORS 9

int RandomGraphs(char const *const commandString, int NumGraphs, int SizeOfGraphs, char *outfileName)
{
    int Result = OK;
    int writeResult = OK;

    int K = 0, countUpdateFreq = 0, embedFlags = 0, MainStatistic = 0;
    int ReuseGraphs = TRUE;

    char command = '\0', modifier = '\0';

    int ObstructionMinorFreqs[NUM_MINORS];

    graphP theGraph = NULL, origGraph = NULL;

    platform_time start, end;

    G6WriteIteratorP pG6WriteIterator = NULL;

    int charsAvailForStr = 0;
    char const *messageFormat = NULL;
    char messageContents[MAXLINE + 1];
    char theFileName[FILENAMEMAXLENGTH + 1];

    memset(ObstructionMinorFreqs, 0, NUM_MINORS * sizeof(int));
    memset(messageContents, '\0', (MAXLINE + 1));
    memset(theFileName, '\0', (FILENAMEMAXLENGTH + 1));

    if ((Result = GetCommandAndOptionalModifier(commandString, &command, &modifier)) != OK)
    {
        ErrorMessage("Unable to extract command and optional modifier character from commandString.\n");
        return Result;
    }

    if ((Result = GetEmbedFlags(command, modifier, &embedFlags)) != OK)
    {
        ErrorMessage("Unable to derive embedFlags from command and optional modifier character.\n");
        return Result;
    }

    if ((Result = GetNumberIfZero(&NumGraphs, "Enter number of graphs to generate:", 1, 1000000000)) != OK)
    {
        ErrorMessage("Encountered unrecoverable error when prompting for NumGraphs.\n");
        return Result;
    }

    if ((Result = GetNumberIfZero(&SizeOfGraphs, "Enter size of graphs:", 1, 10000)) != OK)
    {
        ErrorMessage("Encountered unrecoverable error when prompting for SizeOfGraphs.\n");
        return Result;
    }

    theGraph = MakeGraph(SizeOfGraphs, command);
    origGraph = MakeGraph(SizeOfGraphs, command);
    if (theGraph == NULL || origGraph == NULL)
    {
        ErrorMessage("Unable to allocate and initialize graph datastructures to contain randomly generated graphs.\n");

        gp_Free(&theGraph);
        gp_Free(&origGraph);

        return NOTOK;
    }

    if (outfileName != NULL || (tolower(OrigOut) == 'y' && tolower(OrigOutFormat) == 'g'))
    {
        if (allocateG6WriteIterator(&pG6WriteIterator, theGraph) != OK)
        {
            ErrorMessage("Unable to allocate G6WriteIterator.\n");

            gp_Free(&theGraph);
            gp_Free(&origGraph);

            return NOTOK;
        }
    }

    messageFormat = "Unable to begin writing random graphs to G6 outfile \"%.*s\".\n";
    charsAvailForStr = (int)(MAXLINE - strlen(messageFormat));
    if (outfileName != NULL)
    {
        if (beginG6WriteIterationToG6FilePath(pG6WriteIterator, outfileName) != OK)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, charsAvailForStr, outfileName);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

            if (freeG6WriteIterator(&pG6WriteIterator) != OK)
                ErrorMessage("Unable to free G6WriteIterator.\n");

            gp_Free(&theGraph);
            gp_Free(&origGraph);

            return NOTOK;
        }
    }
    else if (tolower(OrigOut) == 'y' && tolower(OrigOutFormat) == 'g')
    {
        // If outfileName is NULL, then the only case in which we would want to
        // output the generated random graphs to .g6 is if we Reconfigure() and
        // choose these options; in that case, need to set a default output filename.
        sprintf(theFileName, "random%cn%d.k%d.g6", FILE_DELIMITER, SizeOfGraphs, NumGraphs);
        if (beginG6WriteIterationToG6FilePath(pG6WriteIterator, theFileName) != OK)
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

            if (freeG6WriteIterator(&pG6WriteIterator) != OK)
                ErrorMessage("Unable to free G6WriteIterator.\n");

            gp_Free(&theGraph);
            gp_Free(&origGraph);

            return NOTOK;
        }
    }

    // Seed the random number generator with "now". Do it after any prompting
    // to tie randomness to human process of answering the prompt.
    // Acceptable downcast of time_t to unsigned int (seeding benefits from the lower bits of now)
    srand(time(NULL));

    // Select a counter update frequency that updates more frequently with larger graphs
    // and which is relatively prime with 10 so that all digits of the count will change
    // even though we aren't showing the count value on every iteration
    countUpdateFreq = 3579 / SizeOfGraphs;
    countUpdateFreq = countUpdateFreq < 1 ? 1 : countUpdateFreq;
    countUpdateFreq = countUpdateFreq % 2 == 0 ? countUpdateFreq + 1 : countUpdateFreq;
    countUpdateFreq = countUpdateFreq % 5 == 0 ? countUpdateFreq + 2 : countUpdateFreq;

    // Start the count
    fprintf(stdout, "0\r");
    fflush(stdout);

    // Start the timer
    platform_GetTime(start);

    messageFormat = "Failed to write graph \"%.*s\"\nMake the directory if not present\n";
    charsAvailForStr = (int)(MAXLINE - strlen(messageFormat));
    // Generate and process the number of graphs requested
    for (K = 0; K < NumGraphs; K++)
    {
        if ((Result = gp_CreateRandomGraph(theGraph)) == OK)
        {
            if (pG6WriteIterator != NULL)
            {
                if ((writeResult = writeGraphUsingG6WriteIterator(pG6WriteIterator)) != OK)
                {
                    sprintf(messageContents, "Unable to write graph number %d using G6WriteIterator.\n", K);
                    ErrorMessage(messageContents);

                    Result = writeResult;

                    break;
                }
            }
            else if (tolower(OrigOut) == 'y' && tolower(OrigOutFormat) == 'a')
            {
                sprintf(theFileName, "random%c%d.txt", FILE_DELIMITER, K % 10);
                if ((writeResult = gp_Write(theGraph, theFileName, WRITE_ADJLIST)) != OK)
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
                    ErrorMessage(messageContents);

                    Result = writeResult;

                    break;
                }
            }

            if ((Result = gp_CopyGraph(origGraph, theGraph)) != OK)
            {
                sprintf(messageContents, "Unable to make a copy of graph number %d before embedding.\n", K);
                ErrorMessage(messageContents);

                gp_Free(&theGraph);
                gp_Free(&origGraph);
                freeG6WriteIterator(&pG6WriteIterator);

                return Result;
            }

            Result = gp_Embed(theGraph, embedFlags);

            if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != Result)
                Result = NOTOK;

            if (Result == OK)
            {
                MainStatistic++;

                if (tolower(EmbeddableOut) == 'y')
                {
                    sprintf(theFileName, "embedded%c%d.txt", FILE_DELIMITER, K % 10);

                    if ((writeResult = gp_Write(theGraph, theFileName, WRITE_ADJMATRIX)) != OK)
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                        sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
                        ErrorMessage(messageContents);

                        Result = writeResult;
                    }
                }

                if (tolower(AdjListsForEmbeddingsOut) == 'y')
                {
                    sprintf(theFileName, "adjlist%c%d.txt", FILE_DELIMITER, K % 10);

                    if ((writeResult = gp_Write(theGraph, theFileName, WRITE_ADJLIST)) != OK)
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                        sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
                        ErrorMessage(messageContents);

                        Result = writeResult;
                    }
                }
            }
            else if (Result == NONEMBEDDABLE)
            {
                if (embedFlags == EMBEDFLAGS_PLANAR || embedFlags == EMBEDFLAGS_OUTERPLANAR)
                {
                    if (theGraph->IC.minorType & MINORTYPE_A)
                        ObstructionMinorFreqs[0]++;
                    else if (theGraph->IC.minorType & MINORTYPE_B)
                        ObstructionMinorFreqs[1]++;
                    else if (theGraph->IC.minorType & MINORTYPE_C)
                        ObstructionMinorFreqs[2]++;
                    else if (theGraph->IC.minorType & MINORTYPE_D)
                        ObstructionMinorFreqs[3]++;
                    else if (theGraph->IC.minorType & MINORTYPE_E)
                        ObstructionMinorFreqs[4]++;

                    if (theGraph->IC.minorType & MINORTYPE_E1)
                        ObstructionMinorFreqs[5]++;
                    else if (theGraph->IC.minorType & MINORTYPE_E2)
                        ObstructionMinorFreqs[6]++;
                    else if (theGraph->IC.minorType & MINORTYPE_E3)
                        ObstructionMinorFreqs[7]++;
                    else if (theGraph->IC.minorType & MINORTYPE_E4)
                        ObstructionMinorFreqs[8]++;

                    if (tolower(ObstructedOut) == 'y')
                    {
                        sprintf(theFileName, "obstructed%c%d.txt", FILE_DELIMITER, K % 10);

                        if ((writeResult = gp_Write(theGraph, theFileName, WRITE_ADJMATRIX)) != OK)
                        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                            sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
                            ErrorMessage(messageContents);

                            Result = writeResult;
                        }
                    }
                }
            }

            // If there is an error in processing, then write the file for debugging
            if (Result != OK && Result != NONEMBEDDABLE)
            {
                sprintf(theFileName, "error%c%d.txt", FILE_DELIMITER, K % 10);
                if ((writeResult = gp_Write(origGraph, theFileName, WRITE_ADJLIST)) != OK)
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
                    ErrorMessage(messageContents);

                    Result = writeResult;
                }
            }
        }

        // Terminate loop on error
        if (Result != OK && Result != NONEMBEDDABLE)
        {
            ErrorMessage("\nError found\n");

            Result = NOTOK;

            break;
        }

        // Reinitialize or recreate graphs for next iteration
        ReinitializeGraph(&theGraph, ReuseGraphs, command);

        // Show progress, but not so often that it bogs down progress
        if (!getQuietModeSetting() && (K + 1) % countUpdateFreq == 0)
        {
            fprintf(stdout, "%d\r", K + 1);
            fflush(stdout);
        }
    }

    // Stop the timer
    platform_GetTime(end);

    // Finish the count
    fprintf(stdout, "%d\n", NumGraphs);
    fflush(stdout);

    sprintf(messageContents, "\nDone (%.3lf seconds).\n", platform_GetDuration(start, end));
    Message(messageContents);

    // Print some demographic results
    if (Result == OK || Result == NONEMBEDDABLE)
    {
        Message("\nNo Errors Found.\n");
        // Report statistics for planar or outerplanar embedding
        if (embedFlags == EMBEDFLAGS_PLANAR || embedFlags == EMBEDFLAGS_OUTERPLANAR)
        {
            sprintf(messageContents, "Num Embedded=%d.\n", MainStatistic);
            Message(messageContents);

            for (K = 0; K < 5; K++)
            {
                // Outerplanarity does not produces minors C and D
                if (embedFlags == EMBEDFLAGS_OUTERPLANAR && (K == 2 || K == 3))
                    continue;

                sprintf(messageContents, "Minor %c = %d\n", K + 'A', ObstructionMinorFreqs[K]);
                Message(messageContents);
            }

            if (!(embedFlags & ~EMBEDFLAGS_PLANAR))
            {
                sprintf(messageContents, "\nNote: E1 are added to C, E2 are added to A, and E=E3+E4+K5 homeomorphs.\n");
                Message(messageContents);

                for (K = 5; K < NUM_MINORS; K++)
                {
                    sprintf(messageContents, "Minor E%d = %d\n", K - 4, ObstructionMinorFreqs[K]);
                    Message(messageContents);
                }
            }
        }

        // Report statistics for graph drawing
        else if (embedFlags == EMBEDFLAGS_DRAWPLANAR)
        {
            sprintf(messageContents, "Num Graphs Embedded and Drawn=%d.\n", MainStatistic);
            Message(messageContents);
        }

        // Report statistics for subgraph homeomorphism algorithms
        else if (embedFlags == EMBEDFLAGS_SEARCHFORK23)
        {
            sprintf(messageContents, "Of the generated graphs, %d did not contain a K_{2,3} homeomorph as a subgraph.\n", MainStatistic);
            Message(messageContents);
        }
        else if (embedFlags == EMBEDFLAGS_SEARCHFORK33)
        {
            sprintf(messageContents, "Of the generated graphs, %d did not contain a K_{3,3} homeomorph as a subgraph.\n", MainStatistic);
            Message(messageContents);
        }
        else if (embedFlags == EMBEDFLAGS_SEARCHFORK4)
        {
            sprintf(messageContents, "Of the generated graphs, %d did not contain a K_4 homeomorph as a subgraph.\n", MainStatistic);
            Message(messageContents);
        }
    }

    FlushConsole(stdout);

    if (pG6WriteIterator != NULL)
    {
        if (endG6WriteIteration(pG6WriteIterator) != OK)
        {
            ErrorMessage("Unable to end .g6 write iteration.\n");
            Result = NOTOK;
        }

        if (freeG6WriteIterator(&pG6WriteIterator) != OK)
        {
            ErrorMessage("Unable to free .g6 write iterator.\n");
            Result = NOTOK;
        }
    }

    // Free the graph structures created before the loop
    gp_Free(&theGraph);
    gp_Free(&origGraph);

    return Result == OK || Result == NONEMBEDDABLE ? OK : NOTOK;
}

/****************************************************************************
 GetNumberIfZero()
 Internal function that gets a number if the given *pNum is zero.
 The prompt is displayed if the number must be obtained from the user.
 Whether the given number is used or obtained from the user, the function
 ensures it is in the range [min, max] and assigns the midpoint value if
 it is not.
 ****************************************************************************/

int GetNumberIfZero(int *pNum, char const *prompt, int min, int max)
{
    char lineBuff[MAXLINE + 1];

    memset(lineBuff, '\0', (MAXLINE + 1));

    if (pNum == NULL)
    {
        ErrorMessage("Unable to get number, as pointer to int is NULL.\n");
        return NOTOK;
    }

    if (prompt == NULL || strlen(prompt) == 0)
    {
        ErrorMessage("Invalid prompt supplied.\n");
        return NOTOK;
    }

    while (*pNum == 0)
    {
        Prompt(prompt);
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to read integer choice from stdin.\n");
            return NOTOK;
        }

        if (strlen(lineBuff) == 0 ||
            sscanf(lineBuff, " %d", pNum) != 1)
        {
            ErrorMessage("Invalid integer choice.\n");
            (*pNum) = 0;
        }
    }

    if (min < 1)
        min = 1;
    if (max < min)
        max = min;

    if (*pNum < min || *pNum > max)
    {
        char messageContents[MAXLINE + 1];
        messageContents[0] = '\0';
        *pNum = (max + min) / 2;
        sprintf(messageContents, "Number out of range [%d, %d]; changed to %d\n", min, max, *pNum);
        ErrorMessage(messageContents);
    }

    return OK;
}

/****************************************************************************
 MakeGraph()
 Internal function that makes a new graph, initializes it, and attaches an
 algorithm to it based on the command.
 ****************************************************************************/

graphP MakeGraph(int Size, char command)
{
    graphP theGraph = NULL;
    char messageContents[MAXLINE + 1];

    memset(messageContents, '\0', (MAXLINE + 1));

    if ((theGraph = gp_New()) == NULL || gp_InitGraph(theGraph, Size) != OK)
    {
        ErrorMessage("Error creating space for a graph of the given size.\n");
        gp_Free(&theGraph);
        return NULL;
    }

    if (AttachAlgorithm(theGraph, command) != OK)
    {
        sprintf(messageContents, "Unable to attach graph algorithm extension corresponding to command '%c'\n", command);
        ErrorMessage(messageContents);
        gp_Free(&theGraph);
    }

    return theGraph;
}

/****************************************************************************
 ReinitializeGraph()
 Internal function that will either reinitialize the given graph or free it
 and make a new one just like it.
 ****************************************************************************/

void ReinitializeGraph(graphP *pGraph, int ReuseGraphs, char command)
{
    if (ReuseGraphs)
        gp_ReinitializeGraph(*pGraph);
    else
    {
        graphP newGraph = MakeGraph((*pGraph)->N, command);
        gp_Free(pGraph);
        *pGraph = newGraph;
    }
}

/****************************************************************************
 Creates a random maximal planar graph, then adds 'extraEdges' edges to it.
 ****************************************************************************/

int RandomGraph(char const *const commandString, int extraEdges, int numVertices, char *outfileName, char *outfile2Name)
{
    int Result = OK;

    platform_time start, end;
    graphP theGraph = NULL, origGraph = NULL;
    int embedFlags = 0;
    char command = '\0', modifier = '\0';
    char messageContents[MAXLINE + 1];

    memset(messageContents, '\0', (MAXLINE + 1));

    if ((Result = GetCommandAndOptionalModifier(commandString, &command, &modifier)) != OK)
    {
        ErrorMessage("Unable to extract command and optional modifier character from commandString.\n");
        return Result;
    }

    if ((Result = GetEmbedFlags(command, modifier, &embedFlags)) != OK)
    {
        ErrorMessage("Unable to derive embedFlags from command and optional modifier character.\n");
        return Result;
    }

    if ((Result = GetNumberIfZero(&numVertices, "Enter number of vertices:", 1, 1000000) != OK))
    {
        ErrorMessage("Encountered unrecoverable error when prompting for numVertices.\n");
        return Result;
    }

    if ((theGraph = MakeGraph(numVertices, command)) == NULL)
        return NOTOK;

    // Acceptable downcast of time_t to unsigned int (seeding benefits from the lower bits of now)
    srand(time(NULL));

    Message("Creating the random graph...\n");
    platform_GetTime(start);
    if (gp_CreateRandomGraphEx(theGraph, 3 * numVertices - 6 + extraEdges) != OK)
    {
        ErrorMessage("gp_CreateRandomGraphEx() failed\n");
        gp_Free(&theGraph);
        return NOTOK;
    }
    platform_GetTime(end);

    sprintf(messageContents, "Created random graph with %d edges in %.3lf seconds. ", theGraph->M, platform_GetDuration(start, end));
    Message(messageContents);
    FlushConsole(stdout);

    // The user may have requested a copy of the random graph before processing
    if (outfile2Name != NULL)
    {
        if (gp_Write(theGraph, outfile2Name, WRITE_ADJLIST) != OK)
        {
            ErrorMessage("Unable to write generated random graph before embedding.\n");
            gp_Free(&theGraph);
            return NOTOK;
        }
    }

    if ((origGraph = gp_DupGraph(theGraph)) == NULL)
    {
        ErrorMessage("Unable to create copy of generated random graph before embedding.\n");
        gp_Free(&theGraph);
        return NOTOK;
    }

    // Do the requested algorithm on the randomly generated graph
    Message("Now processing\n");
    FlushConsole(stdout);

    platform_GetTime(start);
    Result = gp_Embed(theGraph, embedFlags);
    platform_GetTime(end);

    if (Result != OK && Result != NONEMBEDDABLE)
    {
        ErrorMessage("Failed to embed randomly generated graph\n");

        gp_Free(&theGraph);
        gp_Free(&origGraph);

        return NOTOK;
    }

    if (gp_SortVertices(theGraph) != OK)
    {
        ErrorMessage("Unable to sort vertices of generated random graph\n");

        gp_Free(&theGraph);
        gp_Free(&origGraph);

        return NOTOK;
    }

    if (gp_TestEmbedResultIntegrity(theGraph, origGraph, Result) != Result)
        Result = NOTOK;

    // Write what the algorithm determined and how long it took
    WriteAlgorithmResults(theGraph, Result, command, start, end, NULL);

    // On successful algorithm result, write the output file and see if the
    // user wants the edge list formatted file.
    if (Result == OK || Result == NONEMBEDDABLE)
    {
        if (outfileName != NULL && gp_Write(theGraph, outfileName, WRITE_ADJLIST) != OK)
        {
            ErrorMessage("Unable to write graph as adjacency list after successful gp_Embed() and gp_TestEmbedResultIntegrity().\n");
            Result = NOTOK;
        }

        if (Result == OK && !getQuietModeSetting() && WriteEdgeListFormat(theGraph, origGraph, extraEdges) != OK)
        {
            ErrorMessage("Encountered an error when attempting to write graph to edge list format.\n");
            Result = NOTOK;
        }
    }
    else
        ErrorMessage("Failure occurred.\n");

    gp_Free(&theGraph);
    gp_Free(&origGraph);

    FlushConsole(stdout);

    return Result;
}

int WriteEdgeListFormat(graphP theGraph, graphP origGraph, int extraEdges)
{
    char saveEdgeListFormat = '\0';
    int charsAvailForStr = 0;
    char const *messageFormat = NULL;
    char messageContents[MAXLINE + 1];
    char theFileName[MAXLINE + 1];
    char lineBuff[MAXLINE + 1];

    memset(messageContents, '\0', (MAXLINE + 1));
    memset(theFileName, '\0', (MAXLINE + 1));
    memset(lineBuff, '\0', (MAXLINE + 1));

    while (1)
    {
        Prompt("Do you want to save the generated graph in edge list format (y/n)? ");
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to read user input to indicate whether to save edge list format from stdin.\n");
            return NOTOK;
        }

        if (strlen(lineBuff) != 1 ||
            sscanf(lineBuff, " %c", &saveEdgeListFormat) != 1 ||
            !strchr(YESNOCHOICECHARS, saveEdgeListFormat))
            ErrorMessage("Invalid choice whether to save graph in edge list format.\n");
        else
        {
            saveEdgeListFormat = (char)tolower(saveEdgeListFormat);
            break;
        }
    }

    if (extraEdges > 0)
        strcpy(theFileName, "nonPlanarEdgeList.txt");
    else
        strcpy(theFileName, "maxPlanarEdgeList.txt");

    messageFormat = "Saving edge list format of original graph to \"%.*s\"\n";
    charsAvailForStr = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
    Message(messageContents);
    SaveAsciiGraph(origGraph, theFileName);

    strcat(theFileName, ".out.txt");
    messageFormat = "Saving edge list format of result to \"%.*s\"\n";
    charsAvailForStr = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    sprintf(messageContents, messageFormat, charsAvailForStr, theFileName);
#pragma GCC diagnostic pop
    Message(messageContents);
    SaveAsciiGraph(theGraph, theFileName);

    return OK;
}
