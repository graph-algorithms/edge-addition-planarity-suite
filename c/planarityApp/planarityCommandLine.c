/*
Copyright (c) 1997-2025, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"

#if defined(_MSC_VER) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
// MSVC under Windows doesn't have unistd.h, but does define functions like getcwd and chdir
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

int runQuickRegressionTests(int argc, char *argv[]);
int callRandomGraphs(int argc, char *argv[]);
int callSpecificGraph(int argc, char *argv[]);
int callRandomMaxPlanarGraph(int argc, char *argv[]);
int callRandomNonplanarGraph(int argc, char *argv[]);
int callTestAllGraphs(int argc, char *argv[]);
int callTransformGraph(int argc, char *argv[]);

/****************************************************************************
 Command Line Processor
 ****************************************************************************/

int commandLine(int argc, char *argv[])
{
    int Result = OK;

#ifdef DEBUG
    char lineBuff[MAXLINE + 1];
#endif

    if (argc >= 3 && strcmp(argv[2], "-q") == 0)
        setQuietModeSetting(TRUE);

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)
    {
        Result = helpMessage(argc >= 3 ? argv[2] : NULL);
    }

    else if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "-info") == 0)
    {
        Result = helpMessage(argv[1]);
    }

    else if (strcmp(argv[1], "-test") == 0)
        Result = runQuickRegressionTests(argc, argv);

    else if (strcmp(argv[1], "-r") == 0)
        Result = callRandomGraphs(argc, argv);

    else if (strcmp(argv[1], "-s") == 0)
        Result = callSpecificGraph(argc, argv);

    else if (strcmp(argv[1], "-rm") == 0)
        Result = callRandomMaxPlanarGraph(argc, argv);

    else if (strcmp(argv[1], "-rn") == 0)
        Result = callRandomNonplanarGraph(argc, argv);

    else if (strncmp(argv[1], "-x", 2) == 0)
        Result = callTransformGraph(argc, argv);

    else if (strncmp(argv[1], "-t", 2) == 0)
        Result = callTestAllGraphs(argc, argv);

    else
    {
        ErrorMessage("Unsupported command line.  Here is the help for this program.\n");
        helpMessage(NULL);
        Result = NOTOK;
    }

#ifdef DEBUG
    // When one builds and runs the executable in an external console from an IDE
    // such as VSCode, the external console window will close immediately upon
    // exit 0 being returned. This means that one may miss Messages and
    // ErrorMessages that are crucial to the debugging process. Hence, if we compile
    // with the DDEBUG flag, this means that in appconst.h we #define DEBUG. That way,
    // this prompt will appear only for debug builds, and will ensure the console
    // window stays open until the user proceeds.
    if (!getQuietModeSetting())
    {
        printf("\n\tPress return key to exit...\n");
        fflush(stdout);
        if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
        {
            ErrorMessage("Unable to fetch from stdin; exiting.\n");
            Result = NOTOK;
        }
    }
#endif

    // NOTE: Translates internal planarity codes to appropriate exit codes
    return Result == OK ? 0 : (Result == NONEMBEDDABLE ? 1 : -1);
}

/****************************************************************************
 Legacy Command Line Processor from version 1.x
 ****************************************************************************/

int legacyCommandLine(int argc, char *argv[])
{
    int Result = OK;

    graphP theGraph = gp_New();

    Result = gp_Read(theGraph, argv[1]);
    if (Result != OK)
    {
        if (Result != NONEMBEDDABLE)
        {
            char const *messageFormat = "Failed to read graph \"%.*s\"";
            char messageContents[MAXLINE + 1];
            int charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
            sprintf(messageContents, messageFormat, charsAvailForFilename, argv[1]);
#pragma GCC diagnostic pop
            ErrorMessage(messageContents);

            if (theGraph != NULL)
                gp_Free(&theGraph);

            return -2;
        }
    }

    Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);

    if (Result == OK)
    {
        gp_SortVertices(theGraph);
        gp_Write(theGraph, argv[2], WRITE_ADJLIST);
    }

    else if (Result == NONEMBEDDABLE)
    {
        if (argc >= 5 && strcmp(argv[3], "-n") == 0)
        {
            gp_SortVertices(theGraph);
            gp_Write(theGraph, argv[4], WRITE_ADJLIST);
        }
    }
    else
        Result = NOTOK;

    gp_Free(&theGraph);

    // In the legacy 1.x versions, OK/NONEMBEDDABLE was 0 and NOTOK was -2
    return Result == OK || Result == NONEMBEDDABLE ? 0 : -2;
}

/****************************************************************************
 Quick regression test
 ****************************************************************************/

int runSpecificGraphTests(char const *);
int runSpecificGraphTest(char const *command, char const *infileName, int inputInMemFlag);
int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag);

int runQuickRegressionTests(int argc, char *argv[])
{
    char const *samplesDir = "samples";
    int samplesDirArgLocation = 2;

    // Skip optional -q quiet mode command-line parameter, if present
    if (argc > samplesDirArgLocation && strcmp(argv[samplesDirArgLocation], "-q") == 0)
        samplesDirArgLocation++;

    // Accept overriding sample directory command-line parameter, if present
    if (argc > samplesDirArgLocation)
        samplesDir = argv[samplesDirArgLocation];

    return runSpecificGraphTests(samplesDir);
}

int runSpecificGraphTests(char const *samplesDir)
{
    int retVal = OK;

    char origDir[2 * MAXLINE + 1];

    memset(origDir, '\0', (2 * MAXLINE + 1));

    if (!getcwd(origDir, 2 * MAXLINE))
        return NOTOK;

    // Preserve original behavior before the samplesDir command-line parameter was available
    if (strcmp(samplesDir, "samples") == 0)
    {
        if (chdir(samplesDir) != 0)
        {
            if (chdir("..") != 0 || chdir(samplesDir) != 0)
            {
                // Give success result, but Warn if no samples (except no warning if in quiet mode)
                Message("WARNING: Unable to change to samples directory to run tests on samples.\n");

                return OK;
            }
        }
    }
    else
    {
        // New behavior if samplesDir command-line parameter was specified
        if (chdir(samplesDir) != 0)
        {
            Message("WARNING: Unable to change to samples directory to run tests on samples.\n");

            return OK;
        }
    }

#ifdef USE_FASTER_1BASEDARRAYS
    Message("\n\tStarting 1-based Array Index Tests\n\n");

    if (runSpecificGraphTest("-p", "maxPlanar5.txt", TRUE) != OK)
    {
        ErrorMessage("Planarity test on maxPlanar5.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "maxPlanar5.txt", FALSE) != OK)
    {
        ErrorMessage("Graph drawing test maxPlanar5.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "drawExample.txt", TRUE) != OK)
    {
        ErrorMessage("Graph drawing on drawExample.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-p", "Petersen.txt", FALSE) != OK)
    {
        ErrorMessage("Planarity test on Petersen.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-o", "Petersen.txt", TRUE) != OK)
    {
        ErrorMessage("Outerplanarity test on Petersen.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-2", "Petersen.txt", FALSE) != OK)
    {
        ErrorMessage("K_{2,3} search on Petersen.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-3", "Petersen.txt", TRUE) != OK)
    {
        ErrorMessage("K_{3,3} search on Petersen.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-4", "Petersen.txt", FALSE) != OK)
    {
        ErrorMessage("K_4 search on Petersen.txt failed.\n");

        retVal = NOTOK;
    }

    Message("\tFinished 1-based Array Index Tests.\n\n");
#endif

    if (runSpecificGraphTest("-p", "maxPlanar5.0-based.txt", FALSE) != OK)
    {
        ErrorMessage("Planarity test on maxPlanar5.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "maxPlanar5.0-based.txt", TRUE) != OK)
    {
        ErrorMessage("Graph drawing test maxPlanar5.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-d", "drawExample.0-based.txt", FALSE) != OK)
    {
        ErrorMessage("Graph drawing on drawExample.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-p", "Petersen.0-based.txt", TRUE) != OK)
    {
        ErrorMessage("Planarity test on Petersen.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-o", "Petersen.0-based.txt", FALSE) != OK)
    {
        ErrorMessage("Outerplanarity test on Petersen.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-2", "Petersen.0-based.txt", TRUE) != OK)
    {
        ErrorMessage("K_{2,3} search on Petersen.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-3", "Petersen.0-based.txt", FALSE) != OK)
    {
        ErrorMessage("K_{3,3} search on Petersen.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    if (runSpecificGraphTest("-4", "Petersen.0-based.txt", TRUE) != OK)
    {
        ErrorMessage("K_4 search on Petersen.0-based.txt failed.\n");

        retVal = NOTOK;
    }

    /*
        GRAPH TRANSFORMATION TESTS
    */
    //  TRANSFORM TO ADJACENCY LIST

    // runGraphTransformationTest by reading file contents into string
    if (runGraphTransformationTest("-a", "nauty_example.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming nauty_example.g6 file contents as string to adjacency list failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-a", "nauty_example.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming nauty_example.g6 using file pointer to adjacency list failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file into string
    if (runGraphTransformationTest("-a", "N5-all.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming first graph in N5-all.g6 (read as string) to adjacency list failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file pointer
    if (runGraphTransformationTest("-a", "N5-all.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming first graph in N5-all.g6 (read from file pointer) to adjacency list failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading file contents corresponding to dense graph into string
    if (runGraphTransformationTest("-a", "K10.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming K10.g6 file contents as string to adjacency list failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading dense graph from file
    if (runGraphTransformationTest("-a", "K10.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming K10.g6 using file pointer to adjacency list failed.\n");

        retVal = NOTOK;
    }

    //  TRANSFORM TO ADJACENCY MATRIX

    // runGraphTransformationTest by reading file contents into string
    if (runGraphTransformationTest("-m", "nauty_example.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming nauty_example.g6 file contents as string to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-m", "nauty_example.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming nauty_example.g6 using file pointer to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file into string
    if (runGraphTransformationTest("-m", "N5-all.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming first graph in N5-all.g6 (read as string) to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading first graph from file pointer
    if (runGraphTransformationTest("-m", "N5-all.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming first graph in N5-all.g6 (read from file pointer) to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading file contents corresponding to dense graph into string
    if (runGraphTransformationTest("-m", "K10.g6", TRUE) != OK)
    {
        ErrorMessage("Transforming K10.g6 file contents as string to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading dense graph from file
    if (runGraphTransformationTest("-m", "K10.g6", FALSE) != OK)
    {
        ErrorMessage("Transforming K10.g6 using file pointer to adjacency matrix failed.\n");

        retVal = NOTOK;
    }

    //  TRANSFORM TO .G6

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-g", "nauty_example.g6.0-based.AdjList.out.txt", TRUE) != OK)
    {
        ErrorMessage("Transforming nauty_example.g6.0-based.AdjList.out.txt using file pointer to .g6 failed.\n");

        retVal = NOTOK;
    }

    // runGraphTransformationTest by reading from file
    if (runGraphTransformationTest("-g", "K10.g6.0-based.AdjList.out.txt", TRUE) != OK)
    {
        ErrorMessage("Transforming K10.g6.0-based.AdjList.out.txt using file pointer to .g6 failed.\n");

        retVal = NOTOK;
    }

    if (retVal == OK)
        Message("Tests of all specific graphs succeeded.\n");
    else
        Message("One or more specific graph tests FAILED.\n");

    chdir(origDir);
    FlushConsole(stdout);

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
        ErrorMessage("Unable to extract command (and optionally modifier) from command string.\n");

        return NOTOK;
    }

    // The algorithm, indicated by algorithmCode, operating on 'infilename' is expected to produce
    // an output that is stored in the file named 'expectedResultFileName' (return string not owned)
    expectedPrimaryResultFileName = ConstructPrimaryOutputFilename(infileName, NULL, command);

    // SpecificGraph() can invoke gp_Read() if the graph is to be read from a file, or it can invoke
    // gp_ReadFromString() if the inputInMemFlag is set.
    if (inputInMemFlag)
    {
        inputString = ReadTextFileIntoString(infileName);
        if (inputString == NULL)
        {
            ErrorMessage("Failed to read input file into string.\n");

            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
        // Perform the indicated algorithm on the graph in the input file or string. gp_ReadFromString()
        // will handle freeing inputString.
        Result = SpecificGraph(commandString,
                               infileName, NULL, NULL,
                               inputString, &actualOutput, &actualOutput2);
    }

    if (Result != OK && Result != NONEMBEDDABLE)
    {
        ErrorMessage("Test failed (graph processor returned failure result).\n");
        Result = NOTOK;
    }
    else
    {
        // Test that the primary actual output matches the primary expected output
        if (TextFileMatchesString(expectedPrimaryResultFileName, actualOutput) == TRUE)
            Message("Test succeeded (result equal to exemplar).\n");
        else
        {
            ErrorMessage("Test failed (result not equal to exemplar).\n");

            Result = NOTOK;
        }
    }

    // Test that the secondary actual output matches the secondary expected output
    if (command == 'd' && (Result == OK || Result == NONEMBEDDABLE))
    {
        char *expectedSecondaryResultFileName = (char *)malloc(strlen(expectedPrimaryResultFileName) + strlen(".render.txt") + 1);

        if (expectedSecondaryResultFileName == NULL)
        {
            ErrorMessage("Unable to allocate memory for expected secondary output filename.\n");

            Result = NOTOK;
        }
        else
        {
            sprintf(expectedSecondaryResultFileName, "%s%s", expectedPrimaryResultFileName, ".render.txt");

            if (TextFileMatchesString(expectedSecondaryResultFileName, actualOutput2) == TRUE)
                Message("Test succeeded (secondary result equal to exemplar).\n");
            else
            {
                ErrorMessage("Test failed (secondary result not equal to exemplar).\n");

                Result = NOTOK;
            }

            if (expectedSecondaryResultFileName != NULL)
            {
                free(expectedSecondaryResultFileName);
                expectedSecondaryResultFileName = NULL;
            }
        }
    }

    Message("\n");

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

int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag)
{
    int Result = OK;

    char *inputString = NULL;
    char transformationCode = '\0';

    // runGraphTransformationTest will not test performing an algorithm on a given
    // input graph; it will only support "-(gam)"
    if (command == NULL || strlen(command) < 2)
    {
        ErrorMessage("runGraphTransformationTest only supports -(gam).\n");

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
            ErrorMessage("Failed to read input file into string.\n");

            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
        // We need to capture whether output is 0- or 1-based to construct the name of the file to compare actualOutput with
        int zeroBasedOutputFlag = 0;
        char *actualOutput = NULL;
        // We want to handle the test being run when we read from an input file or read from a string,
        // so pass both infileName and inputString. Ownership of inputString is relinquished to TransformGraph(),
        // and gp_ReadFromString() will handle freeing it.
        // We want to output to string, so we pass in the address of the actualOutput string.
        Result = TransformGraph(command, infileName, inputString, &zeroBasedOutputFlag, NULL, &actualOutput);

        if (Result != OK || actualOutput == NULL)
        {
            ErrorMessage("Failed to perform transformation.\n");

            Result = NOTOK;
        }
        else
        {
            char *expectedOutfileName = NULL;
            char const *messageFormat = NULL;
            int charsAvailForFilename = 0;
            char messageContents[MAXLINE + 1];
            messageContents[0] = '\0';

            // Final arg is baseFlag, which is dependent on whether the FLAGS_ZEROBASEDIO is set in a graphP's internalFlags
            Result = ConstructTransformationExpectedResultFilename(infileName, &expectedOutfileName, transformationCode, zeroBasedOutputFlag ? 0 : 1);

            if (Result != OK || expectedOutfileName == NULL)
            {
                ErrorMessage("Unable to construct output filename for expected transformation output.\n");

                Result = NOTOK;
            }
            else
            {
                Result = TextFileMatchesString(expectedOutfileName, actualOutput);

                if (Result == TRUE)
                {
                    messageFormat = "For the transformation %s on file \"%.*s\", actual output matched expected output file.\n";
                    charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
                    Message(messageContents);

                    Result = OK;
                }
                else
                {
                    messageFormat = "For the transformation %s on file \"%.*s\", actual output did not match expected output file.\n";
                    charsAvailForFilename = (int)(MAXLINE - strlen(messageFormat));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
                    sprintf(messageContents, messageFormat, command, charsAvailForFilename, infileName);
#pragma GCC diagnostic pop
                    ErrorMessage(messageContents);

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

    Message("\n");

    if (inputString != NULL)
    {
        free(inputString);
        inputString = NULL;
    }

    return Result;
}

/****************************************************************************
 callRandomGraphs()
 ****************************************************************************/

// 'planarity -r [-q] C K N [O]': Random graphs
int callRandomGraphs(int argc, char *argv[])
{
    int offset = 0, NumGraphs = 0, SizeOfGraphs = 0;
    char *commandString = NULL, *outfileName = NULL;

    if (argc < 5 || argc > 7)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (6 + offset))
        return NOTOK;

    commandString = argv[2 + offset];
    NumGraphs = atoi(argv[3 + offset]);
    SizeOfGraphs = atoi(argv[4 + offset]);

    if (argc == (6 + offset))
        outfileName = argv[5 + offset];

    return RandomGraphs(commandString, NumGraphs, SizeOfGraphs, outfileName);
}

/****************************************************************************
 callSpecificGraph()
 ****************************************************************************/

// 'planarity -s [-q] C I O [O2]': Specific graph
int callSpecificGraph(int argc, char *argv[])
{
    char *commandString = NULL, *infileName = NULL, *outfileName = NULL, *outfile2Name = NULL;
    int offset = 0;

    if (argc < 5)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (6 + offset))
        return NOTOK;

    commandString = argv[2 + offset];
    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    if (argc == 6 + offset)
        outfile2Name = argv[5 + offset];

    return SpecificGraph(commandString, infileName, outfileName, outfile2Name, NULL, NULL, NULL);
}

/****************************************************************************
 callRandomMaxPlanarGraph()
 ****************************************************************************/

// 'planarity -rm [-q] N O [O2]': Maximal planar random graph
int callRandomMaxPlanarGraph(int argc, char *argv[])
{
    int offset = 0, numVertices;
    char *outfileName = NULL, *outfile2Name = NULL;

    if (argc < 4)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 5)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    numVertices = atoi(argv[2 + offset]);
    outfileName = argv[3 + offset];

    if (argc == 5 + offset)
        outfile2Name = argv[4 + offset];

    return RandomGraph("-p", 0, numVertices, outfileName, outfile2Name);
}

/****************************************************************************
 callRandomNonplanarGraph()
 ****************************************************************************/

// 'planarity -rn [-q] N O [O2]': Non-planar random graph (maximal planar plus edge)
int callRandomNonplanarGraph(int argc, char *argv[])
{
    int offset = 0, numVertices;
    char *outfileName = NULL, *outfile2Name = NULL;

    if (argc < 4)
        return NOTOK;

    if (strncmp(argv[2], "-q", 2) == 0)
    {
        if (argc < 5)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    numVertices = atoi(argv[2 + offset]);
    outfileName = argv[3 + offset];

    if (argc == 5 + offset)
        outfile2Name = argv[4 + offset];

    return RandomGraph("-p", 1, numVertices, outfileName, outfile2Name);
}

/****************************************************************************
 callTransformGraph()
 ****************************************************************************/

// 'planarity -x [-q] -(gam) I O': Input file I is transformed from its given
// format to the format given by the g (g6), a (adjacency list) or m (matrix),
// and written to output file O.
int callTransformGraph(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL;

    if (argc < 5)
        return NOTOK;

    if (argv[2][0] == '-' && argv[2][1] == 'q')
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    commandString = argv[2 + offset];

    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    // We don't want to read from string, so inputStr is NULL
    // We don't want to write to string, so outputStr is NULL
    // We don't need to capture whether output is 0- or 1-based, so zeroBasedOutputFlag arg is NULL
    return TransformGraph(commandString, infileName, NULL, NULL, outfileName, NULL);
}

/****************************************************************************
 callTestAllGraphs()
 ****************************************************************************/

// 'planarity -t [-q] C I O': If the command line argument after -t [-q] is a
// recognized algorithm command C, then the input file I must be in ".g6" format
// (report an error otherwise), and the algorithm(s) indicated by C are executed
// on the graph(s) in the input file, with the results of the execution stored
// in output file O.
int callTestAllGraphs(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL;

    if (argc < 5)
        return NOTOK;

    if (argv[2][0] == '-' && argv[2][1] == 'q')
    {
        if (argc < 6)
            return NOTOK;

        offset = 1;
    }

    if (argc > (5 + offset))
        return NOTOK;

    commandString = argv[2 + offset];

    infileName = argv[3 + offset];
    outfileName = argv[4 + offset];

    // NOTE: We don't want to write to string, so outputStr is NULL
    return TestAllGraphs(commandString, infileName, outfileName, NULL);
}
