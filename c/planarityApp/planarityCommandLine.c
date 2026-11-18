/*
Copyright (c) 1997-2026, John M. Boyer
All rights reserved.
See the LICENSE.TXT file for licensing information.
*/

#include "planarity.h"
#include "../test/testMain.h"

// FIXME: to be removed once behavioural tests broken out
#include "../graphLib/io/strOrFile.h"

int callRandomGraphs(int argc, char *argv[]);
int callSpecificGraph(int argc, char *argv[]);
int callRandomMaxPlanarGraph(int argc, char *argv[]);
int callRandomNonplanarGraph(int argc, char *argv[]);
int callTestAllGraphs(int argc, char *argv[]);
int callTransformGraph(int argc, char *argv[]);

int runFaceListTest(void);
int runAddInsertEdgeTests(void);

int runReadWithExtensionAtEofTest(void);
int runHighByteRoundTripTest(void);
int runCapacityLimitTests(void);

extern char *copySparse6TestString(char const *s6Str);

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
        Result = runTestMain(argc, argv);

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

/********************************************************************
 runCapacityLimitTests()

 Exercises the issue #325 guards: a vertex count or edge capacity
 whose capacity arithmetic cannot be represented in int must be
 rejected by the public wrappers before any allocation is attempted.

 The first rejected vertex count on the default edge capacity is
 (INT_MAX - 2) / 4 / DEFAULT_EDGE_CAPACITY_FACTOR + 1, and the first
 rejected edge capacity is (INT_MAX - 2) / 4 + 1; the largest
 acceptable edge capacity is tested on the accept side too, which is
 safe because storing a capacity before gp_EnsureVertexCapacity()
 allocates nothing.
 ********************************************************************/

int runCapacityLimitTests(void)
{
    int Result = OK;
    graphP theGraph = NULL;
    int firstBadN = (int)((((long long)INT_MAX - 2) / 4) / DEFAULT_EDGE_CAPACITY_FACTOR + 1);
    int maxGoodEdgeCapacity = (int)(((long long)INT_MAX - 2) / 4);
    unsigned quietModeCache = gp_GetQuietMode();

    gp_Message("Testing rejection of excessive vertex and edge capacity requests.");

    if ((theGraph = gp_New()) == NULL)
        Result = NOTOK;

    // The first tests below intentionally cause a NOTOK to come from the APIs,
    // so we set fully quiet mode to prevent error messages on expected behaviors.
    gp_SetQuietMode(QUIETMODE_ALL);

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, INT_MAX) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureVertexCapacity(theGraph, firstBadN) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity + 1) == OK)
    {
        gp_SetQuietMode(quietModeCache);
        Result = NOTOK;
        gp_SetQuietMode(QUIETMODE_ALL);
    }

    // The end of tests intended to produce NOTOK has been reached, so we can go back
    // to normal quiet mode behavior.
    gp_SetQuietMode(quietModeCache);

    if (Result == OK && gp_EnsureEdgeCapacity(theGraph, maxGoodEdgeCapacity) != OK)
        Result = NOTOK;

    gp_Free(&theGraph);

    // The guards must not disturb ordinary use
    if (Result == OK)
    {
        if ((theGraph = gp_New()) == NULL)
            Result = NOTOK;

        if (Result == OK && gp_EnsureVertexCapacity(theGraph, 5) != OK)
            Result = NOTOK;

        gp_Free(&theGraph);
    }

    if (Result == OK)
        gp_Message("Test succeeded.\n");

    return Result;
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

    return RandomGraphs(commandString, NumGraphs, SizeOfGraphs, outfileName, FALSE, FALSE, NULL);
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
    return TestAllGraphs(commandString, infileName, outfileName, NULL, NULL);
}
