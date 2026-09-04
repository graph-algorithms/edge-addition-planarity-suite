/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "../graphLib/io/strOrFile.h"

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

int runSpecificGraphTests(void);
int runRandomGraphsTests(void);
int runGraphTransformationTests(void);
int runTestAllGraphsTests(void);
int runFaceListTest(void);
int runAddInsertEdgeTests(void);
int runHideRestoreTests(void);
int runIdentifyContractTests(void);
int runSpecificGraphTest(char const *command, char const *infileName, int inputInMemFlag);
int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag);
int runTestAllGraphsTest(char const *commandString, char const *infileName);
int runHideRestoreTest(graphP theGraph);
int runIdentifyContractTest(graphP theGraph);
int runDigraphTests(void);
int runGraphMLTests(void);
int runDrawPlanarNonplanarWriteTest(void);
int runReadWithExtensionAtEofTest(void);
int runHighByteRoundTripTest(void);
int testDirectedDFS(void);
int testPetersenDigraph(void);
int testDigraphTranspose(void);
int runGraphMLWriteTest(char const *inputFileName, char const *expectedOutputFileName);
int runBasicGraphMLWriteTest(void);

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
        gp_SetQuietMode(QUIETMODE_ALL);

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
        gp_ErrorMessage("Unsupported command line.  Here is the help for this "
                        "program.");
        helpMessage(NULL);
        Result = NOTOK;
    }

#ifdef DEBUG
    // When one builds and runs the executable in an external console from an IDE
    // such as VSCode, the external console window will close immediately upon
    // exit 0 being returned. This means that one may miss gp_Message() and
    // gp_ErrorMessage() outputs that are crucial to the debugging process.
    // Hence, if we compile with the DDEBUG flag, this means that in appconst.h
    // we #define DEBUG. That way, this prompt will appear only for debug builds,
    // and will ensure the console window stays open until the user proceeds.
    printf("\n\tPress return key to exit...\n");
    fflush(stdout);
    if (GetLineFromStdin(lineBuff, MAXLINE) != OK)
    {
        gp_ErrorMessage("Unable to fetch from stdin; exiting.");
        Result = NOTOK;
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

    if (theGraph == NULL)
    {
        gp_ErrorMessage("Unable to allocate memory for theGraph.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        Result = gp_Read(theGraph, argv[1]);
        if (Result != OK)
        {
            gp_ErrorMessage("Failed to read graph \"%.*s\"",
                            FILENAME_MAX, argv[1]);
            Result = NOTOK;
        }
    }

    if (Result == OK)
    {
        Result = gp_Embed(theGraph, EMBEDFLAGS_PLANAR);
        if (Result == OK)
        {
            if ((Result = gp_SortVertices(theGraph)) != OK)
                gp_ErrorMessage("Failed to restore original vertex labelling.");

            if (Result == OK && (Result = gp_Write(theGraph, argv[2], WRITE_ADJLIST)) != OK)
                gp_ErrorMessage("Failed to write embedding.");
        }

        else if (Result == NONEMBEDDABLE)
        {
            if (argc >= 5 && strcmp(argv[3], "-n") == 0)
            {
                if ((Result = gp_SortVertices(theGraph)) != OK)
                    gp_ErrorMessage("Failed to restore original vertex "
                                    "labelling.");

                if (Result == OK && (Result = gp_Write(theGraph, argv[4], WRITE_ADJLIST)) != OK)
                    gp_ErrorMessage("Failed to write obstruction.");
            }
        }
        else
            Result = NOTOK;
    }

    gp_Free(&theGraph);

    // In the legacy 1.x versions, OK/NONEMBEDDABLE was 0 and NOTOK was -2
    return Result == OK || Result == NONEMBEDDABLE ? 0 : -2;
}

/****************************************************************************
 Quick regression test
 ****************************************************************************/

int runQuickRegressionTests(int argc, char *argv[])
{
    char const *samplesDir = "samples";
    int samplesDirArgLocation = 2;
    int retVal = OK;
    char origDir[2 * MAXLINE + 1];

    // Skip optional -q quiet mode command-line parameter, if present
    if (argc > samplesDirArgLocation && strcmp(argv[samplesDirArgLocation], "-q") == 0)
        samplesDirArgLocation++;

    // Accept overriding sample directory command-line parameter, if present
    if (argc > samplesDirArgLocation)
        samplesDir = argv[samplesDirArgLocation];

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
                gp_Message("WARNING: Unable to change to samples directory to "
                           "run tests on samples.");
                if (chdir(origDir) != 0)
                    gp_Message("WARNING: Unable to restore the original "
                               "working directory.");

                return OK;
            }
        }
    }
    else
    {
        // New behavior if samplesDir command-line parameter was specified
        if (chdir(samplesDir) != 0)
        {
            gp_Message("WARNING: Unable to change to samples directory to run "
                       "tests on samples.");

            return OK;
        }
    }

    if (runSpecificGraphTests() != OK)
        retVal = NOTOK;
    else if (runRandomGraphsTests() != OK)
        retVal = NOTOK;
    else if (runGraphTransformationTests() != OK)
        retVal = NOTOK;
    else if (runTestAllGraphsTests() != OK)
        retVal = NOTOK;
    else if (runFaceListTest() != OK)
        retVal = NOTOK;
    else if (runAddInsertEdgeTests() != OK)
        retVal = NOTOK;
    else if (runHideRestoreTests() != OK)
        retVal = NOTOK;
    else if (runIdentifyContractTests() != OK)
        retVal = NOTOK;
    else if (runDigraphTests() != OK)
        retVal = NOTOK;
    else if (runReadWithExtensionAtEofTest() != OK)
        retVal = NOTOK;
    else if (runHighByteRoundTripTest() != OK)
        retVal = NOTOK;
    else if (runGraphMLTests() != OK)
        retVal = NOTOK;

    // All done.
    if (retVal == OK)
        gp_Message("============\n\nAll tests have succeeded.");
    else
        gp_Message("============\n\nOne or more tests FAILED.");

    if (chdir(origDir) != 0)
        gp_Message("WARNING: Unable to restore the original working directory.");

    FlushConsole(stdout);

    return retVal;
}

int runAddInsertEdgeTests(void)
{
    graphP theGraph = NULL;
    int initialEdgeCapacity;
    int edgeToDelete;
    int v;

    gp_Message("Starting Add/Insert Edge Tests");

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for add/insert edge tests.");
        return NOTOK;
    }

    if (gp_EnsureEdgeCapacity(theGraph, 2) != OK ||
        gp_EnsureVertexCapacity(theGraph, 3) != OK)
    {
        gp_ErrorMessage("Unable to initialize graph for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    v = gp_LowerBoundVertices(theGraph);
    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);

    if (gp_AddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_AddEdge(theGraph, v + 1, 1, v + 2, 0) != OK)
    {
        gp_ErrorMessage("Unable to fill initial edge capacity.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_InsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != AT_EDGE_CAPACITY_LIMIT)
    {
        gp_ErrorMessage("gp_InsertEdge() did not report the edge capacity limit.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    if (gp_DynamicInsertEdge(theGraph, v, NIL, 1, v + 2, NIL, 1) != OK ||
        gp_GetEdgeCapacity(theGraph) <= initialEdgeCapacity ||
        gp_GetM(theGraph) != 3)
    {
        gp_ErrorMessage("gp_DynamicInsertEdge() did not grow edge capacity and insert once.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    edgeToDelete = gp_FindEdge(theGraph, v, v + 1);
    if (edgeToDelete == NIL || gp_DeleteEdge(theGraph, edgeToDelete) != OK)
    {
        gp_ErrorMessage("Unable to create an edge hole for add/insert edge tests.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    initialEdgeCapacity = gp_GetEdgeCapacity(theGraph);
    if (gp_DynamicAddEdge(theGraph, v, 0, v + 1, 0) != OK ||
        gp_GetEdgeCapacity(theGraph) != initialEdgeCapacity ||
        gp_GetM(theGraph) != 3 ||
        gp_FindEdge(theGraph, v, v + 1) == NIL)
    {
        gp_ErrorMessage("gp_DynamicAddEdge() did not reuse an available edge hole.");
        gp_Free(&theGraph);
        return NOTOK;
    }

    gp_Message("Finished Add/Insert Edge Tests.\n");

    gp_Free(&theGraph);

    return OK;
}

int runRandomGraphsTests(void)
{
    int retVal = OK;
    unsigned quietModeCache = gp_GetQuietMode();
    platform_time start, end;
    double duration;

    gp_Message("Starting Random Graph Tests");
    platform_GetTime(start);

    if (RandomGraphs("-p", 1000, 20, NULL, TRUE, FALSE) != OK)
    {
        gp_ErrorMessage("gp_CreateRandomGraph() test failed.");
        retVal = NOTOK;
    }

    if (RandomGraphs("-p", 1000, 20, NULL, TRUE, TRUE) != OK)
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

/********************************************************************
 runReadWithExtensionAtEofTest()

 Reads a graph with an extension attached before gp_Read(), which
 exercises the end-of-input handling in _ReadGraph(): the extra-data
 check must see a true EOF rather than a fabricated byte.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is unsigned, (char)EOF == 255, so this
 read handed a spurious 0xFF extra-data byte to the extension's
 post-processor and failed on a valid input file.
 ********************************************************************/

int runReadWithExtensionAtEofTest(void)
{
    int Result = OK;
    graphP theGraph = NULL;

    gp_Message("Test EOF Handling (for platforms that have char unsigned).");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    if (Result == OK && gp_ExtendWith_DrawPlanar(theGraph) != OK)
        Result = NOTOK;

    if (Result == OK && gp_Read(theGraph, "maxPlanar5.txt") != OK)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    gp_Free(&theGraph);

    return Result;
}

/********************************************************************
 runHighByteRoundTripTest()

 Reads the byte 0xFF from a string container directly and through the
 unget buffer, and confirms it is never mistaken for EOF.

 Before the sf_getc()/sf_ungetc() int conversion (issue #319), on
 platforms where plain char is signed, a literal 0xFF byte fetched
 from the string container sign-extended to EOF, and the same
 happened to bytes round-tripped through the unget buffer.
 ********************************************************************/

int runHighByteRoundTripTest(void)
{
    int Result = OK;
    int currChar = EOF;
    unsigned char eofChar = 0xFF;
    char highByteStr[4];
    // char const highByteStr[] = {'a', (char)0xFF, 'b', '\0'};
    strOrFileP inputContainer = NULL;

    gp_Message("Test Support of 0xFF Byte.");

    highByteStr[0] = 'a';
    highByteStr[1] = (char)eofChar;
    highByteStr[2] = 'b';
    highByteStr[3] = '\0';

    if ((inputContainer = sf_NewInputContainer(highByteStr, NULL)) == NULL)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'a')
        Result = NOTOK;

    if (Result == OK)
    {
        currChar = sf_getc(inputContainer);
        if (currChar != 0xFF)
            Result = NOTOK;
    }

    if (Result == OK && sf_ungetc(currChar, inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 0xFF)
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != 'b')
        Result = NOTOK;

    if (Result == OK && sf_getc(inputContainer) != EOF)
        Result = NOTOK;

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    if (inputContainer != NULL)
        sf_Free(&inputContainer);

    return Result;
}

int runGraphTransformationTests(void)
{
    int retVal = OK;

    /*
        GRAPH TRANSFORMATION TESTS
    */
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

    return retVal;
}

int runTestAllGraphsTests(void)
{
    int retVal = OK;

    // Run TestAllGraphs Tests
    if (runTestAllGraphsTest("-p", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Planarity test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-d", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Planar graph drawing test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-o", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Outerplanarity test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-2", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("K2,3 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-3", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("K3,3 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }
    if (runTestAllGraphsTest("-4", "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("K4 homeomorph search test on all graphs failed.");
        retVal = NOTOK;
    }

    return retVal;
}

int runFaceListTest(void)
{
    graphP theGraph = NULL, origGraph = NULL;
    char *faceList = NULL, *drawing = NULL;
    char const *infileName = NULL, *expectedOutfileName = NULL, *expectedDrawingFileName = NULL;
    int embedResult, retVal = OK;

#ifdef USE_1BASEDARRAYS
    infileName = "faceListComponents.txt";
    expectedOutfileName = "faceListComponents.out.txt";
    expectedDrawingFileName = "faceListComponents.Drawing.txt";
#else
    infileName = "faceListComponents.0-based.txt";
    expectedOutfileName = "faceListComponents.0-based.out.txt";
    expectedDrawingFileName = "faceListComponents.0-based.Drawing.txt";
#endif

    gp_Message("Starting Face List Test");

    if ((theGraph = gp_New()) == NULL ||
        gp_Read(theGraph, infileName) != OK ||
        (origGraph = gp_DupGraph(theGraph)) == NULL ||
        gp_ExtendWith_DrawPlanar(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to set up the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    embedResult = gp_Embed(theGraph, EMBEDFLAGS_DRAWPLANAR);
    if (embedResult != OK ||
        gp_TestEmbedResultIntegrity(theGraph, origGraph, embedResult) != OK)
    {
        gp_ErrorMessage("Unable to embed the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_SortVertices(theGraph) != OK)
    {
        gp_ErrorMessage("Unable to restore original vertex labelling.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CountEmbeddingFaces(theGraph) != 7)
    {
        gp_ErrorMessage("Unexpected face count for the face list sample graph.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_CreateEmbeddingFaceList(theGraph, &faceList) != OK || faceList == NULL)
    {
        gp_ErrorMessage("Unable to create the face list sample output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (TextFileMatchesString(expectedOutfileName, faceList) != TRUE)
    {
        gp_ErrorMessage("Face list sample output did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    if (gp_DrawPlanar_RenderToString(theGraph, &drawing) != OK || drawing == NULL ||
        TextFileMatchesString(expectedDrawingFileName, drawing) != TRUE)
    {
        gp_ErrorMessage("Face list sample drawing did not match the expected output.");
        retVal = NOTOK;
        goto runFaceListTest_Cleanup;
    }

    gp_Message("Finished Face List Test.\n");

runFaceListTest_Cleanup:

    if (drawing != NULL)
    {
        free(drawing);
        drawing = NULL;
    }

    if (faceList != NULL)
    {
        free(faceList);
        faceList = NULL;
    }

    gp_Free(&origGraph);
    gp_Free(&theGraph);

    return retVal;
}

int runHideRestoreTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Hide/Restore Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for hide/restore tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for hide/restore tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for hide/restore tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runHideRestoreTest(theGraph) != OK)
        {
            gp_ErrorMessage("Hide/restore test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Hide/Restore Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

int runHideRestoreTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int v;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runHideRestoreTest() received NULL graph.");
        return NOTOK;
    }

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before hide/restore test.");
        Result = NOTOK;
    }

    for (v = gp_LowerBoundVertices(theGraph); Result == OK && v < gp_UpperBoundVertices(theGraph); ++v)
    {
        if (gp_HideVertex(theGraph, v) != OK)
        {
            gp_ErrorMessage("gp_HideVertex() failed during hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during hide/restore test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after hide/restore test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after hide/restore test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}

int runIdentifyContractTests(void)
{
    graphP theGraph = NULL;
    G6ReadIteratorP theG6ReadIterator = NULL;
    platform_time start, end;
    int Result = OK;
    int lineNum = 0;

    gp_Message("Starting Identify/Contract Tests");
    platform_GetTime(start);

    if ((theGraph = gp_New()) == NULL)
    {
        gp_ErrorMessage("Unable to allocate graph for identify/contract tests.");
        return NOTOK;
    }

    if (g6_NewReader((&theG6ReadIterator), theGraph) != OK ||
        g6_InitReaderWithFileName(theG6ReadIterator, "n8.mALL.g6") != OK)
    {
        gp_ErrorMessage("Unable to allocate or initialize G6 read iterator for identify/contract tests.");
        Result = NOTOK;
    }

    while (Result == OK)
    {
        if (g6_ReadGraph(theG6ReadIterator) != OK)
        {
            gp_ErrorMessage("Unable to read graph on line %d for identify/contract tests.", lineNum + 1);
            Result = NOTOK;
            break;
        }

        if (g6_EndReached(theG6ReadIterator))
            break;

        lineNum++;

        if (runIdentifyContractTest(theGraph) != OK)
        {
            gp_ErrorMessage("Identify/contract test failed for graph on line %d.", lineNum);
            Result = NOTOK;
            break;
        }
    }

    platform_GetTime(end);

    if (Result == OK)
        gp_Message("Done running Identify/Contract Tests (%.3lf seconds).", platform_GetDuration(start, end));

    gp_Message(" ");

    g6_FreeReader((&theG6ReadIterator));
    gp_Free(&theGraph);

    return Result;
}

int runIdentifyContractTest(graphP theGraph)
{
    char *beforeStr = NULL, *afterStr = NULL;
    int Result = OK;
    int vertexOffset;
    int pairs[7][2];
    int i;

    if (theGraph == NULL)
    {
        gp_ErrorMessage("runIdentifyContractTest() received NULL graph.");
        return NOTOK;
    }

    vertexOffset = gp_LowerBoundVertices(theGraph);

    pairs[0][0] = vertexOffset;
    pairs[0][1] = vertexOffset + 1;
    pairs[1][0] = vertexOffset + 2;
    pairs[1][1] = vertexOffset + 3;
    pairs[2][0] = vertexOffset + 4;
    pairs[2][1] = vertexOffset + 5;
    pairs[3][0] = vertexOffset + 6;
    pairs[3][1] = vertexOffset + 7;
    pairs[4][0] = vertexOffset;
    pairs[4][1] = vertexOffset + 2;
    pairs[5][0] = vertexOffset + 4;
    pairs[5][1] = vertexOffset + 6;
    pairs[6][0] = vertexOffset;
    pairs[6][1] = vertexOffset + 4;

    if (gp_WriteToString(theGraph, &beforeStr, WRITE_ADJLIST) != OK || beforeStr == NULL)
    {
        gp_ErrorMessage("Unable to write graph to string before identify/contract test.");
        Result = NOTOK;
    }

    for (i = 0; Result == OK && i < 7; i++)
    {
        if (gp_IdentifyVertices(theGraph, pairs[i][0], pairs[i][1], NIL) != OK)
        {
            gp_ErrorMessage("gp_IdentifyVertices() failed during identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && gp_RestoreVertices(theGraph) != OK)
    {
        gp_ErrorMessage("gp_RestoreVertices() failed during identify/contract test.");
        Result = NOTOK;
    }

    if (Result == OK)
    {
        if (gp_WriteToString(theGraph, &afterStr, WRITE_ADJLIST) != OK || afterStr == NULL)
        {
            gp_ErrorMessage("Unable to write graph to string after identify/contract test.");
            Result = NOTOK;
        }
    }

    if (Result == OK && strcmp(beforeStr, afterStr) != 0)
    {
        gp_ErrorMessage("Graph changed after identify/contract test.");
        Result = NOTOK;
    }

    if (beforeStr != NULL)
    {
        free(beforeStr);
        beforeStr = NULL;
    }

    if (afterStr != NULL)
    {
        free(afterStr);
        afterStr = NULL;
    }

    return Result;
}

int runTestAllGraphsTest(char const *commandString, char const *infileName)
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

    Result = TestAllGraphs(commandString, infileName, NULL, &outputStr);

    if (Result == OK)
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

int runGraphTransformationTest(char const *command, char const *infileName, int inputInMemFlag)
{
    int Result = OK;

    char *inputString = NULL;
    char transformationCode = '\0';

    // runGraphTransformationTest will not test performing an algorithm on a given
    // input graph; it will only support "-(gam)"
    if (command == NULL || strlen(command) < 2)
    {
        gp_ErrorMessage("runGraphTransformationTest only supports -(gam).");
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

    gp_Message(" ");

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

    return RandomGraphs(commandString, NumGraphs, SizeOfGraphs, outfileName, FALSE, FALSE);
}

/****************************************************************************
 callSpecificGraph()
 ****************************************************************************/

// 'planarity -s [-q] C I O [O2]': Specific graph
int callSpecificGraph(int argc, char *argv[])
{
    int offset = 0;
    char *commandString = NULL;
    char *infileName = NULL, *outfileName = NULL, *outfile2Name = NULL;

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
    int offset = 0, numVertices = 0;
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
    int offset = 0, numVertices = 0;
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
    // We don't want to write to string, so pOutputStr is NULL
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

    // NOTE: We don't want to write to string, so pOutputStr is NULL
    return TestAllGraphs(commandString, infileName, outfileName, NULL);
}
/****************************************************************************
 testDirectedDFS()
 ****************************************************************************/

int testDirectedDFS(void)
{
    graphP G = gp_New();
    graphP G1 = NULL;
    int const expectedDiscoveryTimes[] = {1, 11, 2, 3, 7, 4};
    int const expectedFinishTimes[] = {10, 12, 9, 6, 8, 5};
    int lowerVertex, v, e, source, target;
    unsigned expectedType;

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, "DirectedDFSTest.txt") != OK)
    {
        gp_ErrorMessage("Failed to read directed DFS sample.");
        gp_Free(&G);
        return NOTOK;
    }

    if (gp_DepthFirstSearchEx(G, DFSMODE_DIRECTED) != OK)
    {
        gp_ErrorMessage("Directed DFS returned an unexpected result.");
        gp_Free(&G);
        return NOTOK;
    }

    if (!(gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED_DIRECTED) ||
        (gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED))
    {
        gp_ErrorMessage("Directed DFS graph flags were not set correctly.");
        gp_Free(&G);
        return NOTOK;
    }

    lowerVertex = gp_LowerBoundVertices(G);
    for (v = lowerVertex; v < gp_UpperBoundVertices(G); ++v)
    {
        if (gp_GetIndex(G, v) != expectedDiscoveryTimes[v - lowerVertex] ||
            gp_GetVisitedIndex(G, v) != expectedFinishTimes[v - lowerVertex])
        {
            gp_ErrorMessage("Directed DFS timestamp mismatch at vertex %d: "
                            "expected (%d, %d), found (%d, %d).",
                            v - lowerVertex + 1,
                            expectedDiscoveryTimes[v - lowerVertex],
                            expectedFinishTimes[v - lowerVertex],
                            gp_GetIndex(G, v),
                            gp_GetVisitedIndex(G, v));
            gp_Free(&G);
            return NOTOK;
        }
    }

    for (e = gp_LowerBoundEdges(G); e < gp_UpperBoundEdges(G); ++e)
    {
        if (!gp_EdgeInUse(G, e) ||
            gp_GetDirection(G, e) == EDGEFLAG_DIRECTION_INONLY)
            continue;

        source = gp_GetNeighbor(G, gp_GetTwin(G, e)) - lowerVertex + 1;
        target = gp_GetNeighbor(G, e) - lowerVertex + 1;
        expectedType = EDGE_TYPE_TREE;

        if ((source == 5 && (target == 1 || target == 3)))
            expectedType = EDGE_TYPE_BACK;
        else if (source == 2 && (target == 3 || target == 4))
            expectedType = EDGE_TYPE_CROSS;
        else if (source == 1 && target == 4)
            expectedType = EDGE_TYPE_FORWARD;

        if (gp_GetEdgeType(G, e) != expectedType)
        {
            gp_ErrorMessage("Directed DFS edge type mismatch.");
            gp_Free(&G);
            return NOTOK;
        }
    }

    G1 = gp_DupGraph(G);
    if (G1 == NULL || !(gp_GetGraphFlags(G1) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Graph duplication method failed to preserve directed DFS state.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }

    // The optimized embedding initialization performs its own undirected
    // DFS and must replace the directed DFS state on a duplicated graph.
    if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != OK ||
        (gp_GetGraphFlags(G1) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Embedding did not replace directed DFS state.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }
    gp_Free(&G1);

    // The legacy DFS must remain available for directed input and replace
    // directed timestamps and flags with its original undirected DFS state.
    if (gp_DepthFirstSearchEx(G, DFSMODE_UNDIRECTED) != OK ||
        !(gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED) ||
        (gp_GetGraphFlags(G) & GRAPHFLAGS_DFSNUMBERED_DIRECTED))
    {
        gp_ErrorMessage("Undirected DFS did not replace directed DFS state.");
        gp_Free(&G);
        return NOTOK;
    }

    for (v = lowerVertex; v < gp_UpperBoundVertices(G); ++v)
    {
        if (gp_GetVisitedIndex(G, v) != 0)
        {
            gp_ErrorMessage("Undirected DFS did not clear directed finish times.");
            gp_Free(&G);
            return NOTOK;
        }
    }

    gp_Free(&G);
    return OK;
}

/****************************************************************************
 testPetersenDigraph()
 ****************************************************************************/

int testPetersenDigraph(void)
{
    graphP G = gp_New();
    graphP G1 = NULL;
    char const *inputFileName = NULL;
    int quietModeCache, v, e, eTwin, eDir, eTwinDir;
    char *dummyStr = NULL; // Safe throwaway pointer for early-outs

    if (G == NULL)
        return NOTOK;

#ifdef USE_1BASEDARRAYS
    inputFileName = "Petersen.digraph.txt";
#else
    inputFileName = "Petersen.digraph.0-based.txt";
#endif

    if (gp_Read(G, inputFileName) != OK)
    {
        gp_ErrorMessage("Failed to read Petersen digraph sample.");
        gp_Free(&G);
        return NOTOK;
    }

    //  Verify edges and flags
    if (gp_GetM(G) != 15)
    {
        gp_ErrorMessage("Petersen Digraph should have exactly 15 edges.");
        gp_Free(&G);
        return NOTOK;
    }

    // Verify the directed edge flag detected and that all edges are directed
    if (!(gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED))
    {
        gp_ErrorMessage("Directed edge flag was not set upon reading digraph.");
        gp_Free(&G);
        return NOTOK;
    }

    for (v = gp_LowerBoundVertices(G); v < gp_UpperBoundVertices(G); ++v)
    {
        e = gp_GetFirstEdge(G, v);
        while (gp_IsEdge(G, e))
        {
            eTwin = gp_GetTwin(G, e);
            eDir = gp_GetDirection(G, e);
            eTwinDir = gp_GetDirection(G, eTwin);

            if (eDir == 0 || eTwinDir == 0 || eDir == eTwinDir)
            {
                gp_ErrorMessage("Edge direction mismatch found.");
                gp_Free(&G);
                return NOTOK;
            }
            e = gp_GetNextEdge(G, e);
        }
    }

    // Test that the planarity algorithm works on digraphs
    if ((G1 = gp_DupGraph(G)) == NULL)
    {
        gp_Free(&G);
        return NOTOK;
    }

    if (gp_Embed(G1, EMBEDFLAGS_PLANAR) != NONEMBEDDABLE)
    {
        gp_ErrorMessage("Digraph embed did not return expected result.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }

    if (gp_TestEmbedResultIntegrity(G1, G, NONEMBEDDABLE) != NONEMBEDDABLE)
    {
        gp_ErrorMessage("Embed integrity check failed.");
        gp_Free(&G);
        gp_Free(&G1);
        return NOTOK;
    }

    gp_Free(&G1);

    // Run early-outs quietly (with a dummy string for write operations)
    quietModeCache = gp_GetQuietMode();
    gp_SetQuietMode(QUIETMODE_ALL);

    if (gp_DepthFirstSearch(G) != OK ||
        gp_ComputeLowpoints(G) == OK ||
        gp_ComputeLeastAncestors(G) == OK ||
        gp_WriteToString(G, &dummyStr, WRITE_G6) == OK ||
        gp_WriteToString(G, &dummyStr, WRITE_ADJMATRIX) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        gp_ErrorMessage("Digraph early-out test failed.");
        gp_Free(&G);
        if (dummyStr != NULL)
            free(dummyStr);
        return NOTOK;
    }

    gp_SetQuietMode(quietModeCache);

    //  Clear edge direction flags, and verify
    if (gp_ClearEdgeDirectionFlags(G) != OK)
    {
        gp_Free(&G);
        return NOTOK;
    }

    if (gp_GetGraphFlags(G) & GRAPHFLAGS_DIRECTEDEDGEDETECTED)
    {
        gp_ErrorMessage("Directed edge detected flag should be but is not clear.");
        gp_Free(&G);
        return NOTOK;
    }

    for (v = gp_LowerBoundVertices(G); v < gp_UpperBoundVertices(G); ++v)
    {
        e = gp_GetFirstEdge(G, v);
        while (gp_IsEdge(G, e))
        {
            eTwin = gp_GetTwin(G, e);
            eDir = gp_GetDirection(G, e);
            eTwinDir = gp_GetDirection(G, eTwin);

            if (eDir != 0 || eTwinDir != 0)
            {
                gp_ErrorMessage("Unexpected directed edge found.");
                gp_Free(&G);
                return NOTOK;
            }
            e = gp_GetNextEdge(G, e);
        }
    }

    gp_Free(&G);

    return OK;
}

/****************************************************************************
 testDigraphTranspose()
 ****************************************************************************/

int testDigraphTranspose(void)
{
    graphP G = gp_New();
    char *actualOutput = NULL;
    char const *inputFileName = NULL;
    char const *expectedOutputFileName = NULL;
    int preservedEdgeSource = NIL;
    int preservedEdgeTarget = NIL;
    int e = NIL;

    if (G == NULL)
        return NOTOK;

#ifdef USE_1BASEDARRAYS
    inputFileName = "Petersen.digraph.txt";
    expectedOutputFileName = "Digraph.transposeTest.txt";
    preservedEdgeSource = 5;
    preservedEdgeTarget = 1;
#else
    inputFileName = "Petersen.digraph.0-based.txt";
    expectedOutputFileName = "Digraph.transposeTest.0-based.txt";
    preservedEdgeSource = 4;
    preservedEdgeTarget = 0;
#endif

    if (gp_Read(G, inputFileName) != OK)
    {
        gp_ErrorMessage("Failed to read Petersen digraph sample for transpose test.");
        gp_Free(&G);
        return NOTOK;
    }

    // Preserve one undirected edge while transposing all directed edges.
    e = gp_FindDirectedEdge(G, preservedEdgeSource, preservedEdgeTarget, EDGEFLAG_DIRECTION_OUTONLY);
    if (e == NIL)
    {
        gp_ErrorMessage("Failed to find directed edge to preserve during transpose.");
        gp_Free(&G);
        return NOTOK;
    }
    gp_SetDirection(G, e, 0);

    if (gp_TransposeDirectedGraph(G) != OK)
    {
        gp_ErrorMessage("Directed graph transpose returned an unexpected result.");
        gp_Free(&G);
        return NOTOK;
    }

    if (gp_WriteToString(G, &actualOutput, WRITE_ADJLIST) != OK || actualOutput == NULL ||
        TextFileMatchesString(expectedOutputFileName, actualOutput) != TRUE)
    {
        gp_ErrorMessage("Directed graph transpose output did not match the expected sample.");
        gp_Free(&G);
        if (actualOutput != NULL)
            free(actualOutput);
        return NOTOK;
    }

    gp_Free(&G);
    free(actualOutput);
    return OK;
}

int runDigraphTests(void)
{
    int retVal = OK;

    gp_Message("Starting Digraph Tests");

    if (testDirectedDFS() != OK)
    {
        gp_ErrorMessage("Directed DFS test failed.");
        retVal = NOTOK;
    }
    else if (testPetersenDigraph() != OK)
    {
        gp_ErrorMessage("Petersen Digraph test failed.");
        retVal = NOTOK;
    }
    else if (testDigraphTranspose() != OK)
    {
        gp_ErrorMessage("Digraph transpose test failed.");
        retVal = NOTOK;
    }
    else
        gp_Message("Finished Digraph Tests.\n");

    return retVal;
}

int runGraphMLWriteTest(char const *inputFileName, char const *expectedOutputFileName)
{
    graphP G = gp_New();
    char *actualOutput = NULL;
    int Result = OK;

    if (G == NULL)
        return NOTOK;

    if (gp_Read(G, inputFileName) != OK ||
        gp_WriteToString(G, &actualOutput, WRITE_GRAPHML) != OK ||
        actualOutput == NULL ||
        TextFileMatchesString(expectedOutputFileName, actualOutput) != TRUE)
        Result = NOTOK;

    if (actualOutput != NULL)
        free(actualOutput);
    gp_Free(&G);

    return Result;
}

int runBasicGraphMLWriteTest(void)
{
    if (runGraphMLWriteTest("Digraph.transposeTest.txt", "Digraph.transposeTest.graphml") != OK ||
        runGraphMLWriteTest("Digraph.transposeTest.0-based.txt", "Digraph.transposeTest.0-based.graphml") != OK)
        return NOTOK;

    return OK;
}

int runGraphMLTests(void)
{
    int Result = OK;

    gp_Message("Starting GraphML Tests");

    if (runBasicGraphMLWriteTest() != OK)
    {
        gp_ErrorMessage("Basic GraphML write test failed.");
        Result = NOTOK;
    }
    else
        gp_Message("Finished GraphML Tests.\n");

    return Result;
}
